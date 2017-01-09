#include "qtorrent.h"
#include "core/torrent.h"
#include "core/torrentserver.h"

QTorrent::QTorrent() {
	m_server = new TorrentServer(this);
}

QTorrent::~QTorrent() {
	for(auto torrent : m_torrents) {
		delete torrent;
	}
	delete m_server;
}

bool QTorrent::startServer() {
	return m_server->startServer();
}

bool QTorrent::addTorrent(const QString &filename) {
	Torrent* torrent = new Torrent(this);
	if(!torrent->createFromFile(filename)) {
		delete torrent;
		return false;
	}
	m_torrents.push_back(torrent);
	return true;
}

QList<Torrent*>& QTorrent::torrents() {
	return m_torrents;
}

TorrentServer* QTorrent::server() {
	return m_server;
}
