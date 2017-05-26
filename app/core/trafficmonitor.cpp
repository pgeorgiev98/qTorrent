/* qTorrent - An open-source, cross-platform BitTorrent client
 * Copyright (C) 2017 Petko Georgiev
 *
 * trafficmonitor.cpp
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

#include "trafficmonitor.h"
#include "peer.h"

#define MONITOR_HISTORY_LENGTH 1000

TrafficMonitor::TrafficMonitor(QObject *parent)
	: QObject(parent)
	, m_uploadSpeed(0)
	, m_downloadSpeed(0)
{

}

qint64 TrafficMonitor::uploadSpeed() const
{
	return m_uploadSpeed;
}

qint64 TrafficMonitor::downloadSpeed() const
{
	return m_downloadSpeed;
}

void TrafficMonitor::addPeer(Peer *peer)
{
	m_peers.insert(peer);
	connect(peer, &Peer::uploadedData, this, &TrafficMonitor::onDataSent);
	connect(peer, &Peer::downloadedData, this, &TrafficMonitor::onDataReceived);
}

void TrafficMonitor::removePeer(Peer *peer)
{
	m_peers.remove(peer);
	disconnect(peer, &Peer::uploadedData, this, &TrafficMonitor::onDataSent);
	disconnect(peer, &Peer::downloadedData, this, &TrafficMonitor::onDataReceived);
}

void TrafficMonitor::onDataSent(qint64 bytes)
{
	QTimer *t = new QTimer(this);
	m_uploads[t] = bytes;
	connect(t, &QTimer::timeout, this, &TrafficMonitor::removeUpload);
	t->start(MONITOR_HISTORY_LENGTH);
	updateUploadSpeed();
}

void TrafficMonitor::onDataReceived(qint64 bytes)
{
	QTimer *t = new QTimer(this);
	m_downloads[t] = bytes;
	connect(t, &QTimer::timeout, this, &TrafficMonitor::removeDownload);
	t->start(MONITOR_HISTORY_LENGTH);
	updateDownloadSpeed();
}

void TrafficMonitor::removeUpload()
{
	QTimer *t = qobject_cast<QTimer *>(sender());
	if (t == nullptr)
		return;
	m_uploads.remove(t);
	updateUploadSpeed();
}

void TrafficMonitor::removeDownload()
{
	QTimer *t = qobject_cast<QTimer *>(sender());
	if (t == nullptr)
		return;
	m_downloads.remove(t);
	updateDownloadSpeed();
}

void TrafficMonitor::updateUploadSpeed()
{
	qint64 bytes = 0;
	for (qint64 b : m_uploads.values())
		bytes += b;
	m_uploadSpeed = bytes / (MONITOR_HISTORY_LENGTH / 1000.0);
	emit uploadSpeedChanged(m_uploadSpeed);
}

void TrafficMonitor::updateDownloadSpeed()
{
	qint64 bytes = 0;
	for (qint64 b : m_downloads.values())
		bytes += b;
	m_downloadSpeed = bytes / (MONITOR_HISTORY_LENGTH / 1000.0);
	emit downloadSpeedChanged(m_downloadSpeed);
}
