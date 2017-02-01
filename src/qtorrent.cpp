#include "qtorrent.h"
#include "core/torrent.h"
#include "core/torrentserver.h"
#include "ui/mainwindow.h"
#include <QGuiApplication>
#include <QMessageBox>
#include <QUrlQuery>

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

bool QTorrent::addTorrent(const QString &filename, const QString& downloadPath) {
	Torrent* torrent = new Torrent(this);
	if(!torrent->createFromFile(filename, downloadPath)) {
		warning("Failed to read torrent file\n" + torrent->errorString());
		delete torrent;
		return false;
	}
	m_torrents.push_back(torrent);
	m_mainWindow->addTorrent(torrent);
	return true;
}

bool QTorrent::addTorrentFromMagnetLink(QUrl url) {
	Torrent* torrent = new Torrent(this);
	if(!torrent->createFromMagnetLink(url)) {
		warning("Failed to load torrent from magnet link\n" + torrent->errorString());
		delete torrent;
		return false;
	}
	m_torrents.push_back(torrent);
	m_mainWindow->addTorrent(torrent);
	return true;
}

bool QTorrent::addTorrentFromUrl(QUrl url) {
	if(url.isLocalFile()) {
		// It's a local file
		QString downloadLocation = m_mainWindow->getDownloadLocation();
		if(!downloadLocation.isEmpty()) {
			return addTorrent(url.toLocalFile(), downloadLocation);
		}
	} else if(url.scheme() == "magnet") {
		// It's a magnet link
		return addTorrentFromMagnetLink(url);
	}
	return false;
}


void QTorrent::showMainWindow() {
	m_mainWindow->show();
}


/* Message Boxes */

void QTorrent::critical(const QString &text) {
	QMessageBox::critical(m_mainWindow, QGuiApplication::applicationDisplayName(), text);
}

void QTorrent::information(const QString &text) {
	QMessageBox::information(m_mainWindow, QGuiApplication::applicationDisplayName(), text);
}

bool QTorrent::question(const QString &text) {
	QMessageBox::StandardButton ans;
	ans = QMessageBox::question(m_mainWindow, QGuiApplication::applicationDisplayName(), text);
	return ans == QMessageBox::Yes;
}

void QTorrent::warning(const QString &text) {
	QMessageBox::warning(m_mainWindow, QGuiApplication::applicationDisplayName(), text);
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
