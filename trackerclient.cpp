#include "trackerclient.h"
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrlQuery>
#include <QUrl>

TrackerClient::TrackerClient() {
}

TrackerClient::~TrackerClient() {
}

void TrackerClient::fetchPeerList(TorrentInfo &torrentInfo) {
	QUrl url;
	url.setUrl(torrentInfo.announceUrl());

	QUrlQuery query(url);
	auto hash = torrentInfo.infoHashPercentEncoded();
	query.addQueryItem("info_hash", hash);
	query.addQueryItem("peer_id", "ASEDRFGYQIWKSJDUEYTF");
	query.addQueryItem("port", "6881");
	query.addQueryItem("uploaded", "0");
	query.addQueryItem("downloaded", "0");
	query.addQueryItem("left", QString::number(torrentInfo.length()));
	query.addQueryItem("event", "started");
	/* TODO: Use non-hardcoded values */

	url.setQuery(query);
	QNetworkRequest request(url);
	m_reply = m_accessManager.get(request);
	connect(m_reply, &QNetworkReply::finished, this, &TrackerClient::httpFinished);
	connect(m_reply, &QIODevice::readyRead, this, &TrackerClient::httpReadyRead);
}

void TrackerClient::httpFinished() {
	QTextStream err(stderr);
	if(m_reply->error()) {
		err << endl << "Error:" << m_reply->errorString() << endl;
	} else {
		err << endl << "Successfull" << endl;
	}
}

void TrackerClient::httpReadyRead() {
	QTextStream out(stdout);
	out << m_reply->readAll();
}
