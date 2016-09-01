#ifndef TORRENT_H
#define TORRENT_H

#include <QString>
#include <QList>

class QTorrent;
class Peer;
class TorrentInfo;
class TrackerClient;
class TorrentClient;

class Torrent {
public:
	Torrent(QTorrent* qTorrent);
	~Torrent();
	bool createFromFile(const QString& filename);
private:
	QTorrent* m_qTorrent;
	QList<Peer*> m_peers;
	TorrentInfo* m_torrentInfo;
	TrackerClient* m_trackerClient;
	TorrentClient* m_torrentClient;
};

#endif // TORRENT_H
