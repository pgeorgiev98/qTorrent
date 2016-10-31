#ifndef TRACKERCLIENT_H
#define TRACKERCLIENT_H

#include "torrentinfo.h"
#include <QNetworkAccessManager>

class Bencode;
class Torrent;

class TrackerClient : public QObject {
	Q_OBJECT

	Torrent* m_torrent;
	QNetworkAccessManager m_accessManager;
	QNetworkReply *m_reply;
	TorrentInfo* m_torrentInfo;
	QByteArray m_peerListData;
public slots:
	void httpFinished();
	void httpReadyRead();
signals:
	void bencodePeerListParsed(Bencode* bencode);
public:
	TrackerClient(Torrent* torrent, TorrentInfo* torrentInfo);
	~TrackerClient();
	void fetchPeerList();
};

#endif // TRACKERCLIENT_H
