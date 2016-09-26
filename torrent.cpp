#include "torrent.h"
#include "peer.h"
#include "torrentinfo.h"
#include "torrentclient.h"
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
		delete m_torrentInfo;
		m_torrentInfo = nullptr;
		return false;
	}
	if(!createFileTree(".")) {
		return false;
	}
	TrackerClient* trackerClient = new TrackerClient(this, m_torrentInfo);
	trackerClient->fetchPeerList();

	m_trackerClient = trackerClient;
	for(int i = 0; i < m_torrentInfo->numberOfPieces()-1; i++) {
		Piece* piece = new Piece(this, i, m_torrentInfo->pieceLength());
		m_pieces.push_back(piece);
	}
	int lastPieceLength = m_torrentInfo->length() % m_torrentInfo->pieceLength();
	m_pieces.push_back(new Piece(this, m_torrentInfo->numberOfPieces()-1, lastPieceLength));
	return true;
}

bool Torrent::createFileTree(const QString &directory) {
	qDebug() << "Creating file tree for" << m_torrentInfo->torrentName();
	const auto& files = m_torrentInfo->fileInfos();
	qDebug() << "files:" << files.size();
	QDir dir(directory);
	if(files.size() > 1) {
		qDebug() << "Creating sub-directory";
		if(!dir.exists(m_torrentInfo->torrentName())) {
			if(!dir.mkdir(m_torrentInfo->torrentName())) {
				return false;
			}
		}
		if(!dir.cd(m_torrentInfo->torrentName())) {
			return false;
		}
	}
	for(auto& f : files) {
		QFile* file = new QFile();
		file->setFileName(dir.absoluteFilePath(f.path));
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

void Torrent::addPeer(Peer *peer) {
	for(auto p : m_peers) {
		if(p == peer) {
			return;
		}
		if(p->port() != peer->port()) {
			continue;
		}
		if(p->address() != peer->address()) {
			continue;
		}
		delete peer;
		return;
	}
	m_peers.push_back(peer);
}

Block* Torrent::requestBlock(TorrentClient *client, int size) {
	m_requestBlockMutex.lock();

	Block* block = nullptr;
	for(int i = 0; i < m_pieces.size(); i++) {
		auto piece = m_pieces[i];
		if(client->peer()->bitfield()[i] && !piece->downloaded()) {
			block = piece->requestBlock(size);
			if(block != nullptr) {
				break;
			}
		}
	}

	m_requestBlockMutex.unlock();
	return block;
}

bool Torrent::savePiece(int pieceNumber) {
	m_savePieceMutex.lock();
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

	QByteArray data = m_pieces[pieceNumber]->data();
	char* dataPtr = data.data();

	int bytesToWrite = thisPieceLength;
	for(int i = firstFileNumber;; i++) {
		QFile* file = m_files[i];
		if(!file->open(QIODevice::ReadWrite)) { // Append causes bugs with seek
			qDebug() << "Failed to open file" << i << file->fileName() << ":" << file->errorString();
			m_savePieceMutex.unlock();
			return false;
		}
		int pos = 0;
		if(i == firstFileNumber) {
			pos = startingPos;
		}
		if(!file->seek(pos)) {
			qDebug() << "Failed to seek in file" << i << file->fileName() << ":" << file->errorString();
			m_savePieceMutex.unlock();
			return false;
		}
		int toWrite = bytesToWrite;
		int bytesToEnd = fileInfos[i].length - pos;
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
				m_savePieceMutex.unlock();
				return false;
			}
		}
		file->close();
		if(bytesToWrite == 0) {
			break;
		}
	}
	m_savePieceMutex.unlock();
	return true;
}

int Torrent::downloadedPieces() {
	int ans = 0;
	m_getDownloadedPiecesMutex.lock();
	for(auto& p : m_pieces) {
		if(p->downloaded()) {
			ans++;
		}
	}
	m_getDownloadedPiecesMutex.unlock();
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
