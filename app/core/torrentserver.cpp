/* qTorrent - An open-source, cross-platform BitTorrent client
 * Copyright (C) 2017 Petko Georgiev
 *
 * torrentserver.cpp
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

#include "torrentserver.h"
#include "peer.h"
#include <QSettings>
#include <QDebug>

TorrentServer::TorrentServer()
{
	connect(&m_server, SIGNAL(newConnection()), this, SLOT(newConnection()));
}

TorrentServer::~TorrentServer()
{
	disconnect(&m_server, SIGNAL(newConnection()), this, SLOT(newConnection()));
}

bool TorrentServer::startServer()
{
	QSettings settings;
	quint64 startPort = settings.value("ServerStartPort", 6881).toInt();
	quint64 endPort = settings.value("ServerEndPort", 6889).toInt();
	settings.setValue("ServerStartPort", startPort);
	settings.setValue("ServerEndPort", endPort);

	if (endPort == 0)
		endPort = startPort;

	// Get a port in the range [startPort, endPort]
	if (startPort > 0) {
		for (quint16 port = startPort; port <= endPort; ++port) {
			if (m_server.listen(QHostAddress::Any, port)) {
				qDebug() << "Server started on port" << QString::number(port);
				return true;
			}
		}
	}

	// Just try to get a free port
	if (!m_server.listen()) {
		qDebug() << "Failed to start server:" << m_server.errorString();
		return false;
	}
	qDebug() << "Server started on port" << QString::number(m_server.serverPort());
	return true;
}

void TorrentServer::newConnection()
{
	QTcpSocket *socket = m_server.nextPendingConnection();
	Peer *peer = Peer::createClient(socket);
	m_peers.push_back(peer);
}

QTcpServer &TorrentServer::server()
{
	return m_server;
}

int TorrentServer::port()
{
	return m_server.serverPort();
}

QHostAddress TorrentServer::address()
{
	return m_server.serverAddress();
}

QList<Peer *> &TorrentServer::peers()
{
	return m_peers;
}
