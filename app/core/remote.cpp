/* qTorrent - An open-source, cross-platform BitTorrent client
 * Copyright (C) 2017 Petko Georgiev
 *
 * remote.cpp
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

#include "remote.h"
#include "qtorrent.h"
#include "ui/mainwindow.h"
#include <QLocalSocket>
#include <QLocalServer>

#define SERVER_NAME ".qTorrent-localServer-socket"
#define TIMEOUT 100 // milliseconds connection time-out

Remote::Remote()
	: m_server(new QLocalServer)
	, m_socket(nullptr)
{
	connect(m_server, SIGNAL(newConnection()), this, SLOT(newConnection()));
}

Remote::~Remote()
{
	delete m_server;
	if (m_socket) delete m_socket;
}

bool Remote::start()
{
	m_socket = new QLocalSocket;
	m_socket->connectToServer(SERVER_NAME);
	if (m_socket->waitForConnected(TIMEOUT)) {
		// Connected to main application instance
		sendShowWindow();
		return false;
	}

	// This is the main instance
	// Try to start server
	delete m_socket;
	m_socket = nullptr;
	bool serverStarted = false;
	if (!m_server->listen(SERVER_NAME)) {
		// For UNIX - remove server and retry
		if (m_server->serverError() == QAbstractSocket::AddressInUseError) {
			if (QLocalServer::removeServer(SERVER_NAME)) {
				if (m_server->listen(SERVER_NAME)) {
					serverStarted = true;
				}
			}
		}
	} else {
		serverStarted = true;
	}

	return serverStarted;
}

void Remote::newConnection()
{
	if (!m_socket) {
		m_socket = m_server->nextPendingConnection();
		connect(m_socket, SIGNAL(disconnected()), this, SLOT(disconnected()));
		connect(m_socket, SIGNAL(readyRead()), this, SLOT(readyRead()));
	}
}

void Remote::disconnected()
{
	m_socket->deleteLater();
	m_buffer.clear();
	m_socket = nullptr;
}

void Remote::readyRead()
{
	m_buffer.append(m_socket->readAll());
	readMessages();
}

void Remote::readMessages()
{
	QList<QByteArray> readData = m_buffer.split('\n');
	// Read commands
	for (const QByteArray& command : readData) {
		// Only one command is supported - show window
		if (command == "0") {
			showWindow();
		}
	}

	// Remove read data from buffer
	int last = m_buffer.lastIndexOf('\n');
	if (last == -1) {
		return;
	}
	m_buffer.remove(0, last+1);
}

void Remote::sendShowWindow()
{
	// Command '0' means show window
	m_socket->write("0\n");
	m_socket->waitForBytesWritten();
}

void Remote::showWindow()
{
	QTorrent::instance()->mainWindow()->show();
}
