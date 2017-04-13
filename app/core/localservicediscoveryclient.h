#ifndef LOCALSERVICEDISCOVERY_H
#define LOCALSERVICEDISCOVERY_H

#define LSD_ADDRESS "239.192.152.143"
#define LSD_PORT 6771
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
	void announce();
	void processPendingDatagrams();

signals:
	void foundPeer(QHostAddress address, int port, Torrent *torrent);

private:
	QTimer *m_announceTimer;
	QElapsedTimer m_elapsedTimer;
	QUdpSocket *m_socket;
	QByteArray m_cookie;
};

#endif // LOCALSERVICEDISCOVERY_H
