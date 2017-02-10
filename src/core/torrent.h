#ifndef TORRENT_H
#define TORRENT_H

#include "resumeinfo.h"
#include "trackerclient.h"
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

	bool createNew(TorrentInfo* torrentInfo, const QString& downloadLocation);
	bool createFromResumeInfo(TorrentInfo* torrentInfo, ResumeInfo* resumeInfo);
	bool createFromMagnetLink(QUrl url);
	bool createFileTree(const QString& directory);

	ResumeInfo getResumeInfo() const;

	// Start downloading/uploading
	void start();
	// Pause the torrent
	void pause();

	/* Creates a peer and connects to him */
	Peer* addPeer(const QByteArray& address, int port);
	/* Add a peer that has connected to us to the list */
	void addPeer(Peer* peer);

	Block* requestBlock(Peer* client, int size);

	bool savePiece(int pieceNumber);

	/* Sets a piece's downloaded/available status.
	 * Does not increment m_bytesDownloaded
	 * if torrent is currently being added, announce must be true */
	void setPieceAvailable(Piece* piece, bool announce);

	void successfullyAnnounced(TrackerClient::Event event);

	/* Getters */

	QTorrent* qTorrent();
	QList<Peer*>& peers();
	QList<Piece*>& pieces();
	TorrentInfo* torrentInfo();
	TrackerClient* trackerClient();
	QList<QFile*>& files();

	// The number of bytes since startup
	qint64 bytesDownloaded() const;
	qint64 bytesUploaded() const;
	// The number of bytes since the torrent was added
	qint64 totalBytesDownloaded() const;
	qint64 totalBytesUploaded() const;
	// The number of bytes we have
	qint64 bytesAvailable() const;
	// The number of bytes we need to have the full torrent
	qint64 bytesLeft() const;

	int downloadedPieces();
	bool downloaded();
	bool isPaused() const;
	bool hasAnnouncedStarted() const;
	int connectedPeersCount() const;
	int allPeersCount() const;

	const QString& downloadLocation() const;

	/* Calculates the current percentage of the downloaded pieces */
	float percentDownloaded();
	/* Returns this torrent's current bitfield */
	QVector<bool> bitfield() const;

	// Returns m_torrentInfo->errorString();
	QString errorString() const;


	/* Signals */

	/* Called when a piece is successfully downloaded */
	void downloadedPiece(Piece* piece);

	/* Called when a piece is successfully uploaded */
	void uploadedBlock(int bytes);

	/* Called when torrent is fully downloaded if announce == true,
	 * then it will send a 'completed' announce to the tracker
	 * Must be set to false when the torrent is still being added */
	void fullyDownloaded(bool announce = true);

private:
	QTorrent* m_qTorrent;
	Status m_status;
	QList<Peer*> m_peers;
	QList<Piece*> m_pieces;
	TorrentInfo* m_torrentInfo;
	TrackerClient* m_trackerClient;
	QList<QFile*> m_files;

	// The number of bytes on startup
	qint64 m_bytesDownloadedOnStartup;
	qint64 m_bytesUploadedOnStartup;
	// The number of bytes since the torrent was added
	qint64 m_totalBytesDownloaded;
	qint64 m_totalBytesUploaded;

	/* The total number of successfully downloaded pieces */
	int m_downloadedPieces;

	/* This flag is set when the torrent is completely downloaded */
	bool m_downloaded;

	/* Is the downloading/uploading paused? */
	bool m_paused;

	/* Have we sent a 'Started' announce to the tracker */
	bool m_hasAnnouncedStarted;

	/* The torrent's download location */
	QString m_downloadLocation;

	/* Contains last error */
	QString m_errorString;
	void clearError();
	void setError(const QString& errorString);
};

#endif // TORRENT_H
