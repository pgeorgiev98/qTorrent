#include "bencode.h"
#include "torrent.h"
#include "peer.h"
#include "trackerclient.h"
#include "torrentclient.h"
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrlQuery>
#include <QUrl>

TrackerClient::TrackerClient(Torrent* torrent, TorrentInfo* torrentInfo) :
	m_torrent(torrent),
	m_torrentInfo(torrentInfo)
{
}

TrackerClient::~TrackerClient() {
}

void TrackerClient::fetchPeerList() {
	QUrl url;
	url.setUrl(m_torrentInfo->announceUrl());

	QUrlQuery query(url);
	auto hash = m_torrentInfo->infoHashPercentEncoded();
	query.addQueryItem("info_hash", hash);
	query.addQueryItem("peer_id", "ASEDRFGYQIWKSJDUEYTF");
	query.addQueryItem("port", "6881");
	query.addQueryItem("uploaded", "0");
	query.addQueryItem("downloaded", "0");
	query.addQueryItem("left", QString::number(m_torrentInfo->length()));
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
		err << "Error:" << m_reply->errorString() << endl;
		return;
	}
	Bencode* bencodeParser = new Bencode();
	if(!bencodeParser->loadFromByteArray(m_peerListData)) {
		err << "Failed to parse reply" << endl;
		return;
	}
	const auto& values = bencodeParser->values();
	if(values.size() != 1) {
		err << "Bencode must have a size of 1" << endl;
		return;
	}
	try {
		auto mainDict = values[0]->castToEx<BencodeDictionary>();
		auto peersList = mainDict->valueEx("peers")->castToEx<BencodeList>();
		for(auto peerDict : peersList->values<BencodeDictionary>()) {
			QByteArray peerIp = peerDict->valueEx("ip")->castToEx<BencodeString>()->value();
			int peerPort = peerDict->valueEx("port")->castToEx<BencodeInteger>()->value();
			m_torrent->addPeer(new Peer(peerIp, peerPort));
		}
	} catch(BencodeException& ex) {
		err << "Failed to parse: " << ex.what() << endl;
	}
}

void TrackerClient::httpReadyRead() {
	m_peerListData.push_back(m_reply->readAll());
}
