#include "core/torrentmanager.h"
#include "core/torrentinfo.h"
#include "core/torrent.h"
#include "ui/mainwindow.h"
#include "qtorrent.h"

TorrentManager::TorrentManager(QTorrent *qTorrent)
	: m_qTorrent(qTorrent)
{
}

TorrentManager::~TorrentManager() {
	for(Torrent* torrent : m_torrents) {
		delete torrent;
	}
}

Torrent* TorrentManager::addTorrentFromLocalFile(const QString &filename) {
	// Load the torrent file
	TorrentInfo* torrentInfo = new TorrentInfo;
	if(!torrentInfo->loadFromTorrentFile(filename)) {
		m_qTorrent->warning("Invalid torrent file");
		delete torrentInfo;
		return nullptr;
	}

	// Check if torrent already added to list
	for(Torrent* t : m_torrents) {
		if(t->torrentInfo()->infoHash() == torrentInfo->infoHash()) {
			m_qTorrent->warning("The torrent you're trying to add is already in the torrents list.");
			delete torrentInfo;
			return nullptr;
		}
	}

	// Get the download location
	QString downloadLocation = m_qTorrent->mainWindow()->getDownloadLocation();
	if(downloadLocation.isEmpty()) {
		delete torrentInfo;
		return nullptr;
	}

	// Create the torrent
	Torrent* torrent = new Torrent(m_qTorrent);
	if(!torrent->createNew(torrentInfo, downloadLocation)) {
		m_qTorrent->warning(torrent->errorString());
		delete torrent;
		return nullptr;
	}

	m_torrents.push_back(torrent);
	m_qTorrent->mainWindow()->addTorrent(torrent);

	torrent->start();

	return torrent;
}

Torrent* TorrentManager::addTorrentFromMagnetLink(QUrl url) {
	Torrent* torrent = new Torrent(m_qTorrent);
	if(!torrent->createFromMagnetLink(url)) {
		m_qTorrent->warning("Failed to load torrent from magnet link\n" + torrent->errorString());
		delete torrent;
		return nullptr;
	}

	m_torrents.push_back(torrent);
	m_qTorrent->mainWindow()->addTorrent(torrent);

	torrent->start();

	return torrent;
}

QTorrent* TorrentManager::qTorrent() {
	return m_qTorrent;
}

const QList<Torrent*>& TorrentManager::torrents() const {
	return m_torrents;
}
