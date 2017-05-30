/* qTorrent - An open-source, cross-platform BitTorrent client
 * Copyright (C) 2017 Petko Georgiev
 *
 * rctcpsocket.h
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

#ifndef RCTCPSOCKET_H
#define RCTCPSOCKET_H

#include <QTcpSocket>
#include <QByteArray>

class RcTcpSocket : public QTcpSocket
{
	Q_OBJECT

public:
	RcTcpSocket(QObject *parent = nullptr);

	inline bool canReadLine() const override { return m_incoming.contains('\n'); }

	qint64 writeToNetwork(qint64 maxLen);
	qint64 readFromNetwork(qint64 maxLen);

	bool canTransferMore() const;
	inline qint64 bytesAvailable() const override { return m_incoming.size(); }
	inline qint64 networkBytesAvailable() const { return m_socket.bytesAvailable(); }
	inline qint64 networkBytesToWrite() const { return m_socket.bytesToWrite(); }

	void setReadBufferSize(qint64 size) override;

	void connectToHost(const QHostAddress &address, quint16 port, OpenMode mode = ReadWrite) override;
	void connectToHost(const QString &hostName, quint16 port,
					   OpenMode mode = ReadWrite, NetworkLayerProtocol protocol = AnyIPProtocol) override;

signals:
	void readyToTransfer();

public slots:
	void socketStateChanged(QAbstractSocket::SocketState state);

protected:
	qint64 readData(char *data, qint64 maxLen) override;
	qint64 readLineData(char *data, qint64 maxLen) override;
	qint64 writeData(const char *data, qint64 len) override;

private:
	QByteArray m_outgoing;
	QByteArray m_incoming;

	QTcpSocket m_socket;
};


#endif // RCTCPSOCKET_H
