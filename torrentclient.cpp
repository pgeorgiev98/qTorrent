#include "torrentclient.h"
#include "peer.h"
#include "torrent.h"
#include "block.h"
#include "piece.h"
#include <QTcpSocket>
#include <QHostAddress>
#include <QByteArray>
#include <QDebug>

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
	connect(m_socket, SIGNAL(connected()), this, SLOT(connected()));
	connect(m_socket, SIGNAL(readyRead()), this, SLOT(readyRead()));
	connect(m_socket, SIGNAL(disconnected()), this, SLOT(finished()));
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
	m_waitingForBlock = nullptr;
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
}

void TorrentClient::readyRead() {
	QTextStream out(stdout);
	m_receivedData.push_back(m_socket->readAll());

	switch(m_status) {
	case Handshaking:
		if(!readHandshakeReply()) {
			break;
		}
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
		} else if(m_waitingForBlock == nullptr && !m_peerChoking) {
			requestPiece();
		}
		break;
	default:
		m_receivedData.clear();
		break;
	}
}

void TorrentClient::finished() {
	m_status = Created;
	qDebug() << "Connection to" << m_peer->address() << ":" << m_peer->port() << "closed:" << m_socket->errorString();
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

void TorrentClient::requestPiece() {
	Block* block = m_peer->torrent()->requestBlock(this, 16384);
	if(block == nullptr) {
		return;
	}
	int len = 13;
	int index = block->piece()->pieceNumber();
	int begin = block->begin();
	int length = block->size();

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
	qDebug() << "sending request to" << m_socket->peerAddress().toString() << "index:" << index << "begin:" << begin << "length:" << length;
	m_socket->write(message);
	m_waitingForBlock = block;
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
	out << m_peer->address() << ":" << m_peer->port() << ": ";
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
		out << "have" << endl;
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
		out << "bitfield" << endl;
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
		out << "piece index: " << index << " begin: " << begin << " length: " << blockLength << endl;
		if(m_waitingForBlock == nullptr) {
			out << "Error: was not waiting for block, but received piece!" << endl;
			break;
		}
		if(m_waitingForBlock->piece()->pieceNumber() != index ||
				m_waitingForBlock->begin() != begin ||
				m_waitingForBlock->size() != blockLength) {
			out << "Error: block info does not match requested one!" << endl;
			break;
		}
		QByteArray block;
		for(int j = 0; j < blockLength; j++) {
			block.push_back(m_receivedData[i++]);
		}
		m_waitingForBlock->setData(block);
		m_waitingForBlock = nullptr;
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
