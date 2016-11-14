#include "peer.h"
#include "torrentclient.h"
#include <QTcpSocket>
#include <QDebug>

Peer::Peer(Torrent* torrent, const QByteArray &address, int port) :
	m_torrent(torrent),
	m_address(address),
	m_port(port),
	m_torrentClient(new TorrentClient(this))
{
}

Peer::~Peer() {
	delete m_torrentClient;
	delete[] m_bitfield;
}

Torrent* Peer::torrent() {
	return m_torrent;
}

QByteArray& Peer::address() {
	return m_address;
}

int Peer::port() {
	return m_port;
}

TorrentClient* Peer::torrentClient() {
	return m_torrentClient;
}

void Peer::startConnection() {
	m_torrentClient->connectToPeer();
}

int& Peer::bitfieldSize() {
	return m_bitfieldSize;
}

bool*& Peer::bitfield() {
	return m_bitfield;
}

QByteArray& Peer::protocol() {
	return m_protocol;
}

QByteArray& Peer::reserved() {
	return m_reserved;
}

QByteArray& Peer::infoHash() {
	return m_infoHash;
}

QByteArray& Peer::peerId() {
	return m_peerId;
}
