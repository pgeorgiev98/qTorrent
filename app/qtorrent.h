/* qTorrent - An open-source, cross-platform BitTorrent client
 * Copyright (C) 2017 Petko Georgiev
 *
 * qtorrent.h
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

#ifndef QTORRENT_H
#define QTORRENT_H

#include "core/torrentsettings.h"
#include <QObject>
#include <QHostAddress>
#include <QList>
#include <QString>
#include <QUrl>

class Torrent;
class TorrentManager;
class TorrentServer;
class MainWindow;
class LocalServiceDiscoveryClient;

class QTorrent : public QObject
{
	Q_OBJECT

public:
	QTorrent();
	~QTorrent();

	bool startServer();
	bool resumeTorrents();
	void startLSDClient();
	bool addTorrentFromUrl(QUrl url);

	bool removeTorrent(Torrent *torrent, bool deleteData);

	void shutDown();

	void showMainWindow();

	/* Opens a critical MessageBox */
	void critical(const QString &text);
	/* Opens an information MessageBox */
	void information(const QString &text);
	/* Opens question MessageBox. Returns true on 'yes' */
	bool question(const QString &text);
	/* Opens a warning MessageBox */
	void warning(const QString &text);

	const QByteArray &peerId() const;
	const QList<Torrent *> &torrents() const;
	TorrentManager *torrentManager();
	TorrentServer *server();
	MainWindow *mainWindow();

	static QTorrent *instance();

public slots:
	void LSDPeerFound(QHostAddress address, int port, Torrent *torrent);

private:
	QByteArray m_peerId;

	TorrentManager *m_torrentManager;
	TorrentServer *m_server;
	LocalServiceDiscoveryClient *m_LSDClient;

	MainWindow *m_mainWindow;

	static QTorrent *m_instance;
};

#endif // QTORRENT_H
