#include "qtorrent.h"
#include "core/torrent.h"
#include "core/torrentinfo.h"
#include "core/torrentmanager.h"
#include "core/torrentserver.h"
#include "core/trackerclient.h"
#include "ui/mainwindow.h"
#include <QGuiApplication>
#include <QMessageBox>
#include <QUrlQuery>

QTorrent::QTorrent()
	: m_torrentManager(new TorrentManager(this))
	, m_server(new TorrentServer(this))
	, m_mainWindow(new MainWindow(this))
{
	// Generate random peer id that starts with 'qT'
	qsrand(QDateTime::currentMSecsSinceEpoch());
	m_peerId.push_back("qT");
	while(m_peerId.size() < 20) {
		m_peerId.push_back(char(qrand() % 256));
	}
}

QTorrent::~QTorrent() {
	delete m_torrentManager;
	delete m_server;
}


bool QTorrent::startServer() {
	return m_server->startServer();
}

bool QTorrent::resumeTorrents() {
	return m_torrentManager->resumeTorrents();
}

bool QTorrent::addTorrentFromLocalFile(const QString& filename, const QString& downloadLocation) {
	Torrent* torrent = m_torrentManager->addTorrentFromLocalFile(filename, downloadLocation);
	if(torrent == nullptr) {
		delete torrent;
		return false;
	}
	m_torrentManager->saveTorrentsResumeInfo();
	return true;
}

bool QTorrent::addTorrentFromMagnetLink(QUrl url) {
	Torrent* torrent = torrentManager()->addTorrentFromMagnetLink(url);
	if(torrent == nullptr) {
		delete torrent;
		return false;
	}
	return true;
}

bool QTorrent::addTorrentFromUrl(QUrl url) {
	/*
	if(url.isLocalFile()) {
		// It's a local file
		return addTorrentFromLocalFile(url.toLocalFile());
	} else if(url.scheme() == "magnet") {
		// It's a magnet link
		return addTorrentFromMagnetLink(url);
	}
	*/
	return false;
}

void QTorrent::shutDown() {
	for(Torrent* torrent : torrents()) {
		TrackerClient* tracker = torrent->trackerClient();
		tracker->announce(TrackerClient::Stopped);
	}
	m_torrentManager->saveTorrentsResumeInfo();
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


const QByteArray& QTorrent::peerId() const {
	return m_peerId;
}

QByteArray QTorrent::peerIdPercentEncoded() const {
	QByteArray encoded;
	for(char b : m_peerId) {
		encoded += '%';
		encoded += QByteArray::number(b, 16).right(2).rightJustified(2, '0');
	}
	return encoded;
}


const QList<Torrent*>& QTorrent::torrents() const {
	return m_torrentManager->torrents();
}

TorrentManager* QTorrent::torrentManager() {
	return m_torrentManager;
}

TorrentServer* QTorrent::server() {
	return m_server;
}


MainWindow* QTorrent::mainWindow() {
	return m_mainWindow;
}
