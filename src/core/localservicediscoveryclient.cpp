#include "localservicediscoveryclient.h"
#include <QTimer>
#include <QUdpSocket>
#include <QString>
#include <QStringList>

#include "qtorrent.h"
#include "torrentserver.h"
#include "torrent.h"
#include "torrentinfo.h"

#include <QDebug>

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
	qDebug() << "LSD: Server started";
}

void LocalServiceDiscoveryClient::announce()
{
	QByteArray datagram = QString(
			"BT-SEARCH * HTTP/1.1\r\n"
			"Host: %1:%2\r\n"
			"Port: %3\r\n")
			.arg(LSD_ADDRESS)
			.arg(LSD_PORT)
			.arg(QTorrent::instance()->server()->port()).toLatin1();

	for (Torrent *torrent : QTorrent::instance()->torrents()) {
		QByteArray hash = torrent->torrentInfo()->infoHash().toHex().toLower();
		datagram.append("Infohash: " + hash + "\r\n");
	}

	datagram.append("cookie: " + m_cookie + "\r\n");
	datagram.append("\r\n\r\n");

	m_socket->writeDatagram(datagram, QHostAddress(LSD_ADDRESS), LSD_PORT);
	qDebug() << "LSD: Sent datagram" << endl << datagram;
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

		int port;
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
