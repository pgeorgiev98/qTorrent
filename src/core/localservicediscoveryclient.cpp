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
	m_socket = new QUdpSocket(this);
	m_socket->bind(QHostAddress::AnyIPv4, LSD_PORT, QUdpSocket::ShareAddress);
	m_socket->joinMulticastGroup(QHostAddress(LSD_ADDRESS));
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

	connect(m_announceTimer, &QTimer::timeout, this, &LocalServiceDiscoveryClient::announce);
	connect(m_socket, &QUdpSocket::readyRead, this, &LocalServiceDiscoveryClient::processPendingDatagrams);
}

void LocalServiceDiscoveryClient::announce()
{
	// Don't announce more than once every LSD_MIN_INTERVAL milliseconds
	int elapsed = m_elapsedTimer.elapsed();
	if (elapsed < LSD_MIN_INTERVAL && m_announceTimer->isActive()) {
		m_announceTimer->start(LSD_MIN_INTERVAL - elapsed);
		return;
	}
	QString datagramString;
	QTextStream datagramStream(&datagramString);
	datagramStream << "BT-SEARCH * HTTP/1.1\r\n"
				   << "Host: " << LSD_ADDRESS << ":" << LSD_PORT << "\r\n"
				   << "Port: " << QTorrent::instance()->server()->port() << "\r\n";

	for (Torrent *torrent : QTorrent::instance()->torrents()) {
		QByteArray hash = torrent->torrentInfo()->infoHash().toHex().toLower();
		datagramStream << "Infohash: " << hash << "\r\n";
	}

	datagramStream << "cookie: " << m_cookie << "\r\n";
	datagramStream << "\r\n\r\n";

	m_socket->writeDatagram(datagramString.toLatin1(), QHostAddress(LSD_ADDRESS), LSD_PORT);
	m_announceTimer->start(LSD_INTERVAL);
	m_elapsedTimer.start();
}

void LocalServiceDiscoveryClient::processPendingDatagrams()
{
	QList<QByteArray> infoHashes;
	for (Torrent *torrent : QTorrent::instance()->torrents()) {
		infoHashes.append(torrent->torrentInfo()->infoHash().toHex().toLower());
	}

	while (m_socket->hasPendingDatagrams()) {
		QHostAddress sender;
		QByteArray datagram;
		datagram.resize(m_socket->pendingDatagramSize());
		m_socket->readDatagram(datagram.data(), datagram.size(), &sender);

		int port = 0;
		QList<QByteArray> receivedInfoHashes;

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
						continue;
					}
				} else if (header == "Infohash") {
					receivedInfoHashes.append(splitLine.last().toLatin1().toLower());
				} else if (header == "cookie") {
					if (splitLine.last() == m_cookie) {
						continue;
					}
				}
			}
		}

		if (port == 0) {
			return;
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
