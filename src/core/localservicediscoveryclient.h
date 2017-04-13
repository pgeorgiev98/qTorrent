#ifndef LOCALSERVICEDISCOVERY_H
#define LOCALSERVICEDISCOVERY_H

#define LSD_ADDRESS "239.192.152.143"
#define LSD_PORT 6771

#include <QObject>
#include <QByteArray>
#include <QHostAddress>

class Torrent;
class QTimer;
class QUdpSocket;

class LocalServiceDiscoveryClient : public QObject
{
	Q_OBJECT

public:
	LocalServiceDiscoveryClient(QObject *parent = nullptr);

public slots:
	void announce();
	void processPendingDatagrams();

signals:
	void foundPeer(QHostAddress address, int port, Torrent *torrent);

private:
	QTimer *m_announceTimer;
	QUdpSocket *m_socket;
	QByteArray m_cookie;
};

#endif // LOCALSERVICEDISCOVERY_H
