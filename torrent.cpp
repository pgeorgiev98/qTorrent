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
	for(int i = 0; i < m_torrentInfo->numberOfPieces(); i++) {
		Piece* piece = new Piece(this, i);
		m_pieces.push_back(piece);
	}
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
		QFile file;
		file.setFileName(dir.absoluteFilePath(f.path));
		if(!file.open(QFile::ReadWrite)) {
			qDebug() << "Failed to open file" << file.errorString();
			return false;
		}
		qDebug() << "file" << file.fileName();
		if(!file.resize(f.length)) {
			qDebug() << "Failed to resize" << file.errorString();
			return false;
		}
		file.close();
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
