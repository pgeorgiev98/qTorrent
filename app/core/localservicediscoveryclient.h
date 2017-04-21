/* qTorrent - An open-source, cross-platform BitTorrent client
 * Copyright (C) 2017 Petko Georgiev
 *
 * localservicediscoveryclient.h
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

#ifndef LOCALSERVICEDISCOVERY_H
#define LOCALSERVICEDISCOVERY_H

#define LSD_ADDRESS_IPV4 "239.192.152.143"
#define LSD_PORT_IPV4 6771
#define LSD_ADDRESS_IPV6 "ff15::efc0:988f"
#define LSD_PORT_IPV6 6771
#define LSD_INTERVAL 5*60*1000 // milliseconds
#define LSD_MIN_INTERVAL 60*1000 // milliseconds

#include <QObject>
#include <QElapsedTimer>
#include <QHostAddress>
#include <QByteArray>

class Torrent;
class QTimer;
class QUdpSocket;

class LocalServiceDiscoveryClient : public QObject
{
	Q_OBJECT

public:
	LocalServiceDiscoveryClient(QObject *parent = nullptr);

public slots:
	// Normal announce for both IPv4 and IPv6
	void announceAll();
	// Announce for IPv4
	void announceIPv4();
	// Announce for IPv6
	void announceIPv6();
	// Called when a datagram is received
	void processPendingDatagrams();

signals:
	void foundPeer(QHostAddress address, int port, Torrent *torrent);

private:
	QTimer *m_announceTimer;
	QElapsedTimer m_elapsedTimer;
	QUdpSocket *m_socketIPv4;
	QUdpSocket *m_socketIPv6;
	QByteArray m_cookie;

	void announce(QUdpSocket *socket, const char *address, int port);
};

#endif // LOCALSERVICEDISCOVERY_H
