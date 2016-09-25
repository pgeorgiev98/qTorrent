#ifndef TORRENT_H
#define TORRENT_H

#include <QString>
#include <QList>
#include <QMutex>

class QTorrent;
class Peer;
class TorrentInfo;
class TrackerClient;
class TorrentClient;
class Piece;
class Block;

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
	Block* requestBlock(TorrentClient* client, int size);
private:
	QTorrent* m_qTorrent;
	QList<Peer*> m_peers;
	QList<Piece*> m_pieces;
	TorrentInfo* m_torrentInfo;
	TrackerClient* m_trackerClient;
	QMutex m_requestBlockMutex;
};

#endif // TORRENT_H
