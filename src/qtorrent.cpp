/* qTorrent - An open-source, cross-platform BitTorrent client
 * Copyright (C) 2017 Petko Georgiev
 *
 * qtorrent.cpp
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "qtorrent.h"
#include "core/torrent.h"
#include "core/torrentinfo.h"
#include "core/torrentmanager.h"
#include "core/torrentserver.h"
#include "core/localservicediscoveryclient.h"
#include "core/trackerclient.h"
#include "ui/mainwindow.h"
#include <QGuiApplication>
#include <QMessageBox>
#include <QUrlQuery>

QTorrent *QTorrent::m_instance;

QTorrent::QTorrent()
{
	m_instance = this;

	m_torrentManager = new TorrentManager;
	m_server = new TorrentServer;
	m_LSDClient = new LocalServiceDiscoveryClient;
	m_mainWindow = new MainWindow;

	// Generate random peer id that starts with 'qT'
	qsrand(QDateTime::currentMSecsSinceEpoch());
	m_peerId.push_back("qT");
	while (m_peerId.size() < 20) {
		m_peerId.push_back(char(qrand() % 256));
	}

	connect(m_LSDClient, &LocalServiceDiscoveryClient::foundPeer, this, &QTorrent::LSDPeerFound);
}

QTorrent::~QTorrent()
{
	delete m_torrentManager;
	delete m_server;
	delete m_LSDClient;
	delete m_mainWindow;
}


bool QTorrent::startServer()
{
	return m_server->startServer();
}

bool QTorrent::resumeTorrents()
{
	if (m_torrentManager->resumeTorrents()) {
		return true;
	}
	critical("Failed to resume torrents: " + m_torrentManager->errorString());
	return false;
}

void QTorrent::startLSDClient()
{
	m_LSDClient->announce();
	connect(m_torrentManager, &TorrentManager::torrentAdded, m_LSDClient, &LocalServiceDiscoveryClient::announce);
}

bool QTorrent::addTorrentFromLocalFile(const QString &filename, const TorrentSettings &settings)
{
	Torrent *torrent = m_torrentManager->addTorrentFromLocalFile(filename, settings);
	if (torrent == nullptr) {
		warning("Failed to add torrent: " + m_torrentManager->errorString());
		return false;
	}
	if (!m_torrentManager->saveTorrentsResumeInfo()) {
		critical("Failed to save torrent resume info: " + m_torrentManager->errorString());
	}
	return true;
}

bool QTorrent::addTorrentFromMagnetLink(QUrl url)
{
	Torrent *torrent = torrentManager()->addTorrentFromMagnetLink(url);
	if (torrent == nullptr) {
		return false;
	}
	return true;
}

bool QTorrent::addTorrentFromUrl(QUrl url)
{
	if (url.isLocalFile()) {
		m_mainWindow->addTorrentFromUrl(url);
	} else if (url.scheme() == "magnet") {

	}
	return false;
}

bool QTorrent::removeTorrent(Torrent *torrent, bool deleteData)
{
	return m_torrentManager->removeTorrent(torrent, deleteData);
}

void QTorrent::shutDown()
{
	for (Torrent *torrent : torrents()) {
		torrent->stop();
	}
	if (!m_torrentManager->saveTorrentsResumeInfo()) {
		critical("Failed to save torrents resume info: " + m_torrentManager->errorString());
	}
}

void QTorrent::showMainWindow()
{
	m_mainWindow->show();
}


/* Message Boxes */

void QTorrent::critical(const QString &text)
{
	QMessageBox::critical(m_mainWindow, QGuiApplication::applicationDisplayName(), text);
}

void QTorrent::information(const QString &text)
{
	QMessageBox::information(m_mainWindow, QGuiApplication::applicationDisplayName(), text);
}

bool QTorrent::question(const QString &text)
{
	QMessageBox::StandardButton ans;
	ans = QMessageBox::question(m_mainWindow, QGuiApplication::applicationDisplayName(), text);
	return ans == QMessageBox::Yes;
}

void QTorrent::warning(const QString &text)
{
	QMessageBox::warning(m_mainWindow, QGuiApplication::applicationDisplayName(), text);
}


const QByteArray &QTorrent::peerId() const
{
	return m_peerId;
}


const QList<Torrent *> &QTorrent::torrents() const
{
	return m_torrentManager->torrents();
}

TorrentManager *QTorrent::torrentManager()
{
	return m_torrentManager;
}

TorrentServer *QTorrent::server()
{
	return m_server;
}


MainWindow *QTorrent::mainWindow()
{
	return m_mainWindow;
}

QTorrent *QTorrent::instance()
{
	return m_instance;
}


void QTorrent::LSDPeerFound(QHostAddress address, int port, Torrent *torrent)
{
	torrent->connectToPeer(address, port);
}
