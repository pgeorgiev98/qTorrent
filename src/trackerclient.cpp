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
	m_torrentInfo(torrentInfo),
	m_bytesDownloadedAtStarted(-1),
	m_bytesUploadedAtStarted(-1)
{
	connect(&m_updatePeerListTimer, SIGNAL(timeout()), this, SLOT(updatePeerListTimeoutSlot()));
}

TrackerClient::~TrackerClient() {
}

void TrackerClient::updatePeerListTimeoutSlot() {
	fetchPeerList(Event::None);
}

void TrackerClient::fetchPeerList(Event event) {
	QUrl url;
	url.setUrl(m_torrentInfo->announceUrl());

	qint64 bytesDownloaded = m_torrent->bytesDownloaded();
	qint64 bytesUploaded = m_torrent->bytesUploaded();
	qint64 torrentLength = m_torrentInfo->length();

	if(event == Event::Started) {
		m_bytesDownloadedAtStarted = bytesDownloaded;
		m_bytesUploadedAtStarted = bytesUploaded;
	}

	QString bytesDownloadedString = QString::number(bytesDownloaded - m_bytesDownloadedAtStarted);
	QString bytesUploadedString = QString::number(bytesUploaded - m_bytesUploadedAtStarted);
	QString bytesLeftString = QString::number(torrentLength - bytesDownloaded);

	QUrlQuery query(url);
	auto hash = m_torrentInfo->infoHashPercentEncoded();
	query.addQueryItem("info_hash", hash);
	query.addQueryItem("peer_id", "ThisIsNotAFakePeerId");
	query.addQueryItem("port", "6881");
	query.addQueryItem("uploaded", bytesUploadedString);
	query.addQueryItem("downloaded", bytesDownloadedString);
	query.addQueryItem("left", bytesLeftString);
	query.addQueryItem("compact", "1");
	if(event == Event::Started) {
		query.addQueryItem("event", "started");
	} else if(event == Event::Stopped) {
		query.addQueryItem("event", "stopped");
	} else if(event == Event::Completed) {
		query.addQueryItem("event", "completed");
	}
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
		err << "Failed to parse reply: " << bencodeParser->errorString() << endl;
		err << m_peerListData << endl;
		return;
	}
	const auto& values = bencodeParser->values();
	if(values.size() != 1) {
		err << "Bencode must have a size of 1" << endl;
		return;
	}
	try {
		auto mainDict = values[0]->castToEx<BencodeDictionary>();

		// Update interval
		m_interval = mainDict->valueEx("interval")->castToEx<BencodeInteger>()->value();
		m_updatePeerListTimer.setInterval(m_interval);
		m_updatePeerListTimer.start();

		// Peer list
		QByteArray peersData = mainDict->valueEx("peers")->castToEx<BencodeString>()->value();
		if(peersData.size() % 6 != 0) {
			err << "Tracker response parse error: peers string length is not a multiple of 6; length = " << peersData.size() << endl;
			return;
		}
		int numberOfPeers = peersData.size() / 6;
		for(int i = 0, counter = 0; i < numberOfPeers; i++)  {
			// Address
			QByteArray peerIp;
			peerIp += QString::number((unsigned char)peersData[counter++]);
			peerIp += '.';
			peerIp += QString::number((unsigned char)peersData[counter++]);
			peerIp += '.';
			peerIp += QString::number((unsigned char)peersData[counter++]);
			peerIp += '.';
			peerIp += QString::number((unsigned char)peersData[counter++]);

			// Port
			int peerPort = 0;
			peerPort += (unsigned char)peersData[counter++];
			peerPort *= 256;
			peerPort += (unsigned char)peersData[counter++];
			err << "Peer " << peerIp << ":" << peerPort << endl;
			m_torrent->addPeer(peerIp, peerPort);
		}
		if(m_torrent->peers().isEmpty()) {
			err << "No peers" << endl;
		} else {
			for(auto peer : m_torrent->peers()) {
				peer->startConnection();
			}
		}
	} catch(BencodeException& ex) {
		err << "Failed to parse: " << ex.what() << endl;
		err << m_peerListData << endl;
		return;
	}
}

void TrackerClient::httpReadyRead() {
	m_peerListData.push_back(m_reply->readAll());
}
