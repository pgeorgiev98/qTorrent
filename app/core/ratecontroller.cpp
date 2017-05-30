/* qTorrent - An open-source, cross-platform BitTorrent client
 * Copyright (C) 2017 Petko Georgiev
 *
 * ratecontroller.cpp
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

#include "ratecontroller.h"
#include "rctcpsocket.h"
#include <QTimer>

Q_GLOBAL_STATIC(RateController, rateController)

RateController::RateController(QObject *parent)
	: QObject(parent)
	, m_upLimit(0)
	, m_downLimit(0)
	, m_transferScheduled(false)
{
}

RateController* RateController::instance()
{
	return rateController;
}

void RateController::addSocket(RcTcpSocket *socket)
{
	connect(socket, SIGNAL(readyToTransfer()), this, SLOT(transfer()));
	socket->setReadBufferSize(m_downLimit * 2);
	m_sockets.insert(socket);
	scheduleTransfer();
}

void RateController::removeSocket(RcTcpSocket *socket)
{
	disconnect(socket, SIGNAL(readyToTransfer()), this, SLOT(transfer()));
	socket->setReadBufferSize(0);
	m_sockets.remove(socket);
}

void RateController::setUploadLimit(int bytesPerSecond)
{
	m_upLimit = bytesPerSecond;
}

void RateController::setDownloadLimit(int bytesPerSecond)
{
	m_downLimit = bytesPerSecond;
	foreach (RcTcpSocket *socket, m_sockets)
		socket->setReadBufferSize(m_downLimit * 2);
}

void RateController::scheduleTransfer()
{
	if (m_transferScheduled)
		return;
	m_transferScheduled = true;
	QTimer::singleShot(50, this, SLOT(transfer()));
}

void RateController::transfer()
{
	m_transferScheduled = false;
	int msecs = 1000;
	if (!m_stopWatch.isNull())
		msecs = qMin(msecs, m_stopWatch.elapsed());
	qint64 bytesToWrite = (m_upLimit * msecs) / 1000;
	qint64 bytesToRead = (m_downLimit * msecs) / 1000;
	if (bytesToWrite == 0 && bytesToRead == 0) {
		scheduleTransfer();
		return;
	}
	QSet<RcTcpSocket *> pendingSockets;
	foreach (RcTcpSocket *client, m_sockets) {
		if (client->canTransferMore())
			pendingSockets.insert(client);
	}
	if (pendingSockets.isEmpty())
		return;
	m_stopWatch.start();

	bool canTransferMore;
	do {
		canTransferMore = false;
		qint64 writeChunk = qMax(qint64(1), bytesToWrite / pendingSockets.size());
		qint64 readChunk = qMax(qint64(1), bytesToRead / pendingSockets.size());

		QSetIterator<RcTcpSocket *> it(pendingSockets);
		while (it.hasNext() && (bytesToWrite > 0 || bytesToRead > 0)) {
			RcTcpSocket *socket = it.next();
			bool dataTransferred = false;
			qint64 available = qMin(readChunk,
									socket->networkBytesAvailable());
			if (available > 0) {
				qint64 readBytes = socket->readFromNetwork(
							qMin(available, bytesToRead));
				if (readBytes > 0) {
					bytesToRead -= readBytes;
					dataTransferred = true;
				}
			}
			if (m_upLimit * 2 > socket->bytesToWrite()) {
				qint64 chunkSize = qMin(writeChunk, bytesToWrite);
				qint64 toWrite = qMin(chunkSize,
									  m_upLimit * 2 - socket->bytesToWrite());
				if (toWrite > 0) {
					qint64 writtenBytes = socket->writeToNetwork(toWrite);
					if (writtenBytes > 0) {
						bytesToWrite -= writtenBytes;
						dataTransferred = true;
					}
				}
			}
			if (dataTransferred && socket->canTransferMore())
				canTransferMore = true;
			else
				pendingSockets.remove(socket);
		}
	} while (canTransferMore
			 && (bytesToWrite > 0 || bytesToRead > 0)
			 && !pendingSockets.isEmpty());

	if (canTransferMore)
		scheduleTransfer();
}
