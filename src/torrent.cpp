#include "torrent.h"
#include "peer.h"
#include "torrentinfo.h"
#include "trackerclient.h"
#include "piece.h"
#include "block.h"
#include "torrentmessage.h"
#include <QDir>
#include <QFile>

Torrent::Torrent(QTorrent *qTorrent) :
	m_qTorrent(qTorrent),
	m_torrentInfo(nullptr),
	m_trackerClient(nullptr),
	m_bytesDownloaded(0),
	m_bytesUploaded(0),
	m_downloadedPieces(0),
	m_downloaded(false)
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

bool Torrent::createFromFile(const QString &filename) {
	m_torrentInfo = new TorrentInfo();

	// Load torrent info from bencoded .torrent file
	if(!m_torrentInfo->loadTorrentFile(filename)) {
		qDebug() << "Failed to load torrent file" << m_torrentInfo->errorString();
		return false;
	}

	// Create all files and directories
	if(!createFileTree(".")) {
		return false;
	}

	// Create pieces
	if(m_torrentInfo->length() % m_torrentInfo->pieceLength() == 0) {
		// All pieces are the same size
		for(int i = 0; i < m_torrentInfo->numberOfPieces(); i++) {
			Piece* piece = new Piece(this, i, m_torrentInfo->pieceLength());
			m_pieces.push_back(piece);
		}
	} else {
		// Last piece has different size
		for(int i = 0; i < m_torrentInfo->numberOfPieces()-1; i++) {
			Piece* piece = new Piece(this, i, m_torrentInfo->pieceLength());
			m_pieces.push_back(piece);
		}
		int lastPieceLength = m_torrentInfo->length() % m_torrentInfo->pieceLength();
		m_pieces.push_back(new Piece(this, m_torrentInfo->numberOfPieces()-1, lastPieceLength));
	}

	// Create tracker client
	m_trackerClient = new TrackerClient(this, m_torrentInfo);

	// Send the first announce to the tracker
	m_trackerClient->announce(TrackerClient::Started);

	return true;
}

bool Torrent::createFileTree(const QString &directory) {
	const auto& files = m_torrentInfo->fileInfos();
	QDir rootDir(directory);
	for(auto& f : files) {
		QDir dir(rootDir);
		for(int i = 0; i < f.path.size() - 1; i++) {
			if(!dir.exists(f.path[i])) {
				if(!dir.mkdir(f.path[i])) {
					qDebug() << "Failed to create directory" << f.path[i];
					return false;
				}
			}
			dir.cd(f.path[i]);
		}
		QFile* file = new QFile();
		file->setFileName(dir.absoluteFilePath(f.path.last()));
		if(!file->open(QFile::ReadWrite)) {
			qDebug() << "Failed to open file" << file->errorString();
			return false;
		}
		if(!file->resize(f.length)) {
			qDebug() << "Failed to resize file" << file->errorString();
			return false;
		}
		file->close();
		m_files.push_back(file);
	}
	return true;
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

	// Start connecting if torrent isn't downloaded yet
	if(!m_downloaded) {
		peer->startConnection();
	}
	return peer;
}

Block* Torrent::requestBlock(Peer *peer, int size) {
	Block* block = nullptr;
	for(int i = 0; i < m_pieces.size(); i++) {
		auto piece = m_pieces[i];
		if(!piece->downloaded()) {
			if(peer->hasPiece(i)) {
				block = piece->requestBlock(size);
				if(block != nullptr) {
					block->addAssignee(peer);
					break;
				}
			}
		}
	}

	// No unrequested blocks, try to find some timed-out blocks
	if(block == nullptr) {
		for(auto p : m_peers) {
			if(p->timedOut()) {
				for(auto bl : p->blocksQueue()) {
					int pieceIndex = bl->piece()->pieceNumber();
					if(peer->hasPiece(pieceIndex)) {
						block = bl;
						block->addAssignee(peer);
						break;
					}
				}
			}
		}
	}
	return block;
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


/* Getters */

QTorrent* Torrent::qTorrent() {
	return m_qTorrent;
}

QList<Peer*>& Torrent::peers() {
	return m_peers;
}

TorrentInfo* Torrent::torrentInfo() {
	return m_torrentInfo;
}

TrackerClient* Torrent::trackerClient() {
	return m_trackerClient;
}


qint64 Torrent::bytesDownloaded() {
	return m_bytesDownloaded;
}

qint64 Torrent::bytesUploaded() {
	return m_bytesUploaded;
}

int Torrent::downloadedPieces() {
	return m_downloadedPieces;
}

bool Torrent::downloaded() {
	return m_downloaded;
}


float Torrent::percentDownloaded() {
	float percent = m_downloadedPieces;
	percent /= m_torrentInfo->numberOfPieces();
	percent *= 100.0f;
	return percent;
}


/* signals */

void Torrent::downloadedPiece(Piece *piece) {
	// Increment some counters
	m_downloadedPieces++;
	m_bytesDownloaded += piece->size();

	qDebug() << "Downloaded pieces"
			 << m_downloadedPieces << "/" << m_torrentInfo->numberOfPieces()
			 << "=" << percentDownloaded() << "%";

	// Send 'have' messages to all peers
	for(auto peer : m_peers) {
		TorrentMessage::have(peer->socket(), piece->pieceNumber());
	}

	// Check if all pieces are downloaded
	if(m_downloadedPieces == m_torrentInfo->numberOfPieces()) {
		fullyDownloaded();
	}
}

void Torrent::uploadedPiece(Piece *piece) {
	m_bytesUploaded += piece->size();
}

void Torrent::fullyDownloaded() {
	qDebug() << "Torrent fully downloaded!";
	m_downloaded = true;

	// Send announce
	m_trackerClient->announce(TrackerClient::Completed);

	// Disconnect from peers
	for(auto peer : m_peers) {
		peer->disconnect();
		delete peer;
	}
	m_peers.clear();
}
