/* qTorrent - An open-source, cross-platform BitTorrent client
 * Copyright (C) 2017 Petko Georgiev
 *
 * localservicediscoveryclient.cpp
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

#include "localservicediscoveryclient.h"
#include <QTimer>
#include <QUdpSocket>
#include <QString>
#include <QStringList>

#include "qtorrent.h"
#include "torrentserver.h"
#include "torrent.h"
#include "torrentinfo.h"

LocalServiceDiscoveryClient::LocalServiceDiscoveryClient(QObject *parent)
	: QObject(parent)
{
	m_announceTimer = new QTimer(this);
	m_socketIPv4 = new QUdpSocket(this);
	m_socketIPv4->bind(QHostAddress::AnyIPv4, LSD_PORT_IPV4, QUdpSocket::ShareAddress);
	m_socketIPv4->joinMulticastGroup(QHostAddress(LSD_ADDRESS_IPV4));
	m_socketIPv4->setSocketOption(QAbstractSocket::MulticastTtlOption, 20);

	m_socketIPv6 = new QUdpSocket(this);
	m_socketIPv6->bind(QHostAddress::AnyIPv6, LSD_PORT_IPV6, QUdpSocket::ShareAddress);
	m_socketIPv6->joinMulticastGroup(QHostAddress(LSD_ADDRESS_IPV6));
	m_socketIPv6->setSocketOption(QAbstractSocket::MulticastTtlOption, 20);
	// Generate 20-byte cookie uing symbols 0-9, A-Z and a-z
	for (int i = 0; i < 20; i++) {
		int q = qrand() % (10+26+26);
		if (q < 10) {
			m_cookie.append('0' + q);
		} else if (q < 36) {
			m_cookie.append('A' + q - 10);
		} else {
			m_cookie.append('a' + q - 36);
		}
	}

	connect(m_announceTimer, &QTimer::timeout, this, &LocalServiceDiscoveryClient::announceAll);
	connect(m_socketIPv4, &QUdpSocket::readyRead, this, &LocalServiceDiscoveryClient::processPendingDatagrams);
	connect(m_socketIPv6, &QUdpSocket::readyRead, this, &LocalServiceDiscoveryClient::processPendingDatagrams);
}

void LocalServiceDiscoveryClient::announce(QUdpSocket *socket, const char *address, int port)
{
	QHostAddress addr(address);
	QString addressString = address;
	if (addr.protocol() == QAbstractSocket::IPv6Protocol) {
		addressString.prepend('[');
		addressString.append(']');
	}
	QString datagramString;
	QTextStream datagramStream(&datagramString);
	datagramStream << "BT-SEARCH * HTTP/1.1\r\n"
				   << "Host: " << addressString << ":" << port << "\r\n"
				   << "Port: " << QTorrent::instance()->server()->port() << "\r\n";

	for (Torrent *torrent : QTorrent::instance()->torrents()) {
		QByteArray hash = torrent->torrentInfo()->infoHash().toHex().toLower();
		datagramStream << "Infohash: " << hash << "\r\n";
	}

	datagramStream << "cookie: " << m_cookie << "\r\n";
	datagramStream << "\r\n\r\n";
	socket->writeDatagram(datagramString.toLatin1(), addr, port);

	m_announceTimer->start(LSD_INTERVAL);
	m_elapsedTimer.start();
}

void LocalServiceDiscoveryClient::announceAll()
{
	// Don't announce more than once every LSD_MIN_INTERVAL milliseconds
	int elapsed = m_elapsedTimer.elapsed();
	if (elapsed < LSD_MIN_INTERVAL && m_announceTimer->isActive()) {
		m_announceTimer->start(LSD_MIN_INTERVAL - elapsed);
		return;
	}

	// Don't send empty announces
	if (QTorrent::instance()->torrents().isEmpty()) {
		return;
	}

	announceIPv4();
	announceIPv6();
}

void LocalServiceDiscoveryClient::announceIPv4()
{
	announce(m_socketIPv4, LSD_ADDRESS_IPV4, LSD_PORT_IPV4);
}

void LocalServiceDiscoveryClient::announceIPv6()
{
	announce(m_socketIPv6, LSD_ADDRESS_IPV6, LSD_PORT_IPV6);
}

void LocalServiceDiscoveryClient::processPendingDatagrams()
{
	QList<QByteArray> infoHashes;
	for (Torrent *torrent : QTorrent::instance()->torrents()) {
		infoHashes.append(torrent->torrentInfo()->infoHash().toHex().toLower());
	}

	for (;;) {
		QUdpSocket *senderSocket = nullptr;
		if (m_socketIPv4->hasPendingDatagrams()) {
			senderSocket = m_socketIPv4;
		} else if (m_socketIPv6->hasPendingDatagrams()) {
			senderSocket = m_socketIPv6;
		} else {
			break;
		}

		QHostAddress sender;
		QByteArray datagram;
		datagram.resize(senderSocket->pendingDatagramSize());
		senderSocket->readDatagram(datagram.data(), datagram.size(), &sender);

		int port = 0;
		QList<QByteArray> receivedInfoHashes;
		QByteArray cookie;

		QTextStream stream(datagram);
		QString line;
		stream.readLineInto(&line);
		if (line != "BT-SEARCH * HTTP/1.1") {
			continue;
		}
		while (stream.readLineInto(&line)) {
			QStringList splitLine = line.split(": ");
			if (splitLine.size() == 2) {
				QString header = splitLine.first();
				if (header == "Host") {
					// Ignore
				} else if (header == "Port") {
					bool ok;
					port = splitLine.last().toInt(&ok);
					if(!ok) {
						port = 0;
					}
				} else if (header == "Infohash") {
					receivedInfoHashes.append(splitLine.last().toLatin1().toLower());
				} else if (header == "cookie") {
					cookie = splitLine.last().toLatin1();
				}
			}
		}

		if (port == 0 || cookie == m_cookie) {
			continue;
		}

		for (QByteArray& hash : receivedInfoHashes) {
			if (infoHashes.contains(hash)) {
				for (Torrent *torrent : QTorrent::instance()->torrents()) {
					if (torrent->torrentInfo()->infoHash().toHex().toLower() == hash) {
						emit foundPeer(sender, port, torrent);
					}
				}
			}
		}
	}
}
