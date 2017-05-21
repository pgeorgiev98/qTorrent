/* qTorrent - An open-source, cross-platform BitTorrent client
 * Copyright (C) 2017 Petko Georgiev
 *
 * mainwindow.h
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

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSystemTrayIcon>
#include <QTimer>
#include <QUrl>

class Panel;
class TorrentsList;
class TorrentInfoPanel;
class Torrent;

class MainWindow : public QMainWindow
{
	Q_OBJECT
public:
	MainWindow();
	~MainWindow();

	static MainWindow* instance();

	Panel *panel();
	TorrentsList *torrentsList();

	void closeEvent(QCloseEvent *event);

	QString getDownloadLocation();

private:
	Panel *m_panel;
	TorrentsList *m_torrentsList;
	TorrentInfoPanel *m_infoPanel;

	QAction *m_viewTorrentsFilterPanel;
	QAction *m_viewTorrentInfoPanel;

	QMenu *m_trayIconMenu;
	QSystemTrayIcon *m_trayIcon;

	QTimer m_refreshTimer;

	static MainWindow *m_mainWindow;

	// Creates all needed menus
	void createMenus();

public slots:
	void failedToAddTorrent(QString errorString);

	void addTorrentAction();
	void exitAction();

	void toggleHideShowTorrentsFilterPanel();
	void toggleHideShowTorrentInfoPanel();

	void aboutAction();

	void toggleHideShow();
	void trayIconActivated(QSystemTrayIcon::ActivationReason reason);

	void torrentFullyDownloaded(Torrent *torrent);

	// Add torrent from url
	void addTorrentFromUrl(QUrl url);
};

#endif // MAINWINDOW_H
