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

#define MONITOR_INTERVAL 1000

TrafficMonitor::TrafficMonitor(QObject *parent)
	: QObject(parent)
	, m_uploadSpeed(0)
	, m_downloadSpeed(0)
	, m_bytesUploaded(0)
	, m_bytesDownloaded(0)
{
	m_timer.start(MONITOR_INTERVAL);
	connect(&m_timer, &QTimer::timeout, this, &TrafficMonitor::update);
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
	connect(peer, &Peer::uploadedData, this, &TrafficMonitor::onDataSent);
	connect(peer, &Peer::downloadedData, this, &TrafficMonitor::onDataReceived);
}

void TrafficMonitor::removePeer(Peer *peer)
{
	disconnect(peer, &Peer::uploadedData, this, &TrafficMonitor::onDataSent);
	disconnect(peer, &Peer::downloadedData, this, &TrafficMonitor::onDataReceived);
}

void TrafficMonitor::onDataSent(qint64 bytes)
{
	m_bytesUploaded += bytes;
}

void TrafficMonitor::onDataReceived(qint64 bytes)
{
	m_bytesDownloaded += bytes;
}

void TrafficMonitor::update()
{
	m_uploadSpeed = m_bytesUploaded / (MONITOR_INTERVAL / 1000.0);
	m_downloadSpeed = m_bytesDownloaded / (MONITOR_INTERVAL / 1000.0);
	m_bytesUploaded = 0;
	m_bytesDownloaded = 0;
	emit uploadSpeedChanged(m_uploadSpeed);
	emit downloadSpeedChanged(m_downloadSpeed);
}
