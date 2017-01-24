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
	QUrlQuery query(url);

	QString infoHashString;
	QByteArray infoHash;

	QByteArray trackerUrl;

	QString displayName;

	// Every magnet link must have an info hash
	if(!query.hasQueryItem("xt")) {
		return false;
	}

	// We can only support magnet links with trackers
	// Because those without trackers require DHT
	if(!query.hasQueryItem("tr")) {
		return false;
	}

	// Read info hash
	infoHashString = query.queryItemValue("xt");
	if(!infoHashString.startsWith("urn:btih:")) {
		return false;
	}
	infoHashString.remove(0, strlen("urn:btih:"));
	infoHash = QByteArray::fromHex(infoHashString.toLatin1());

	// Read the tracker url
	trackerUrl = QByteArray::fromPercentEncoding(query.queryItemValue("tr").toLatin1());

	// Read display name
	displayName = query.queryItemValue("dn");

	// TODO
	return false;
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
