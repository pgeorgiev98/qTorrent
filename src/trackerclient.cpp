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
#include <QDebug>

TrackerClient::TrackerClient(Torrent* torrent, TorrentInfo* torrentInfo) :
	m_torrent(torrent),
	m_torrentInfo(torrentInfo),
	m_bytesDownloadedAtStarted(-1),
	m_bytesUploadedAtStarted(-1),
	m_urlListCurrentIndex(0)
{
	connect(&m_updatePeerListTimer, SIGNAL(timeout()), this, SLOT(updatePeerListTimeoutSlot()));
}

TrackerClient::~TrackerClient() {
}

void TrackerClient::updatePeerListTimeoutSlot() {
	announce(Event::None);
}

void TrackerClient::announce(Event event) {
	m_lastEvent = event;
	QUrl url;
	auto announceUrls = m_torrentInfo->announceUrlsList();
	if(m_urlListCurrentIndex >= announceUrls.size()) {
		// No more backup urls
		m_urlListCurrentIndex = 0;
	}
	url.setUrl(announceUrls[m_urlListCurrentIndex]);

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

	// Check for errors
	if(m_reply->error()) {
		qDebug() << "Error in httpFinished():" << m_reply->errorString();
		failedToConnect();
		return;
	}

	// Get HTTP status code
	QVariant statusCodeVariant = m_reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
	if(statusCodeVariant.isValid()) {
		int statusCode = statusCodeVariant.toInt();
		if(statusCode != 200) {
			if(statusCode == 301) {
				// Moved permanently
				qDebug() << "Moved Permanently";
				QUrl redirectUrl = m_reply->attribute(QNetworkRequest::RedirectionTargetAttribute).toUrl();
				if(redirectUrl.isEmpty()) {
					qDebug() << "Redirect URL is empty";
				} else {
					qDebug() << "Redirecting to" << redirectUrl;
					m_peerListData.clear();
					m_reply = m_accessManager.get(QNetworkRequest(redirectUrl));
					connect(m_reply, &QNetworkReply::finished, this, &TrackerClient::httpFinished);
					connect(m_reply, &QIODevice::readyRead, this, &TrackerClient::httpReadyRead);
					return;
				}
			} else {
				QString reason = m_reply->attribute(QNetworkRequest::HttpReasonPhraseAttribute).toString();
				qDebug() << "Error: Status code" << statusCode << ":" << reason;
			}
			failedToConnect();
			return;
		}
	}

	QTextStream err(stderr);
	Bencode* bencodeParser = new Bencode();
	if(!bencodeParser->loadFromByteArray(m_peerListData)) {
		err << "Failed to parse reply: " << bencodeParser->errorString() << endl;
		err << m_peerListData << endl;
		failedToConnect();
		return;
	}
	const auto& values = bencodeParser->values();
	if(values.size() != 1) {
		err << "Bencode must have a size of 1" << endl;
		failedToConnect();
		return;
	}
	try {
		auto mainDict = values[0]->castToEx<BencodeDictionary>();

		// Update interval
		m_interval = mainDict->valueEx("interval")->castToEx<BencodeInteger>()->value();
		m_updatePeerListTimer.setInterval(m_interval*1000);
		m_updatePeerListTimer.start();

		// Peer list
		QByteArray peersData = mainDict->valueEx("peers")->castToEx<BencodeString>()->value();
		if(peersData.size() % 6 != 0) {
			err << "Tracker response parse error: peers string length is not a multiple of 6; length = " << peersData.size() << endl;
			failedToConnect();
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
			Peer* peer = m_torrent->addPeer(peerIp, peerPort);
			if(peer != nullptr) {
				peer->startConnection();
			}
		}
		if(m_torrent->peers().isEmpty()) {
			err << "No peers" << endl;
		}
	} catch(BencodeException& ex) {
		err << "Failed to parse: " << ex.what() << endl;
		err << m_peerListData << endl;
		failedToConnect();
		return;
	}
}

void TrackerClient::httpReadyRead() {
	m_peerListData.push_back(m_reply->readAll());
}

void TrackerClient::failedToConnect() {
	auto announceList = m_torrentInfo->announceUrlsList();
	if(m_urlListCurrentIndex + 1 >= announceList.size()) {
		qDebug() << "No more backup URLs";
		return;
	}
	m_urlListCurrentIndex++;
	qDebug() << "Trying backup URL:" << announceList[m_urlListCurrentIndex];
	announce(m_lastEvent);
}
