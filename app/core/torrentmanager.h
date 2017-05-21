/* qTorrent - An open-source, cross-platform BitTorrent client
 * Copyright (C) 2017 Petko Georgiev
 *
 * torrentmanager.h
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

#ifndef TORRENTMANAGER_H
#define TORRENTMANAGER_H

#include "torrentsettings.h"
#include <QObject>
#include <QList>
#include <QUrl>

class Torrent;
class TorrentInfo;

class TorrentManager : public QObject
{
	Q_OBJECT

public:
	TorrentManager();
	~TorrentManager();

	/* Getters */
	const QList<Torrent *> &torrents() const;
	const QString &errorString() const;

signals:
	void torrentAdded(Torrent *torrent);

public slots:
	// Loads all saved for resuming torrents
	bool resumeTorrents();

	Torrent *addTorrentFromInfo(TorrentInfo *torrentInfo, const TorrentSettings &settings);

	// Saves resume info for all torrents
	bool saveTorrentsResumeInfo();
	// Permanently saves the torrent file to the app data directory
	bool saveTorrentFile(const QString &filename, TorrentInfo *torrentInfo);

	bool removeTorrent(Torrent *torrent, bool deleteData);


private:
	QList<Torrent *> m_torrents;
	QString m_errorString;
};

#endif // TORRENTMANAGER_H
