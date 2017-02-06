#ifndef QTORRENT_H
#define QTORRENT_H

#include "core/torrentsettings.h"
#include <QList>
#include <QString>
#include <QUrl>

class Torrent;
class TorrentManager;
class TorrentServer;
class MainWindow;

class QTorrent {
public:
	QTorrent();
	~QTorrent();

	bool startServer();
	bool resumeTorrents();
	bool addTorrentFromLocalFile(const QString& filename, const TorrentSettings& settings);
	bool addTorrentFromMagnetLink(QUrl url);
	bool addTorrentFromUrl(QUrl url);

	void shutDown();

	void showMainWindow();

	/* Opens a critical MessageBox */
	void critical(const QString& text);
	/* Opens an information MessageBox */
	void information(const QString& text);
	/* Opens question MessageBox. Returns true on 'yes' */
	bool question(const QString& text);
	/* Opens a warning MessageBox */
	void warning(const QString& text);

	const QByteArray& peerId() const;
	QByteArray peerIdPercentEncoded() const;
	const QList<Torrent*>& torrents() const;
	TorrentManager* torrentManager();
	TorrentServer* server();
	MainWindow* mainWindow();

private:
	QByteArray m_peerId;

	TorrentManager* m_torrentManager;
	TorrentServer* m_server;

	MainWindow* m_mainWindow;
};

#endif // QTORRENT_H
