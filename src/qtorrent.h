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

	/* Opens a critical MessageBox */
	void critical(const QString& text);
	/* Opens an information MessageBox */
	void information(const QString& text);
	/* Opens question MessageBox. Returns true on 'yes' */
	bool question(const QString& text);
	/* Opens a warning MessageBox */
	void warning(const QString& text);

	QList<Torrent*>& torrents();
	TorrentServer* server();
	MainWindow* mainWindow();

private:
	QList<Torrent*> m_torrents;
	TorrentServer* m_server;

	MainWindow* m_mainWindow;
};

#endif // QTORRENT_H
