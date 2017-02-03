#ifndef TORRENT_H
#define TORRENT_H

#include <QString>
#include <QList>
#include <QUrl>

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
	enum Status {
		New, Loading, Checking, Ready, Downloading, Completed
	};

	/* Constructor and destructor */
	Torrent(QTorrent* qTorrent);
	~Torrent();

	/* Operations */

	bool createFromFile(const QString& filename, const QString& downloadPath);
	bool createFromMagnetLink(QUrl url);
	bool createFileTree(const QString& directory);

	// Start downloading/uploading
	void start();
	// Pause the torrent
	void pause();

	/* Creates a peer and connects to him */
	Peer* addPeer(const QByteArray& address, int port);
	/* Add a peer that has connected to us to the list */
	void addPeer(Peer* peer);

	Block* requestBlock(Peer* client, int size);
	void releaseBlock(Peer* client, Block* block);

	bool savePiece(int pieceNumber);

	/* Getters */

	QTorrent* qTorrent();
	QList<Peer*>& peers();
	QList<Piece*>& pieces();
	TorrentInfo* torrentInfo();
	TrackerClient* trackerClient();
	QList<QFile*>& files();

	qint64 bytesDownloaded();
	qint64 bytesUploaded();
	int downloadedPieces();
	bool downloaded();

	bool isPaused() const;

	bool hasAnnouncedStarted() const;

	/* Calculates the current percentage of the downloaded pieces */
	float percentDownloaded();
	/* Returns this torrent's current bitfield */
	QVector<bool> bitfield();

	// Returns m_torrentInfo->errorString();
	QString errorString() const;


	/* Signals */

	/* Emitted when a piece is successfully downloaded */
	void downloadedPiece(Piece* piece);

	/* Emitted when a piece is successfully uploaded */
	void uploadedBlock(int bytes);

	/* Emitted when torrent is fully downloaded */
	void fullyDownloaded();

private:
	QTorrent* m_qTorrent;
	Status m_status;
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

	/* Is the downloading/uploading paused? */
	bool m_paused;

	/* Have we sent a 'Started' announce to the tracker */
	bool m_hasAnnouncedStarted;

	/* Contains last error */
	QString m_errorString;
	void clearError();
	void setError(const QString& errorString);
};

#endif // TORRENT_H
