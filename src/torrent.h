#ifndef TORRENT_H
#define TORRENT_H

#include <QString>
#include <QList>

class QTorrent;
class Peer;
class TorrentInfo;
class TrackerClient;
class Piece;
class Block;
class QFile;

/*
 * This class represents a torrent
 * It contains information about the state of the download/upload,
 * links to the files, peers, tracker client, pieces, etc.
 * Can be used to control the uploading/downloading of this torrent
 */
class Torrent {
public:
	/* Constructor and destructor */
	Torrent(QTorrent* qTorrent);
	~Torrent();

	/* Operations */

	bool createFromFile(const QString& filename);
	bool createFileTree(const QString& directory);

	Peer* addPeer(const QByteArray& address, int port);

	Block* requestBlock(Peer* client, int size);
	void releaseBlock(Peer* client, Block* block);

	bool savePiece(int pieceNumber);

	/* Getters */

	QTorrent* qTorrent();
	QList<Peer*>& peers();
	TorrentInfo* torrentInfo();
	TrackerClient* trackerClient();

	qint64 bytesDownloaded();
	qint64 bytesUploaded();
	int downloadedPieces();
	bool downloaded();

	/* Calculates the current percentage of the downloaded pieces */
	float percentDownloaded();
	/* Returns this torrent's current bitfield */
	QVector<bool> bitfield();


	/* Signals */

	/* Emitted when a piece is successfully downloaded */
	void downloadedPiece(Piece* piece);

	/* Emitted when a piece is successfully uploaded */
	void uploadedPiece(Piece* piece);

	/* Emitted when torrent is fully downloaded */
	void fullyDownloaded();

private:
	QTorrent* m_qTorrent;
	QList<Peer*> m_peers;
	QList<Piece*> m_pieces;
	TorrentInfo* m_torrentInfo;
	TrackerClient* m_trackerClient;
	QList<QFile*> m_files;

	qint64 m_bytesDownloaded;
	qint64 m_bytesUploaded;

	/* The total number of successfully downloaded pieces */
	int m_downloadedPieces;

	/* This flag is set when the torrent is completely downloaded */
	bool m_downloaded;
};

#endif // TORRENT_H
