#ifndef TRACKERCLIENT_H
#define TRACKERCLIENT_H

#include "torrentinfo.h"
#include <QNetworkAccessManager>

class TrackerClient : public QObject {
	Q_OBJECT

	QNetworkAccessManager m_accessManager;
	QNetworkReply *m_reply;
public slots:
	void httpFinished();
	void httpReadyRead();
public:
	TrackerClient();
	~TrackerClient();
	void fetchPeerList(TorrentInfo& torrentInfo);
};

#endif // TRACKERCLIENT_H
