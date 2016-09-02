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

void Peer::startConnection() {
	m_torrentClient->connectToPeer();
}
