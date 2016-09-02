#ifndef PEER_H
#define PEER_H

#include <QByteArray>
#include <QObject>

class Torrent;
class TorrentClient;

class Peer : public QObject {
	Q_OBJECT

	Torrent* m_torrent;
	QByteArray m_address;
	int m_port;
	TorrentClient* m_torrentClient;
public:
	Peer(Torrent* torrent, const QByteArray& address, int port);
	~Peer();
	Torrent* torrent();
	QByteArray& address();
	int port();
	void startConnection();
};

#endif // PEER_H
