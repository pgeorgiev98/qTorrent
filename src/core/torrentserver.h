#ifndef TORRENTSERVER_H
#define TORRENTSERVER_H

#include <QObject>
#include <QTcpServer>
#include <QList>

class Peer;

/* This class is used to receive and handle incoming peer connections */
class TorrentServer : public QObject {
	Q_OBJECT

public:
	TorrentServer();
	~TorrentServer();

	/* Start server on port 'port'. If port is 0, then
	 * server will automatically choose port */
	bool startServer(int port = 0);

	QTcpServer& server();
	int port();
	QHostAddress address();
	QList<Peer*>& peers();

public slots:
	void newConnection();

private:
	QTcpServer m_server;
	QList<Peer*> m_peers;
};

#endif // TORRENTSERVER_H
