#ifndef QTORRENT_H
#define QTORRENT_H

#include <QList>
#include <QString>

class Torrent;

class QTorrent {
public:
	QTorrent();
	~QTorrent();
	bool addTorrent(const QString& filename);
private:
	QList<Torrent*> m_torrents;
};

#endif // QTORRENT_H
