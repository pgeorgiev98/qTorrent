/* qTorrent - An open-source, cross-platform BitTorrent client
 * Copyright (C) 2017 Petko Georgiev
 *
 * trafficmonitor.h
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

#ifndef TRAFFICMONITOR_H
#define TRAFFICMONITOR_H

#include <QObject>
#include <QSet>
#include <QMap>
#include <QTimer>

class Peer;

class TrafficMonitor : public QObject
{
	Q_OBJECT

public:
	TrafficMonitor(QObject *parent = nullptr);

	qint64 uploadSpeed() const;
	qint64 downloadSpeed() const;

public slots:
	void addPeer(Peer *peer);
	void removePeer(Peer *peer);

	void onDataSent(qint64 bytes);
	void onDataReceived(qint64 bytes);

	void update();

signals:
	void uploadSpeedChanged(qint64 bytesPerSecond);
	void downloadSpeedChanged(qint64 bytesPerSecond);

private:
	qint64 m_uploadSpeed;
	qint64 m_downloadSpeed;
	QSet<Peer *> m_peers;
	QTimer m_timer;
	qint64 m_bytesUploaded;
	qint64 m_bytesDownloaded;
};

#endif // TRAFFICMONITOR_H
