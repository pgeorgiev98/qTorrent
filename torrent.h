#ifndef TORRENT_H
#define TORRENT_H

#include <QString>
#include <QList>

class QTorrent;
class Peer;
class TorrentInfo;
class TrackerClient;
class TorrentClient;
class Piece;

class Torrent {
public:
	Torrent(QTorrent* qTorrent);
	~Torrent();
	bool createFromFile(const QString& filename);
	void addPeer(Peer* peer);
	QTorrent* qTorrent();
	QList<Peer*>& peers();
	TorrentInfo* torrentInfo();
	TrackerClient* trackerClient();
private:
	QTorrent* m_qTorrent;
	QList<Peer*> m_peers;
    QList<Piece*> m_pieces;
    TorrentInfo* m_torrentInfo;
	TrackerClient* m_trackerClient;
};

#endif // TORRENT_H
