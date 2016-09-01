#ifndef TRACKERCLIENT_H
#define TRACKERCLIENT_H

#include "torrentinfo.h"
#include <QNetworkAccessManager>

class TrackerClient : public QObject {
	Q_OBJECT

	QNetworkAccessManager m_accessManager;
	QNetworkReply *m_reply;
	TorrentInfo* m_torrentInfo;
public slots:
	void httpFinished();
	void httpReadyRead();
public:
	TrackerClient(TorrentInfo* torrentInfo);
	~TrackerClient();
	void fetchPeerList();
};

#endif // TRACKERCLIENT_H
