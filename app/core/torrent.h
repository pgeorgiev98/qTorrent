/* qTorrent - An open-source, cross-platform BitTorrent client
 * Copyright (C) 2017 Petko Georgiev
 *
 * torrent.h
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef TORRENT_H
#define TORRENT_H

#include "resumeinfo.h"
#include "trackerclient.h"
#include <QHostAddress>
#include <QString>
#include <QList>
#include <QUrl>

class Peer;
class TorrentInfo;
class TrackerClient;
class FileController;
class Piece;
class Block;
class QFile;

/*
 * This class represents a torrent
 * It contains information about the state of the download/upload,
 * links to the files, peers, tracker client, pieces, etc.
 * Can be used to control the uploading/downloading of this torrent
 */
class Torrent : public QObject
{
	Q_OBJECT

public:
	/*
	 * Torrent state:
	 * New - Just created
	 * Loading - Loading torrent / Fetching torrent metainfo
	 * Checking - Verifying downloaded pieces
	 * Stopped - Downloading/Uploading is stopped.
	 * Started - Downloading/Uploading is allowed.
	 */
	enum State {
		New, Loading, Checking, Stopped, Started
	};

	/* Constructor and destructor */
	Torrent();
	~Torrent();

	/* Operations */

	bool createNew(TorrentInfo *torrentInfo, const QString &downloadLocation);
	bool createFromResumeInfo(TorrentInfo *torrentInfo, ResumeInfo *resumeInfo);
	bool createFromMagnetLink(QUrl url);
	void loadFileDescriptors();

	Block *requestBlock(Peer *client, int size);

	bool savePiece(Piece *piece);

	/* Getters */

	QList<Peer *> &peers();
	QList<Piece *> &pieces();
	QList<QFile *> &files();
	TorrentInfo *torrentInfo();
	TrackerClient *trackerClient();

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
	bool isDownloaded();
	bool isPaused() const;
	bool isStarted() const;
	int connectedPeersCount() const;
	int allPeersCount() const;

	// Return the torrent's state
	State state() const;
	// Returns the torrent's state as a formatted string
	QString stateString() const;

	const QString &downloadLocation() const;

	// Calculates the current percentage of the downloaded pieces
	float percentDownloaded();
	// Returns this torrent's current bitfield
	QVector<bool> bitfield() const;

	ResumeInfo getResumeInfo() const;

	// Returns m_torrentInfo->errorString();
	QString errorString() const;

signals:
	void checkingStarted();
	void checked();
	void fullyDownloaded();

public slots:
	// Called when torrent is checked
	void onChecked();

	// Called when a piece is successfully downloaded
	void onPieceDownloaded(Piece *piece);

	// Called when a block is successfully uploaded
	void onBlockUploaded(int bytes);

	// Called when torrent is fully downloaded. If state is 'Started',
	// then it will send a 'completed' announce to the tracker
	void onFullyDownloaded();

	// Called when announce is successful
	void onSuccessfullyAnnounced(TrackerClient::Event event);


	// Creates a peer and connects to him
	Peer *connectToPeer(QHostAddress address, int port);
	// Add a peer that has connected to us to the list
	void addPeer(Peer *peer);
	// Sets a piece's downloaded/available state.
	// if state is Started, it will increment m_bytesDownloaded
	void setPieceAvailable(Piece *piece, bool available);

	// Start downloading/uploading
	void start();
	// Pause the torrent
	void pause();
	// Stop the torrent
	void stop();
	// Check the torrent
	void check();

private:
	State m_state;
	QList<Peer *> m_peers;
	QList<Piece *> m_pieces;
	TorrentInfo *m_torrentInfo;
	TrackerClient *m_trackerClient;
	QList<QFile *> m_files;
	FileController *m_fileController;

	// The number of bytes on startup
	qint64 m_bytesDownloadedOnStartup;
	qint64 m_bytesUploadedOnStartup;
	// The number of bytes since the torrent was added
	qint64 m_totalBytesDownloaded;
	qint64 m_totalBytesUploaded;

	/* The total number of successfully downloaded pieces */
	int m_downloadedPieces;

	/* This flag is set when the torrent is completely downloaded */
	bool m_isDownloaded;

	/* Is the downloading/uploading paused? */
	bool m_isPaused;

	/* Start torrent after checking? */
	bool m_startAfterChecking;

	/* The torrent's download location */
	QString m_downloadLocation;

	/* Contains last error */
	QString m_errorString;
	void clearError();
	void setError(const QString &errorString);
};

#endif // TORRENT_H
