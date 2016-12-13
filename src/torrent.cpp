#include "torrent.h"
#include "peer.h"
#include "torrentinfo.h"
#include "trackerclient.h"
#include "piece.h"
#include "block.h"
#include <QDir>
#include <QFile>

Torrent::Torrent(QTorrent *qTorrent) :
	m_qTorrent(qTorrent),
	m_torrentInfo(nullptr),
	m_trackerClient(nullptr)
{
}

Torrent::~Torrent() {
	for(auto peer : m_peers) {
		delete peer;
	}
	if(m_torrentInfo) delete m_torrentInfo;
	if(m_trackerClient) delete m_trackerClient;
}

bool Torrent::createFromFile(const QString &filename) {
	m_torrentInfo = new TorrentInfo();
	if(!m_torrentInfo->loadTorrentFile(filename)) {
		qDebug() << "Failed to load torrent file" << m_torrentInfo->errorString();
		delete m_torrentInfo;
		m_torrentInfo = nullptr;
		return false;
	}
	if(!createFileTree(".")) {
		return false;
	}
	TrackerClient* trackerClient = new TrackerClient(this, m_torrentInfo);
	trackerClient->announce(TrackerClient::Started);

	m_trackerClient = trackerClient;

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
	return true;
}

bool Torrent::createFileTree(const QString &directory) {
	qDebug() << "Creating file tree for" << m_torrentInfo->torrentName();
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
		qDebug() << "file" << file->fileName();
		if(!file->resize(f.length)) {
			qDebug() << "Failed to resize" << file->errorString();
			return false;
		}
		file->close();
		m_files.push_back(file);
	}
	qDebug() << "Successful";
	return true;
}

Peer* Torrent::addPeer(const QByteArray &address, int port) {
	for(auto p : m_peers) {
		if(p->port() == port || p->address() == address) {
			return nullptr;
		}
	}
	Peer* peer = new Peer(this, Peer::Server, address, port);
	m_peers.push_back(peer);
	return peer;
}

Block* Torrent::requestBlock(Peer *peer, int size) {
	Block* block = nullptr;
	for(int i = 0; i < m_pieces.size(); i++) {
		auto piece = m_pieces[i];
		if(peer->bitfield()[i] && !piece->downloaded()) {
			block = piece->requestBlock(size);
			if(block != nullptr) {
				block->addAssignee(peer);
				break;
			}
		}
	}

	if(block == nullptr) {
		for(auto p : m_peers) {
			if(p->timedOut()) {
				for(auto bl : p->blocksQueue()) {
					if(peer->bitfield()[bl->piece()->pieceNumber()]) {
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

int Torrent::downloadedPieces() {
	int ans = 0;
	for(auto& p : m_pieces) {
		if(p->downloaded()) {
			ans++;
		}
	}
	return ans;
}

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

void Torrent::addToBytesDownloaded(qint64 value) {
	m_bytesDownloaded += value;
}

void Torrent::addToBytesUploaded(qint64 value) {
	m_bytesUploaded += value;
}
