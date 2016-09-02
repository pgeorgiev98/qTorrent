#ifndef TORRENTCLIENT_H
#define TORRENTCLIENT_H

#include "torrentinfo.h"
#include <QByteArray>
#include <QObject>

class Peer;
class QTcpSocket;

class TorrentClient : QObject {
	Q_OBJECT

public:
	enum Status {
		Created, Connecting, Handshaking, ConnectionEstablished
	};

	TorrentClient(Peer* peer);
	~TorrentClient();
	void connectToPeer();
public slots:
	void connected();
	void readyRead();
	void finished();
private:
	QTcpSocket* m_socket;
	Peer* m_peer;
	QByteArray m_receivedData;
	Status m_status;
};

#endif // TORRENTCLIENT_H
