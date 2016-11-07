#include "torrentclient.h"
#include "peer.h"
#include "torrent.h"
#include "block.h"
#include "piece.h"
#include <QTcpSocket>
#include <QHostAddress>
#include <QByteArray>
#include <QDebug>

const int REPLY_TIMEOUT_MSEC = 20000;
const int HANDSHAKE_TIMEOUT_MSEC = 20000;

const int BLOCKS_TO_REQUEST = 5;

TorrentClient::TorrentClient(Peer* peer) :
	m_socket(new QTcpSocket),
	m_peer(peer)
{
	auto torrentInfo = m_peer->torrent()->torrentInfo();
	qint64 torrentLength = torrentInfo->length();
	qint64 pieceLength = torrentInfo->pieceLength();
	int bitfieldSize = torrentLength/pieceLength;
	if(torrentLength%pieceLength != 0) {
		bitfieldSize++;
	}
	if(bitfieldSize%8 != 0) {
		bitfieldSize += 8 - bitfieldSize%8;
	}
	m_peer->bitfieldSize() = bitfieldSize;
	bool* bitfield = new bool[bitfieldSize];
	for(int i = 0; i < bitfieldSize; i++) {
		bitfield[i] = 0;
	}
	m_peer->bitfield() = bitfield;
	m_status = Created;
	m_waitingForBlocks.clear();

	// Connection events
	connect(m_socket, SIGNAL(connected()), this, SLOT(connected()));
	connect(m_socket, SIGNAL(readyRead()), this, SLOT(readyRead()));
	connect(m_socket, SIGNAL(disconnected()), this, SLOT(finished()));

	// Timeout callbacks
	connect(&m_replyTimeoutTimer, SIGNAL(timeout()), this, SLOT(replyTimeout()));
	connect(&m_handshakeTimeoutTimer, SIGNAL(timeout()), this, SLOT(handshakeTimeout()));
	// Timeout intervals
	m_replyTimeoutTimer.setInterval(REPLY_TIMEOUT_MSEC);
	m_handshakeTimeoutTimer.setInterval(HANDSHAKE_TIMEOUT_MSEC);
}

TorrentClient::~TorrentClient() {
}

Peer* TorrentClient::peer() {
	return m_peer;
}

void TorrentClient::connectToPeer() {
	m_status = Connecting;
	qDebug() << "Connecting to" << m_peer->address() << ":" << m_peer->port();
	m_socket->connectToHost(m_peer->address(), m_peer->port());
}

void TorrentClient::connected() {
	m_status = Handshaking;
	m_amChoking = true;
	m_amInterested = false;
	m_peerChoking = true;
	m_peerInterested = false;
	m_waitingForBlocks.clear();
	m_receivedData.clear();
	qDebug() << "Connected to" << m_peer->address() << ":" << m_peer->port();
	QByteArray dataToWrite;
	dataToWrite.push_back(char(19));
	dataToWrite.push_back("BitTorrent protocol");
	for(int i = 0; i < 8; i++) {
		dataToWrite.push_back(char(0));
	}
	dataToWrite.push_back(m_peer->torrent()->torrentInfo()->infoHash());
	dataToWrite.push_back("ThisIsNotAFakePeerId");
	m_socket->write(dataToWrite);
	m_handshakeTimeoutTimer.start();
}

void TorrentClient::readyRead() {
	QTextStream out(stdout);
	m_receivedData.push_back(m_socket->readAll());

	switch(m_status) {
	case Handshaking:
		if(!readHandshakeReply()) {
			break;
		}
		m_handshakeTimeoutTimer.stop();
		out << "Handshaking completed with peer " << m_peer->address() << ":" << m_peer->port() << endl;
		out << "protocol: " << m_peer->protocol() << endl;
		out << "reserved: " << m_peer->reserved().toHex() << endl;
		out << "infoHash: " << m_peer->infoHash().toHex() << endl;
		out << "peerId: " << m_peer->peerId().toHex() << endl;
		m_status = ConnectionEstablished;
	case ConnectionEstablished:
		while(readPeerMessage());
		if(!m_amInterested) {
			m_amInterested = true;
			QByteArray message;
			message.push_back((char)0);
			message.push_back((char)0);
			message.push_back((char)0);
			message.push_back((char)1);
			message.push_back((char)2);
			m_socket->write(message);
			qDebug() << "interested in" << m_socket->peerAddress().toString();
		} else if(!m_peerChoking) {
			while(m_waitingForBlocks.size() < BLOCKS_TO_REQUEST) {
				if(!requestPiece()) {
					break;
				}
			}
		}
		break;
	default:
		m_receivedData.clear();
		break;
	}
}

void TorrentClient::finished() {
	m_handshakeTimeoutTimer.stop();
	m_replyTimeoutTimer.stop();
	for(auto block : m_waitingForBlocks) {
		m_peer->torrent()->deleteBlock(block);
	}
	m_status = Created;
	m_waitingForBlocks.clear();
	qDebug() << "Connection to" << m_peer->address() << ":" << m_peer->port() << "closed:" << m_socket->errorString();
}

void TorrentClient::replyTimeout() {
	qDebug() << "Peer" << m_peer->address() << ":" << m_peer->port() << "took too long to reply!";
	m_replyTimeoutTimer.stop();
	disconnect();
}

void TorrentClient::handshakeTimeout() {
	qDebug() << "Peer" << m_peer->address() << ":" << m_peer->port() << "took too long to handshake!";
	m_handshakeTimeoutTimer.stop();
	disconnect();
}

bool TorrentClient::readHandshakeReply() {
	if(m_receivedData.isEmpty()) {
		return false;
	}
	int i = 0;
	int protocolLength = m_receivedData[i++];
	if(m_receivedData.size() < 49 + protocolLength) {
		return false;
	}
	for(int j = 0; j < protocolLength; j++) {
		m_peer->protocol().push_back(m_receivedData[i++]);
	}
	for(int j = 0; j < 8; j++) {
		m_peer->reserved().push_back(m_receivedData[i++]);
	}
	for(int j = 0; j < 20; j++) {
		m_peer->infoHash().push_back(m_receivedData[i++]);
	}
	for(int j = 0; j < 20; j++) {
		m_peer->peerId().push_back(m_receivedData[i++]);
	}
	m_receivedData.remove(0, 49 + protocolLength);
	return true;
}

bool TorrentClient::requestPiece() {
	Block* block = m_peer->torrent()->requestBlock(this, 16384);
	if(block == nullptr) {
		return false;
	}
	int len = 13;
	Torrent* torrent = m_peer->torrent();
	int index = torrent->blockPieceNumber(block);
	int begin = torrent->blockBeginIndex(block);
	int length = torrent->blockSize(block);

	QByteArray message;
	for(int i = 0, var = len, div = 256*256*256; i < 4; i++) {
		message.push_back((char)(var/div));
		var %= div;
		div /= 256;
	}
	message.push_back((char)6);
	for(int i = 0, var = index, div = 256*256*256; i < 4; i++) {
		message.push_back((char)(var/div));
		var %= div;
		div /= 256;
	}
	for(int i = 0, var = begin, div = 256*256*256; i < 4; i++) {
		message.push_back((char)(var/div));
		var %= div;
		div /= 256;
	}
	for(int i = 0, var = length, div = 256*256*256; i < 4; i++) {
		message.push_back((char)(var/div));
		var %= div;
		div /= 256;
	}
	//qDebug() << "sending request to" << m_socket->peerAddress().toString() << "index:" << index << "begin:" << begin << "length:" << length;
	m_socket->write(message);
	m_replyTimeoutTimer.start();
	m_waitingForBlocks.push_back(block);
	return true;
}

bool TorrentClient::readPeerMessage() {
	QTextStream out(stdout);
	if(m_receivedData.size() < 4) {
		return false;
	}
	int i = 0;
	int length = 0;
	for(int j = 0; j < 4; j++) {
		length *= 256;
		length += (unsigned char)m_receivedData[i++];
	}
	if(length == 0) { // keep-alive
		out << m_peer->address() << ":" << m_peer->port() << ": keep-alive" << endl;
		m_receivedData.remove(0, i);
		return true;
	}
	if(m_receivedData.size() < 4+length) {
		return false;
	}
	int messageId = m_receivedData[i++];
	//out << m_peer->address() << ":" << m_peer->port() << ": ";
	switch(messageId) {
	case 0: // choke
		out << "choke" << endl;
		m_peerChoking = true;
		break;
	case 1: // unchoke
		out << "unchoke" << endl;
		m_peerChoking = false;
		break;
	case 2: // interested
		out << "interested" << endl;
		m_peerInterested = true;
		break;
	case 3: // not interested
		out << "not interested" << endl;
		m_peerInterested = false;
		break;
	case 4: // have
	{
		//out << "have" << endl;
		int piece = 0;
		for(int j = 0; j < 4; j++) {
			piece *= 256;
			piece += (unsigned char)m_receivedData[i++];
		}
		m_peer->bitfield()[piece] = 1;
		/*
		out << "piece " << piece << endl;
		for(int j = 0; j < m_peer->bitfieldSize(); j++) {
			out << m_peer->bitfield()[j];
		}
		out << endl;
		*/
		break;
	}
	case 5: // bitfield
	{
		//out << "bitfield" << endl;
		int bitfieldSize = length-1;
		if(bitfieldSize*8 != m_peer->bitfieldSize()) {
			out << "Error: wrong bitfield sizes of " << bitfieldSize*8 << "; expected " << m_peer->bitfieldSize() << endl;
		} else {
			for(int j = 0; j < bitfieldSize; j++) {
				unsigned char byte = m_receivedData[i++];
				unsigned char pos = 0b10000000;
				for(int q = 0; q < 8; q++) {
					m_peer->bitfield()[j*8 + q] = ((byte & pos) != 0);
					pos = pos >> 1;
				}
			}
			/*
			out << endl;
			for(int j = 0; j < m_peer->bitfieldSize(); j++) {
				out << m_peer->bitfield()[j];
			}
			out << endl;
			*/
		}
		break;
	}
	case 6: // request
		out << "request" << endl;
		break;
	case 7: // piece
	{
		int index = 0;
		int begin = 0;
		int blockLength = length - 9;
		for(int j = 0; j < 4; j++) {
			index *= 256;
			index += (unsigned char)m_receivedData[i++];
		}
		for(int j = 0; j < 4; j++) {
			begin *= 256;
			begin += (unsigned char)m_receivedData[i++];
		}
		//out << "piece index: " << index << " begin: " << begin << " length: " << blockLength << endl;
		if(m_waitingForBlocks.isEmpty()) {
			out << "Error: was not waiting for block, but received piece!" << endl;
			break;
		}
		Torrent* torrent = m_peer->torrent();
		Block* block = nullptr;
		int blockIndex = 0;
		for(auto b : m_waitingForBlocks) {
			if(torrent->blockPieceNumber(b) == index &&
					torrent->blockBeginIndex(b) == begin &&
					torrent->blockSize(b) == blockLength) {
				block = b;
				break;
			}
			blockIndex++;
		}
		if(block == nullptr) {
			out << "Error: block info does not match any of the requested ones!" << endl;
			break;
		}
		m_replyTimeoutTimer.stop();
		const char* blockData = m_receivedData.data() + i;
		torrent->blockSetData(block, blockData, blockLength);
		m_waitingForBlocks.removeAt(blockIndex);
		break;
	}
	case 8: // cancel
		out << "cancel" << endl;
		break;
	case 9: // port
		out << "port" << endl;
		break;
	}
	m_receivedData.remove(0, 4 + length);
	return true;
}

void TorrentClient::disconnect() {
	qDebug() << "Disconnecting from" << m_peer->address() << ":" << m_peer->port();
	m_socket->close();
}
