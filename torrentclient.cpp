#include "torrentclient.h"
#include "peer.h"
#include "torrent.h"
#include <QTcpSocket>
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

void TorrentClient::connectToPeer() {
	m_status = Connecting;
	qDebug() << "Connecting to" << m_peer->address() << ":" << m_peer->port();
	m_socket->connectToHost(m_peer->address(), m_peer->port());
}

void TorrentClient::connected() {
	m_status = Handshaking;
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
		break;
	case 1: // unchoke
		out << "unchoke" << endl;
		break;
	case 2: // interested
		out << "interested" << endl;
		break;
	case 3: // not interested
		out << "not interested" << endl;
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
		out << "piece " << piece << endl;
		for(int j = 0; j < m_peer->bitfieldSize(); j++) {
			out << m_peer->bitfield()[j];
		}
		out << endl;
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
			out << endl;
			for(int j = 0; j < m_peer->bitfieldSize(); j++) {
				out << m_peer->bitfield()[j];
			}
			out << endl;
		}
		break;
	}
	case 6: // request
		out << "request" << endl;
		break;
	case 7: // piece
		out << "piece" << endl;
		break;
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
