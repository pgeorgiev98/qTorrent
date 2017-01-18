#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

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

private:
	QTorrent* m_qTorrent;

	Panel* m_panel;
	TorrentsList* m_torrentsList;

	// Used to add a new torrent
	QAction* m_addTorrentAction;

	// Creates all needed menus
	void createMenus();

signals:

public slots:
	// Calledd when m_addTorrentAction is clicked
	void addTorrentAction();
};

#endif // MAINWINDOW_H
