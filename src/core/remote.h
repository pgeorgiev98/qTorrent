/* qTorrent - An open-source, cross-platform BitTorrent client
 * Copyright (C) 2017 Petko Georgiev
 *
 * remote.h
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

#ifndef REMOTE_H
#define REMOTE_H

#include <QObject>

class QLocalSocket;
class QLocalServer;

/* This class is used, so that the application won't be started twice
 * It uses QLocalServer and QLocalSocket. When the application is
 * started for the second time, it sends a message to the first one
 * to show the main window.
 * It could be used for remote access to the client if modified a little
 */
class Remote : public QObject
{
	Q_OBJECT

public:
	Remote();
	~Remote();

	bool start();

	void sendShowWindow();
	void showWindow();

public slots:
	void newConnection();
	void disconnected();
	void readyRead();

private:
	QLocalServer *m_server;
	QLocalSocket *m_socket;
	QByteArray m_buffer;

	void readMessages();
};

#endif // REMOTE_H
