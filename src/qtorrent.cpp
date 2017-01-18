#include "qtorrent.h"
#include "core/torrent.h"
#include "core/torrentserver.h"
#include "ui/mainwindow.h"

QTorrent::QTorrent()
	: m_server(new TorrentServer(this))
	, m_mainWindow(new MainWindow(this))
{

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
	m_mainWindow->addTorrent(torrent);
	return true;
}


void QTorrent::showMainWindow() {
	m_mainWindow->show();
}



QList<Torrent*>& QTorrent::torrents() {
	return m_torrents;
}

TorrentServer* QTorrent::server() {
	return m_server;
}


MainWindow* QTorrent::mainWindow() {
	return m_mainWindow;
}
