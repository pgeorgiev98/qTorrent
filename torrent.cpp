#include "torrent.h"
#include "peer.h"
#include "torrentinfo.h"
#include "torrentclient.h"
#include "trackerclient.h"

Torrent::Torrent(QTorrent *qTorrent) :
	m_qTorrent(qTorrent),
	m_torrentInfo(nullptr),
	m_trackerClient(nullptr),
	m_torrentClient(nullptr)
{
}

Torrent::~Torrent() {
	for(auto peer : m_peers) {
		delete peer;
	}
	if(m_torrentInfo) delete m_torrentInfo;
	if(m_torrentClient) delete m_torrentClient;
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
