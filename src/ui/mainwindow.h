#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include <QUrl>

class Panel;
class TorrentsList;
class TorrentInfoPanel;
class Torrent;

class MainWindow : public QMainWindow {
	Q_OBJECT
public:
	MainWindow();
	~MainWindow();

	Panel* panel();
	TorrentsList* torrentsList();

	// Add and remove torrent from list
	void addTorrent(Torrent* torrent);
	void removeTorrent(Torrent* torrent);

	void closeEvent(QCloseEvent *event);

	QString getDownloadLocation();

private:
	Panel* m_panel;
	TorrentsList* m_torrentsList;
	TorrentInfoPanel* m_statusPanel;

	QTimer m_refreshTimer;

	// Creates all needed menus
	void createMenus();

signals:

public slots:
	void addTorrentAction();
	// Add torrent from url
	void addTorrentFromUrl(QUrl url);
};

#endif // MAINWINDOW_H
