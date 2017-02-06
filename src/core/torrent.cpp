#include "torrent.h"
#include "peer.h"
#include "torrentinfo.h"
#include "piece.h"
#include "block.h"
#include "torrentmessage.h"
#include <QDir>
#include <QFile>
#include <QUrlQuery>

Torrent::Torrent(QTorrent *qTorrent)
	: m_qTorrent(qTorrent)
	, m_status(New)
	, m_torrentInfo(nullptr)
	, m_trackerClient(nullptr)
	, m_bytesDownloadedOnStartup(0)
	, m_bytesUploadedOnStartup(0)
	, m_totalBytesDownloaded(0)
	, m_totalBytesUploaded(0)
	, m_downloadedPieces(0)
	, m_downloaded(false)
	, m_paused(true)
	, m_hasAnnouncedStarted(false)
{
}

Torrent::~Torrent() {
	for(auto peer : m_peers) {
		delete peer;
	}

	if(m_torrentInfo) {
		delete m_torrentInfo;
	}

	if(m_trackerClient) {
		delete m_trackerClient;
	}
}

bool Torrent::createNew(TorrentInfo *torrentInfo, const QString &downloadLocation) {
	clearError();

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
	m_trackerClient = new TrackerClient(this, m_torrentInfo);

	m_status = Ready;

	return true;
}

bool Torrent::createFromResumeInfo(TorrentInfo *torrentInfo, ResumeInfo *resumeInfo) {
	clearError();

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
	m_trackerClient = new TrackerClient(this, m_torrentInfo);

	if(m_pieces.size() != resumeInfo->aquiredPieces().size()) {
		setError("The number of pieces in the TorrentInfo does not match the one in the ResumeInfo");
		return false;
	}

	// Set all pieces
	for(int i = 0; i < m_pieces.size(); i++) {
		Piece* piece = m_pieces[i];
		bool downloaded = resumeInfo->aquiredPieces()[i];
		piece->setDownloaded(downloaded);
		if(downloaded) {
			m_downloadedPieces++;
		}
	}
	if(m_downloadedPieces == m_torrentInfo->numberOfPieces()) {
		m_downloaded = true;
	}

	m_downloadLocation = resumeInfo->downloadLocation();

	m_totalBytesDownloaded = resumeInfo->totalBytesDownloaded();
	m_totalBytesUploaded = resumeInfo->totalBytesUploaded();

	m_status = Ready;

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
	resumeInfo.setPaused(m_paused);
	resumeInfo.setAquiredPieces(bitfield());
	return resumeInfo;
}

void Torrent::start() {
	if(!m_hasAnnouncedStarted) {
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
	m_paused = false;
}

void Torrent::pause() {
	// Pause all peers
	for(Peer* peer : m_peers) {
		peer->pause();
	}
	m_paused = true;
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
	if(m_paused) {
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
	Block* block = nullptr;
	for(int i = 0; i < m_pieces.size(); i++) {
		auto piece = m_pieces[i];
		if(!piece->downloaded()) {
			if(peer->hasPiece(i)) {
				block = piece->requestBlock(size);
				if(block != nullptr) {
					return block;
				}
			}
		}
	}

	// No unrequested blocks, try to find some timed-out blocks
	for(auto p : m_peers) {
		if(p->timedOut()) {
			for(auto bl : p->blocksQueue()) {
				int pieceIndex = bl->piece()->pieceNumber();
				if(peer->hasPiece(pieceIndex)) {
					return bl;
				}
			}
		}
	}

	// No blocks
	return nullptr;
}

void Torrent::releaseBlock(Peer *client, Block *block) {
	Piece* piece = block->piece();
	block->removeAssignee(client);
	if(block->assignees().isEmpty()) {
		piece->deleteBlock(block);
	}
}

bool Torrent::savePiece(int pieceNumber) {
	int pieceLength = m_torrentInfo->pieceLength();
	int thisPieceLength = m_pieces[pieceNumber]->size();
	auto& fileInfos = m_torrentInfo->fileInfos();

	qint64 dataBegin = pieceNumber;
	dataBegin *= pieceLength;

	int firstFileNumber = 0;
	qint64 startingPos = 0;
	qint64 tmp = 0;
	for(int i = 0; i < fileInfos.size(); i++) {
		tmp += fileInfos[i].length;
		firstFileNumber = i;
		if(tmp > dataBegin) {
			tmp -= fileInfos[i].length;
			startingPos = dataBegin - tmp;
			break;
		}
	}

	char* dataPtr = m_pieces[pieceNumber]->data();

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

void Torrent::successfullyAnnounced(TrackerClient::Event event) {
	if(event == TrackerClient::Started) {
		m_hasAnnouncedStarted = true;
		m_bytesDownloadedOnStartup = m_totalBytesDownloaded;
		m_bytesUploadedOnStartup = m_totalBytesUploaded;
	}
}


/* Getters */

QTorrent* Torrent::qTorrent() {
	return m_qTorrent;
}

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
	if(!m_hasAnnouncedStarted) {
		return 0;
	}
	return m_totalBytesDownloaded - m_bytesDownloadedOnStartup;
}

qint64 Torrent::bytesUploaded() const {
	if(!m_hasAnnouncedStarted) {
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
		if(piece->downloaded()) {
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

bool Torrent::downloaded() {
	return m_downloaded;
}

bool Torrent::isPaused() const {
	return m_paused;
}

bool Torrent::hasAnnouncedStarted() const {
	return m_hasAnnouncedStarted;
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
		bf.push_back(p->downloaded());
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
	m_downloaded = true;

	// Send announce
	m_trackerClient->announce(TrackerClient::Completed);

	// Disconnect from all peers that have the full torrent
	for(auto peer : m_peers) {
		if(peer->downloaded() || peer->isConnected()) {
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
