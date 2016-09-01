#ifndef TORRENTCLIENT_H
#define TORRENTCLIENT_H

#include "torrentinfo.h"
#include <QTcpSocket>

class TorrentClient {
	QTcpSocket m_socket;
public slots:
	void readyRead();
	void connectionFinished();
public:
	TorrentClient();
	~TorrentClient();
	void handshake(TorrentInfo& torrentInfo);
};

#endif // TORRENTCLIENT_H
