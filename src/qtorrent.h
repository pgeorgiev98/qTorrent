#ifndef QTORRENT_H
#define QTORRENT_H

#include <QList>
#include <QString>

class Torrent;
class TorrentServer;
class MainWindow;

class QTorrent {
public:
	QTorrent();
	~QTorrent();

	bool startServer();
	bool addTorrent(const QString& filename);

	void showMainWindow();

	QList<Torrent*>& torrents();
	TorrentServer* server();
	MainWindow* mainWindow();

private:
	QList<Torrent*> m_torrents;
	TorrentServer* m_server;

	MainWindow* m_mainWindow;
};

#endif // QTORRENT_H
