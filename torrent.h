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
class QFile;

class Torrent {
public:
	Torrent(QTorrent* qTorrent);
	~Torrent();
	bool createFromFile(const QString& filename);
	bool createFileTree(const QString& directory);
	void addPeer(Peer* peer);
	QTorrent* qTorrent();
	QList<Peer*>& peers();
	TorrentInfo* torrentInfo();
	TrackerClient* trackerClient();
	Block* requestBlock(TorrentClient* client, int size);
	bool savePiece(int pieceNumber);
	int downloadedPieces();

	void deleteBlock(Block* block);
	int blockPieceNumber(Block* block);
	int blockBeginIndex(Block* block);
	int blockSize(Block* block);
	void blockSetData(Block* block, const char* data, int length);
private:
	QTorrent* m_qTorrent;
	QList<Peer*> m_peers;
	QList<Piece*> m_pieces;
	TorrentInfo* m_torrentInfo;
	TrackerClient* m_trackerClient;
	QList<QFile*> m_files;
	QMutex m_accessMutex;
};

#endif // TORRENT_H
