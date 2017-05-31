/* qTorrent - An open-source, cross-platform BitTorrent client
 * Copyright (C) 2017 Petko Georgiev
 *
 * rctcpsocket.cpp
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

#include "rctcpsocket.h"
#include <QHostAddress>

RcTcpSocket::RcTcpSocket(QObject *parent)
	: QTcpSocket(parent)
{
	m_socket = new QTcpSocket(this);
	connectAll();
}

RcTcpSocket::RcTcpSocket(QTcpSocket *socket, QObject *parent)
	: QTcpSocket(parent)
{
	setOpenMode(socket->openMode());
	m_socket = socket;
	socketStateChanged(m_socket->state());
	connectAll();
}

void RcTcpSocket::connectAll()
{
	connect(this, SIGNAL(readyRead()), this, SIGNAL(readyToTransfer()));
	connect(this, SIGNAL(connected()), this, SIGNAL(readyToTransfer()));

	connect(m_socket, SIGNAL(connected()),
			this, SIGNAL(connected()));
	connect(m_socket, SIGNAL(readyRead()),
			this, SIGNAL(readyRead()));
	connect(m_socket, SIGNAL(disconnected()),
			this, SIGNAL(disconnected()));
	connect(m_socket, SIGNAL(error(QAbstractSocket::SocketError)),
			this, SIGNAL(error(QAbstractSocket::SocketError)));
	connect(m_socket, SIGNAL(bytesWritten(qint64)),
			this, SIGNAL(bytesWritten(qint64)));
	connect(m_socket, SIGNAL(stateChanged(QAbstractSocket::SocketState)),
			this, SLOT(socketStateChanged(QAbstractSocket::SocketState)));
}


qint64 RcTcpSocket::writeToNetwork(qint64 maxLen)
{
	qint64 bytesWritten =
			m_socket->write(m_outgoing.data(), qMin(maxLen, qint64(m_outgoing.size())));
	if (bytesWritten <= 0)
		return bytesWritten;
	m_outgoing.remove(0, bytesWritten);
	return bytesWritten;
}

qint64 RcTcpSocket::readFromNetwork(qint64 maxLen)
{
	int oldSize = m_incoming.size();
	m_incoming.resize(oldSize + maxLen);
	qint64 bytesRead = m_socket->read(m_incoming.data() + oldSize, maxLen);
	m_incoming.resize(bytesRead <= 0 ? oldSize : oldSize + bytesRead);
	if (bytesRead > 0)
		emit readyRead();
	return bytesRead;
}

void RcTcpSocket::setReadBufferSize(qint64 size)
{
	m_socket->setReadBufferSize(size);
}

bool RcTcpSocket::canTransferMore() const
{
	return bytesAvailable() > 0 || m_socket->bytesAvailable() > 0
			|| !m_outgoing.isEmpty();
}


void RcTcpSocket::connectToHost(const QHostAddress &address, quint16 port, OpenMode mode)
{
	setOpenMode(mode);
	m_socket->connectToHost(address, port, mode);
}

void RcTcpSocket::connectToHost(const QString &hostName, quint16 port, OpenMode mode, NetworkLayerProtocol protocol)
{
	setOpenMode(mode);
	m_socket->connectToHost(hostName, port, mode, protocol);
}

void RcTcpSocket::disconnectFromHost()
{
	m_socket->disconnectFromHost();
}


void RcTcpSocket::socketStateChanged(QAbstractSocket::SocketState state)
{
	setLocalAddress(m_socket->localAddress());
	setLocalPort(m_socket->localPort());
	setPeerName(m_socket->peerName());
	setPeerAddress(m_socket->peerAddress());
	setPeerPort(m_socket->peerPort());
	setSocketState(state);
}

qint64 RcTcpSocket::readData(char *data, qint64 maxLen)
{
	int bytesRead = qMin<int>(maxLen, m_incoming.size());
	if (bytesRead > 0) {
		memcpy(data, m_incoming.constData(), bytesRead);
		m_incoming.remove(0, bytesRead);
	}

	if (state() != ConnectedState) {
		QByteArray buffer;
		buffer.resize(m_socket->bytesAvailable());
		m_socket->read(buffer.data(), buffer.size());
		m_incoming += buffer;
	}
	return qint64(bytesRead);
}

qint64 RcTcpSocket::readLineData(char *data, qint64 maxLen)
{
	return QIODevice::readLineData(data, maxLen);
}

qint64 RcTcpSocket::writeData(const char *data, qint64 len)
{
	int oldSize = m_outgoing.size();
	m_outgoing.resize(oldSize + len);
	memcpy(m_outgoing.data() + oldSize, data, len);
	emit readyToTransfer();
	return len;
}
