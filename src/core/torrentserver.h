/* qTorrent - An open-source, cross-platform BitTorrent client
 * Copyright (C) 2017 Petko Georgiev
 *
 * torrentserver.h
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

#ifndef TORRENTSERVER_H
#define TORRENTSERVER_H

#include <QObject>
#include <QTcpServer>
#include <QList>

class Peer;

/* This class is used to receive and handle incoming peer connections */
class TorrentServer : public QObject {
	Q_OBJECT

public:
	TorrentServer();
	~TorrentServer();

	/* Start server on port 'port'. If port is 0, then
	 * server will automatically choose port */
	bool startServer(int port = 0);

	QTcpServer& server();
	int port();
	QHostAddress address();
	QList<Peer*>& peers();

public slots:
	void newConnection();

private:
	QTcpServer m_server;
	QList<Peer*> m_peers;
};

#endif // TORRENTSERVER_H
