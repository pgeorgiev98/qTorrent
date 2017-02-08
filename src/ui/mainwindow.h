#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QUrl>

class QTorrent;
class Panel;
class TorrentsList;
class Torrent;

class MainWindow : public QMainWindow {
	Q_OBJECT
public:
	MainWindow(QTorrent* qTorrent);
	~MainWindow();

	QTorrent* qTorrent();
	Panel* panel();
	TorrentsList* torrentsList();

	// Add and remove torrent from list
	void addTorrent(Torrent* torrent);
	void removeTorrent(Torrent* torrent);

	void closeEvent(QCloseEvent *event);

	QString getDownloadLocation();

private:
	QTorrent* m_qTorrent;

	Panel* m_panel;
	TorrentsList* m_torrentsList;

	// Creates all needed menus
	void createMenus();

signals:

public slots:
	void addTorrentAction();
	// Add torrent from url
	void addTorrentFromUrl(QUrl url);
};

#endif // MAINWINDOW_H
