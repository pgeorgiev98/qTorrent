#include "qtorrent.h"
#include "torrent.h"

QTorrent::QTorrent() {
}

QTorrent::~QTorrent() {
	for(auto torrent : m_torrents) {
		delete torrent;
	}
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
