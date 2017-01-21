#include "bencodeparser.h"
#include "qtorrent.h"
#include "torrent.h"
#include "peer.h"
#include "torrentserver.h"
#include "trackerclient.h"
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrlQuery>
#include <QUrl>
#include <QDebug>

TrackerClient::TrackerClient(Torrent* torrent, TorrentInfo* torrentInfo) :
	m_torrent(torrent),
	m_torrentInfo(torrentInfo),
	m_reply(nullptr),
	m_bytesDownloadedAtStarted(-1),
	m_bytesUploadedAtStarted(-1),
	m_reannounceInterval(-1),
	m_urlListCurrentIndex(0),
	m_lastEvent(None)
{
	connect(&m_reannounceTimer, SIGNAL(timeout()), this, SLOT(reannounce()));
}

TrackerClient::~TrackerClient() {
}


void TrackerClient::reannounce() {
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
	int port = m_torrent->qTorrent()->server()->port();

	if(event == Event::Started) {
		m_bytesDownloadedAtStarted = bytesDownloaded;
		m_bytesUploadedAtStarted = bytesUploaded;
	}

	QString bytesDownloadedString = QString::number(bytesDownloaded - m_bytesDownloadedAtStarted);
	QString bytesUploadedString = QString::number(bytesUploaded - m_bytesUploadedAtStarted);
	QString bytesLeftString = QString::number(torrentLength - bytesDownloaded);
	QString portString = QString::number(port);

	QUrlQuery query(url);
	auto hash = m_torrentInfo->infoHashPercentEncoded();
	query.addQueryItem("info_hash", hash);
	query.addQueryItem("peer_id", "ThisIsNotAFakePeerId");
	query.addQueryItem("port", portString);
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
	qDebug() << "Announce" << url.toString();

	QNetworkRequest request(url);
	m_announceResponse.clear();
	m_reply = m_accessManager.get(request);
	connect(m_reply, &QNetworkReply::finished, this, &TrackerClient::httpFinished);
	connect(m_reply, &QIODevice::readyRead, this, &TrackerClient::httpReadyRead);
}

void TrackerClient::httpFinished() {
	m_reply->disconnect();
	m_reply->deleteLater();

	// Check for errors
	if(m_reply->error()) {
		qDebug() << "Error in httpFinished():" << m_reply->errorString();
		failedToAnnounce();
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
					m_announceResponse.clear();
					m_reply = m_accessManager.get(QNetworkRequest(redirectUrl));
					connect(m_reply, &QNetworkReply::finished, this, &TrackerClient::httpFinished);
					connect(m_reply, &QIODevice::readyRead, this, &TrackerClient::httpReadyRead);
					return;
				}
			} else {
				QString reason = m_reply->attribute(QNetworkRequest::HttpReasonPhraseAttribute).toString();
				qDebug() << "Error: Status code" << statusCode << ":" << reason;
			}
			failedToAnnounce();
			return;
		}
	}

	BencodeParser bencodeParser;

	try {
		// No statement can catch the ChuckNorrisException
		BencodeException ex("TrackerClient::httpFinished(): ");

		// Try to parse
		if(!bencodeParser.parse(m_announceResponse)) {
			throw ex << "Parse failed" << endl << bencodeParser.errorString();
		}

		// Main list of the response
		QList<BencodeValue*> responseMainList = bencodeParser.toList();
		if(responseMainList.isEmpty()) {
			throw ex << "Tracker sent an empty response";
		} else if(responseMainList.size() > 1) {
			throw ex << "Tracker response main list has a size of " << responseMainList.size() << ". Expected 1";
		}

		BencodeDictionary* mainDict = responseMainList[0]->toBencodeDictionary();

		// Update interval
		m_reannounceInterval = mainDict->value("interval")->toInt();
		m_reannounceTimer.setInterval(m_reannounceInterval*1000);
		m_reannounceTimer.start();

		// Peer list
		BencodeValue* peers = mainDict->value("peers");
		if(peers->isString()) {
			// Compact format
			QByteArray peersData = peers->toByteArray();
			if(peersData.size() % 6 != 0) {
				throw ex << "Peers string length is " << peersData.size() << ". Expected a multiple of 6";
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
				m_torrent->addPeer(peerIp, peerPort);
			}
		} else {
			// Non-compact format
			QList<BencodeValue*> peersDictList = peers->toList();
			for(BencodeValue* peerDictValue : peersDictList) {
				BencodeDictionary* peerDict = peerDictValue->toBencodeDictionary();
				QByteArray ip = peerDict->value("ip")->toByteArray();
				int port = peerDict->value("port")->toInt();
				m_torrent->addPeer(ip, port);
			}
		}
	} catch(BencodeException& ex) {
		qDebug() << "Failed to parse tracker response:" << ex.what() << endl
				 << ">>>>>>>>>>>>>>>>>>>>" << endl
				 << m_announceResponse << endl
				 << "<<<<<<<<<<<<<<<<<<<<";
		failedToAnnounce();
		return;
	}
}

void TrackerClient::httpReadyRead() {
	m_announceResponse.push_back(m_reply->readAll());
}

void TrackerClient::failedToAnnounce() {
	auto announceList = m_torrentInfo->announceUrlsList();
	if(m_urlListCurrentIndex + 1 >= announceList.size()) {
		qDebug() << "No more backup URLs";
		return;
	}
	m_urlListCurrentIndex++;
	qDebug() << "Trying backup URL:" << announceList[m_urlListCurrentIndex];
	announce(m_lastEvent);
}
