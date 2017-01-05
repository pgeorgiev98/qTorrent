#ifndef QTORRENT_H
#define QTORRENT_H

#include <QList>
#include <QString>

class Torrent;
class TorrentServer;

class QTorrent {
public:
	QTorrent();
	~QTorrent();

	bool startServer();
	bool addTorrent(const QString& filename);

	QList<Torrent*>& torrents();
	TorrentServer* server();

private:
	QList<Torrent*> m_torrents;
	TorrentServer* m_server;
};

#endif // QTORRENT_H
