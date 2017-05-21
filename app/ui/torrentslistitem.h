/* qTorrent - An open-source, cross-platform BitTorrent client
 * Copyright (C) 2017 Petko Georgiev
 *
 * torrentslistitem.h
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

#ifndef TORRENTSLISTITEM_H
#define TORRENTSLISTITEM_H

#include <QTreeWidgetItem>
#include <QVariant>
#include <QMap>

class Torrent;

class TorrentsListItem : public QObject, public QTreeWidgetItem
{
	Q_OBJECT

public:
	enum Section {
		Name=0, Size, Peers, State, Progress, Available, Left,
		TotalDownloaded, TotalUploaded, Ratio, Downloaded, Uploaded
	};

	TorrentsListItem(QTreeWidget *view, Torrent *torrent);

	void setSortData(int column, QVariant data);
	bool operator<(const QTreeWidgetItem &other) const;
	void refresh();
	// true if this torrent belongs to the currently opened section
	bool belongsToSection();

	Torrent *torrent() const;

	void setName(const QString &value);
	void setSize(qint64 value);
	void setPeers(int connected, int all);
	void setState(const QString &state);
	void setProgress(float value);
	void setAvailable(qint64 value);
	void setLeft(qint64 value);
	void setTotalDownloaded(qint64 value);
	void setTotalUploaded(qint64 value);
	void setRatio(float value);
	void setDownloaded(qint64 value);
	void setUploaded(qint64 value);

public slots:
	void onOpenAction();
	void onOpenLocationAction();
	void onPauseAction();
	void onStartAction();
	void onStopAction();
	void onRecheckAction();
	void onRemoveAction();

signals:
	void removeTorrent(Torrent *torrent, bool deleteData);

private:
	Torrent *m_torrent;
	QMap<int, QVariant> m_sortData;
};

#endif // TORRENTSLISTITEM_H
