#ifndef TORRENTMANAGER_H
#define TORRENTMANAGER_H

#include "torrentsettings.h"
#include <QList>
#include <QUrl>

class Torrent;
class TorrentInfo;

class TorrentManager
{
public:
	TorrentManager();
	~TorrentManager();

	Torrent* addTorrentFromLocalFile(const QString& filename, const TorrentSettings& settings);

	// Loads all saved for resuming torrents
	bool resumeTorrents();

	// Not usable
	Torrent* addTorrentFromMagnetLink(QUrl url);

	// Saves resume info for all torrents
	bool saveTorrentsResumeInfo();
	// Permanently saves the torrent file to the app data directory
	bool saveTorrentFile(const QString& filename, TorrentInfo* torrentInfo);

	bool removeTorrent(Torrent* torrent, bool deleteData);

	/* Getters */
	const QList<Torrent*>& torrents() const;

private:
	QList<Torrent*> m_torrents;
};

#endif // TORRENTMANAGER_H
