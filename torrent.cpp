#include "torrent.h"
#include "peer.h"
#include "torrentinfo.h"
#include "torrentclient.h"
#include "trackerclient.h"
#include "piece.h"

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
	TorrentInfo* torrentInfo = new TorrentInfo();
	if(!torrentInfo->loadTorrentFile(filename)) {
		delete torrentInfo;
		torrentInfo = nullptr;
		return false;
	}
	TrackerClient* trackerClient = new TrackerClient(this, torrentInfo);
	trackerClient->fetchPeerList();

	m_torrentInfo = torrentInfo;
	m_trackerClient = trackerClient;
    for(int i = 0; i < m_torrentInfo->numberOfPieces(); i++) {
        Piece* piece = new Piece(this, i);
        m_pieces.push_back(piece);
    }
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
