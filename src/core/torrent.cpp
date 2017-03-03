/* qTorrent - An open-source, cross-platform BitTorrent client
 * Copyright (C) 2017 Petko Georgiev
 *
 * torrent.cpp
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

#include "torrent.h"
#include "peer.h"
#include "torrentinfo.h"
#include "piece.h"
#include "block.h"
#include "torrentmessage.h"
#include "qtorrent.h"
#include "ui/mainwindow.h"
#include <QDir>
#include <QFile>
#include <QUrlQuery>

Torrent::Torrent()
	: m_state(New)
	, m_torrentInfo(nullptr)
	, m_trackerClient(nullptr)
	, m_bytesDownloadedOnStartup(0)
	, m_bytesUploadedOnStartup(0)
	, m_totalBytesDownloaded(0)
	, m_totalBytesUploaded(0)
	, m_downloadedPieces(0)
	, m_isDownloaded(false)
	, m_isPaused(true)
{
}

Torrent::~Torrent() {
	for(auto peer : m_peers) {
		delete peer;
	}

	for(Piece* piece : m_pieces) {
		delete piece;
	}

	if(m_torrentInfo) {
		delete m_torrentInfo;
	}

	if(m_trackerClient) {
		delete m_trackerClient;
	}

	for(QFile* file : m_files) {
		delete file;
	}
}

bool Torrent::createNew(TorrentInfo *torrentInfo, const QString &downloadLocation) {
	clearError();
	m_state = Loading;

	m_torrentInfo = torrentInfo;
	m_downloadLocation = downloadLocation;

	// Create all files and directories
	if(!createFileTree(downloadLocation)) {
		setError("Failed to create file tree");
		return false;
	}

	// Create all pieces but the last
	// The last piece could have a different size than the others
	for(int i = 0; i < m_torrentInfo->numberOfPieces() - 1; i++) {
		m_pieces.push_back(new Piece(this, i, m_torrentInfo->pieceLength()));
	}
	int lastPieceLength = m_torrentInfo->length() % m_torrentInfo->pieceLength();
	if(lastPieceLength == 0) {
		// The size of the last piece is the same as the others
		lastPieceLength = m_torrentInfo->pieceLength();
	}
	// Create the last piece
	m_pieces.push_back(new Piece(this, m_torrentInfo->numberOfPieces() - 1, lastPieceLength));

	// Create the tracker client
	m_trackerClient = new TrackerClient(this);

	m_state = Ready;

	return true;
}

bool Torrent::createFromResumeInfo(TorrentInfo *torrentInfo, ResumeInfo *resumeInfo) {
	clearError();
	m_state = Loading;

	m_torrentInfo = torrentInfo;

	// Create all files and directories
	if(!createFileTree(resumeInfo->downloadLocation())) {
		setError("Failed to create file tree");
		return false;
	}

	// Create all pieces but the last
	// The last piece could have a different size than the others
	for(int i = 0; i < m_torrentInfo->numberOfPieces() - 1; i++) {
		m_pieces.push_back(new Piece(this, i, m_torrentInfo->pieceLength()));
	}
	int lastPieceLength = m_torrentInfo->length() % m_torrentInfo->pieceLength();
	if(lastPieceLength == 0) {
		// The size of the last piece is the same as the others
		lastPieceLength = m_torrentInfo->pieceLength();
	}
	// Create the last piece
	m_pieces.push_back(new Piece(this, m_torrentInfo->numberOfPieces() - 1, lastPieceLength));

	// Create the tracker client
	m_trackerClient = new TrackerClient(this);

	if(m_pieces.size() != resumeInfo->aquiredPieces().size()) {
		setError("The number of pieces in the TorrentInfo does not match the one in the ResumeInfo");
		return false;
	}

	// Set all aquired pieces
	for(Piece* piece : m_pieces) {
		if(resumeInfo->aquiredPieces().at(piece->pieceNumber())) {
			setPieceAvailable(piece);
		}
	}

	m_downloadLocation = resumeInfo->downloadLocation();

	m_totalBytesDownloaded = resumeInfo->totalBytesDownloaded();
	m_totalBytesUploaded = resumeInfo->totalBytesUploaded();

	m_state = Ready;

	if(resumeInfo->paused()) {
		pause();
	} else {
		start();
	}

	return true;
}

bool Torrent::createFromMagnetLink(QUrl url) {
	clearError();

	QUrlQuery query(url);

	QString infoHashString;
	QByteArray infoHash;

	QByteArray trackerUrl;

	QString displayName;

	// Every magnet link must have an info hash
	if(!query.hasQueryItem("xt")) {
		setError("Invalid magnet link");
		return false;
	}

	// We can only support magnet links with trackers
	// Because those without trackers require DHT
	if(!query.hasQueryItem("tr")) {
		setError("Magnet link does not have tracker parameter. Magnet links are still not supported.");
		return false;
	}

	// Read info hash
	infoHashString = query.queryItemValue("xt");
	if(!infoHashString.startsWith("urn:btih:")) {
		setError("Magnet link has an invalid info hash");
		return false;
	}
	infoHashString.remove(0, strlen("urn:btih:"));
	infoHash = QByteArray::fromHex(infoHashString.toLatin1());

	// Read the tracker url
	trackerUrl = QByteArray::fromPercentEncoding(query.queryItemValue("tr").toLatin1());

	// Read display name
	displayName = query.queryItemValue("dn");

	// TODO
	setError("The application still does not support magnet links.");
	return false;
}

bool Torrent::createFileTree(const QString &directory) {
	const auto& files = m_torrentInfo->fileInfos();
	QDir rootDir(directory);
	for(auto& f : files) {
		QDir dir(rootDir);
		for(int i = 0; i < f.path.size() - 1; i++) {
			if(!dir.exists(f.path[i])) {
				if(!dir.mkdir(f.path[i])) {
					setError("Failed to create directory " + f.path[i]);
					return false;
				}
			}
			dir.cd(f.path[i]);
		}
		QFile* file = new QFile();
		file->setFileName(dir.absoluteFilePath(f.path.last()));
		if(!file->exists() || file->size() != f.length) {
			if(!file->open(QFile::ReadWrite)) {
				setError("Failed to open file " + file->fileName() + ": " + file->errorString());
				return false;
			}
			if(!file->resize(f.length)) {
				setError("Failed to resize file " + file->fileName() + ": " + file->errorString());
				return false;
			}
			file->close();
		} else {
			qDebug() << "File" << file->fileName() << "exists";
		}
		m_files.push_back(file);
	}
	return true;
}

ResumeInfo Torrent::getResumeInfo() const {
	ResumeInfo resumeInfo(m_torrentInfo);
	resumeInfo.setDownloadLocation(m_downloadLocation);
	resumeInfo.setTotalBytesDownloaded(m_totalBytesDownloaded);
	resumeInfo.setTotalBytesUploaded(m_totalBytesUploaded);
	resumeInfo.setPaused(m_isPaused);
	resumeInfo.setAquiredPieces(bitfield());
	return resumeInfo;
}

void Torrent::start() {
	if(!m_trackerClient->hasAnnouncedStarted()) {
		// Send the first announce to the tracker
		m_trackerClient->announce(TrackerClient::Started);
	} else if(m_trackerClient->numberOfAnnounces() == 0) {
		// Announced if we haven't already
		m_trackerClient->announce(TrackerClient::None);
	}

	// Start all peers
	for(Peer* peer :  m_peers) {
		peer->start();
	}
	m_isPaused = false;
}

void Torrent::pause() {
	// Pause all peers
	for(Peer* peer : m_peers) {
		peer->pause();
	}
	m_isPaused = true;
}

void Torrent::stop() {
	if(m_trackerClient->hasAnnouncedStarted()) {
		m_trackerClient->announce(TrackerClient::Stopped);
	}
}

Peer* Torrent::addPeer(const QByteArray &address, int port) {
	// Don't add the peer if he's already added
	for(auto p : m_peers) {
		if(p->port() == port || p->address() == address) {
			return nullptr;
		}
	}

	// Add the peer
	Peer* peer = Peer::createServer(this, address, port);
	m_peers.push_back(peer);

	// Always start connecting
	peer->startConnection();

	// Pause the peer if needed
	if(m_isPaused) {
		peer->pause();
	}

	qDebug() << "Added peer" << peer->addressPort();
	return peer;
}

void Torrent::addPeer(Peer *peer) {
	// If there's another identical peer - replace it
	for(Peer*& p : m_peers) {
		if(p->port() == peer->port() && p->address() == peer->address()) {
			p->deleteLater();
			p = peer;
			return;
		}
	}

	// Else - add it to the list
	m_peers.push_back(peer);
	qDebug() << "Added peer" << peer->addressPort();
}

Block* Torrent::requestBlock(Peer *peer, int size) {
	Block* returnBlock = nullptr;
	for(auto piece : m_pieces) {
		if(!piece->isDownloaded() && peer->hasPiece(piece)) {
			returnBlock = piece->requestBlock(size);
			if(returnBlock != nullptr) {
				return returnBlock;
			}
		}
	}

	// No unrequested blocks, try to find some timed-out blocks
	for(auto peer : m_peers) {
		if(peer->hasTimedOut()) {
			for(auto block : peer->blocksQueue()) {
				if(peer->hasPiece(block->piece())) {
					return block;
				}
			}
		}
	}

	// No blocks
	return nullptr;
}

bool Torrent::savePiece(Piece* piece) {
	int pieceLength = m_torrentInfo->pieceLength();
	int thisPieceLength = piece->size();
	auto& fileInfos = m_torrentInfo->fileInfos();

	qint64 dataBegin = piece->pieceNumber();
	dataBegin *= pieceLength;

	int firstFileNumber = 0;
	qint64 startingPos = 0;
	qint64 pos = 0;
	for(int i = 0; i < fileInfos.size(); i++) {
		pos += fileInfos[i].length;
		firstFileNumber = i;
		if(pos > dataBegin) {
			pos -= fileInfos[i].length;
			startingPos = dataBegin - pos;
			break;
		}
	}

	char* dataPtr = piece->data();

	int bytesToWrite = thisPieceLength;
	for(int i = firstFileNumber;; i++) {
		QFile* file = m_files[i];
		if(!file->open(QIODevice::ReadWrite)) { // Append causes bugs with seek
			qDebug() << "Failed to open file" << i << file->fileName() << ":" << file->errorString();
			return false;
		}
		qint64 pos = 0;
		if(i == firstFileNumber) {
			pos = startingPos;
		}
		if(!file->seek(pos)) {
			qDebug() << "Failed to seek in file" << i << file->fileName() << ":" << file->errorString();
			return false;
		}
		int toWrite = bytesToWrite;
		qint64 bytesToEnd = fileInfos[i].length - pos;
		if(toWrite > bytesToEnd) {
			toWrite = bytesToEnd;
		}
		for(int written = 0;;) {
			dataPtr += written;
			toWrite -= written;
			bytesToWrite -= written;
			if(toWrite == 0) {
				break;
			}
			written = file->write(dataPtr, toWrite);
			if(written == -1) {
				qDebug() << "Failed to write to file" << i << file->fileName() << ":" << file->errorString();
				file->close();
				return false;
			}
		}
		file->close();
		if(bytesToWrite == 0) {
			break;
		}
	}
	return true;
}

void Torrent::setPieceAvailable(Piece *piece) {
	if(piece->isDownloaded()) {
		return;
	}

	piece->setDownloaded(true);

	// Increment some counters
	m_downloadedPieces++;

	if(m_state == Ready) {
		m_totalBytesDownloaded += piece->size();

		// Send 'have' messages to all peers
		for(auto peer : m_peers) {
			peer->sendHave(piece->pieceNumber());
		}
	}

	// Check if all pieces are downloaded
	if(m_downloadedPieces == m_torrentInfo->numberOfPieces()) {
		fullyDownloaded();
	}
}

void Torrent::successfullyAnnounced(TrackerClient::Event event) {
	if(event == TrackerClient::Started) {
		m_bytesDownloadedOnStartup = m_totalBytesDownloaded;
		m_bytesUploadedOnStartup = m_totalBytesUploaded;
	}
}


/* Getters */

QList<Peer*>& Torrent::peers() {
	return m_peers;
}

QList<Piece*>& Torrent::pieces() {
	return m_pieces;
}

TorrentInfo* Torrent::torrentInfo() {
	return m_torrentInfo;
}

TrackerClient* Torrent::trackerClient() {
	return m_trackerClient;
}

QList<QFile*>& Torrent::files() {
	return m_files;
}


qint64 Torrent::bytesDownloaded() const {
	if(!m_trackerClient->hasAnnouncedStarted()) {
		return 0;
	}
	return m_totalBytesDownloaded - m_bytesDownloadedOnStartup;
}

qint64 Torrent::bytesUploaded() const {
	if(!m_trackerClient->hasAnnouncedStarted()) {
		return 0;
	}
	return m_totalBytesUploaded - m_bytesUploadedOnStartup;
}

qint64 Torrent::totalBytesDownloaded() const {
	return m_totalBytesDownloaded;
}

qint64 Torrent::totalBytesUploaded() const {
	return m_totalBytesUploaded;
}

qint64 Torrent::bytesAvailable() const {
	qint64 bytes = 0;
	for(Piece* piece : m_pieces) {
		if(piece->isDownloaded()) {
			bytes += piece->size();
		}
	}
	return bytes;
}

qint64 Torrent::bytesLeft() const {
	return m_torrentInfo->length() - bytesAvailable();
}


int Torrent::downloadedPieces() {
	return m_downloadedPieces;
}

bool Torrent::isDownloaded() {
	return m_isDownloaded;
}

bool Torrent::isPaused() const {
	return m_isPaused;
}

int Torrent::connectedPeersCount() const {
	int count = 0;
	for(Peer* peer : m_peers) {
		if(peer->isConnected()) {
			count++;
		}
	}
	return count;
}

int Torrent::allPeersCount() const {
	return m_peers.size();
}

Torrent::State Torrent::state() const {
	return m_state;
}

QString Torrent::stateString() const {
	if(m_state == New) {
		return "Created";
	} else if(m_state == Loading) {
		return "Loading torrent";
	} else if(m_state == Checking) {
		return "Checking torrent";
	} else if(m_isPaused) {
		return "Paused";
	} else if(m_isDownloaded) {
		return "Completed";
	} else {
		return "Downloading";
	}
}

const QString& Torrent::downloadLocation() const {
	return m_downloadLocation;
}


float Torrent::percentDownloaded() {
	float percent = m_downloadedPieces;
	percent /= m_torrentInfo->numberOfPieces();
	percent *= 100.0f;
	return percent;
}

QVector<bool> Torrent::bitfield() const {
	QVector<bool> bf;
	for(auto p : m_pieces) {
		bf.push_back(p->isDownloaded());
	}
	return bf;
}


QString Torrent::errorString() const {
	return m_errorString;
}


/* signals */

void Torrent::downloadedPiece(Piece *piece) {
	// Increment some counters
	m_downloadedPieces++;
	m_totalBytesDownloaded += piece->size();

	qDebug() << "Downloaded pieces"
			 << m_downloadedPieces << "/" << m_torrentInfo->numberOfPieces()
			 << "=" << percentDownloaded() << "%";

	// Send 'have' messages to all peers
	for(auto peer : m_peers) {
		peer->sendHave(piece->pieceNumber());
	}

	// Check if all pieces are downloaded
	if(m_downloadedPieces == m_torrentInfo->numberOfPieces()) {
		fullyDownloaded();
	}
}

void Torrent::uploadedBlock(int bytes) {
	m_totalBytesUploaded += bytes;
}

void Torrent::fullyDownloaded() {
	qDebug() << "Torrent fully downloaded!";
	QTorrent::instance()->mainWindow()->torrentFullyDownloaded(this);
	m_isDownloaded = true;

	if(m_state == Ready) {
		// Send announce
		m_trackerClient->announce(TrackerClient::Completed);
	}

	// Disconnect from all peers that have the full torrent
	for(auto peer : m_peers) {
		if(peer->isDownloaded() || peer->isConnected()) {
			peer->disconnect();
		}
	}
}

void Torrent::clearError() {
	m_errorString.clear();
}

void Torrent::setError(const QString &errorString) {
	m_errorString = errorString;
}
