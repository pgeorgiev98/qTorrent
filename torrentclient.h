#ifndef TORRENTCLIENT_H
#define TORRENTCLIENT_H

#include "torrentinfo.h"
#include <QTcpSocket>
#include <QObject>

class Peer;
class QTcpSocket;

class TorrentClient : QObject {
	Q_OBJECT

	QTcpSocket* m_socket;
	Peer* m_peer;
public slots:
	void connected();
	void readyRead();
	void finished();
public:
	TorrentClient(Peer* peer);
	~TorrentClient();
	void connectToPeer();
	//void handshake(TorrentInfo& torrentInfo);
};

#endif // TORRENTCLIENT_H
