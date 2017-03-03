/* qTorrent - An open-source, cross-platform BitTorrent client
 * Copyright (C) 2017 Petko Georgiev
 *
 * torrentslist.h
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef TORRENTSLIST_H
#define TORRENTSLIST_H

#include "torrentslistitem.h"
#include "panel.h"
#include <QTreeWidget>
#include <QList>

class Torrent;

class TorrentsList : public QTreeWidget {
	Q_OBJECT

public:
	TorrentsList();
	~TorrentsList();

	// Get item from list
	TorrentsListItem* torrentItem(Torrent* torrent);
	TorrentsListItem* torrentItem(const QString& name);
	Torrent* currentTorrent();

public slots:
	void addTorrent(Torrent* torrent);
	void removeTorrent(Torrent* torrent);
	void refresh();
	void openContextMenu(const QPoint& pos);
	void openHeaderContextMenu(const QPoint& pos);
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
	QList<TorrentsListItem*> m_items;
};

#endif // TORRENTSLIST_H
