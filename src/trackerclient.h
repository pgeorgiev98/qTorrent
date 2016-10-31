#ifndef TRACKERCLIENT_H
#define TRACKERCLIENT_H

#include "torrentinfo.h"
#include <QNetworkAccessManager>
#include <QTimer>

class Bencode;
class Torrent;

class TrackerClient : public QObject {
public:
	enum Event {
		Started, Stopped, Completed, None
	};
	TrackerClient(Torrent* torrent, TorrentInfo* torrentInfo);
	~TrackerClient();
	void fetchPeerList(Event event);
public slots:
	void httpFinished();
	void httpReadyRead();
	void updatePeerListTimeoutSlot();
private:
	Q_OBJECT

	Torrent* m_torrent;
	QNetworkAccessManager m_accessManager;
	QNetworkReply *m_reply;
	TorrentInfo* m_torrentInfo;
	QByteArray m_peerListData;

	// Bytes uploaded and downloaded at the
	// moment 'started' message was sent
	qint64 m_bytesDownloadedAtStarted;
	qint64 m_bytesUploadedAtStarted;

	// Update Peer list
	int m_interval;
	QTimer m_updatePeerListTimer;
signals:
	void bencodePeerListParsed(Bencode* bencode);
};

#endif // TRACKERCLIENT_H
