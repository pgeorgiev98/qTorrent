#ifndef TORRENTMANAGER_H
#define TORRENTMANAGER_H

#include <QList>
#include <QUrl>

class QTorrent;
class Torrent;

class TorrentManager
{
public:
	TorrentManager(QTorrent* qTorrent);
	~TorrentManager();

	Torrent* addTorrentFromLocalFile(const QString& filename);

	// Not usable
	Torrent* addTorrentFromMagnetLink(QUrl url);

	/* Getters */
	QTorrent* qTorrent();
	const QList<Torrent*>& torrents() const;

private:
	QTorrent* m_qTorrent;
	QList<Torrent*> m_torrents;
};

#endif // TORRENTMANAGER_H
