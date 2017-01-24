#ifndef TORRENTSLIST_H
#define TORRENTSLIST_H

#include "torrentslistitem.h"
#include "panel.h"
#include <QTreeWidget>
#include <QTimer>
#include <QList>

class QTorrent;
class Torrent;

class TorrentsList : public QTreeWidget {
	Q_OBJECT

public:
	TorrentsList(QTorrent* qTorrent);
	~TorrentsList();

	// Get item from list
	TorrentsListItem* torrentItem(Torrent* torrent);
	TorrentsListItem* torrentItem(const QString& name);

public slots:
	void addTorrent(Torrent* torrent);
	void removeTorrent(Torrent* torrent);
	void refresh();
	void showAll();
	void showCompleted();
	void showDownloading();
	void showUploading();

protected:
	// For the drag and drop functionality
	void dragEnterEvent(QDragEnterEvent *event);
	void dragMoveEvent(QDragMoveEvent *event);
	void dropEvent(QDropEvent *event);

private:
	QTorrent* m_qTorrent;
	QList<TorrentsListItem*> m_items;
	QTimer m_refreshTimer;
};

#endif // TORRENTSLIST_H
