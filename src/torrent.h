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
	void addPeer(const QByteArray& address, int port);
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

	qint64 bytesDownloaded();
	qint64 bytesUploaded();
	void addToBytesDownloaded(qint64 value);
	void addToBytesUploaded(qint64 value);
private:
	QTorrent* m_qTorrent;
	QList<Peer*> m_peers;
	QList<Piece*> m_pieces;
	TorrentInfo* m_torrentInfo;
	TrackerClient* m_trackerClient;
	QList<QFile*> m_files;
	QMutex m_accessMutex;

	qint64 m_bytesDownloaded;
	qint64 m_bytesUploaded;
};

#endif // TORRENT_H
