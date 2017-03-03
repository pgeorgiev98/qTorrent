/* qTorrent - An open-source, cross-platform BitTorrent client
 * Copyright (C) 2017 Petko Georgiev
 *
 * trackerclient.cpp
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "bencodeparser.h"
#include "qtorrent.h"
#include "torrent.h"
#include "peer.h"
#include "torrentserver.h"
#include "trackerclient.h"
#include "global.h"
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrlQuery>
#include <QUrl>
#include <QDebug>

TrackerClient::TrackerClient(Torrent* torrent)
	: m_torrent(torrent)
	, m_reply(nullptr)
	, m_reannounceInterval(-1)
	, m_currentAnnounceListIndex(0)
	, m_hasAnnouncedStarted(false)
	, m_numberOfAnnounces(0)
	, m_lastEvent(None)
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
	url.setUrl(currentAnnounceUrl());

	qint64 bytesDownloaded = m_torrent->bytesDownloaded();
	qint64 bytesUploaded = m_torrent->bytesUploaded();
	qint64 bytesLeft = m_torrent->bytesLeft();
	int port = QTorrent::instance()->server()->port();

	QString bytesDownloadedString = QString::number(bytesDownloaded);
	QString bytesUploadedString = QString::number(bytesUploaded);
	QString bytesLeftString = QString::number(bytesLeft);
	QString portString = QString::number(port);

	QUrlQuery query(url);
	auto hash = percentEncode(m_torrent->torrentInfo()->infoHash());
	query.addQueryItem("info_hash", hash);
	query.addQueryItem("peer_id", percentEncode(QTorrent::instance()->peerId()));
	query.addQueryItem("port", portString);
	query.addQueryItem("uploaded", bytesUploadedString);
	query.addQueryItem("downloaded", bytesDownloadedString);
	query.addQueryItem("left", bytesLeftString);
	query.addQueryItem("compact", "1");
	if(event == Event::Started) {
		query.addQueryItem("event", "started");
	} else if(event == Event::Stopped) {
		query.addQueryItem("event", "stopped");
		query.addQueryItem("numwant", "0");
	} else if(event == Event::Completed) {
		query.addQueryItem("event", "completed");
	}

	url.setQuery(query);
	qDebug() << "Announce" << url.toString();

	QNetworkRequest request(url);
	m_reply = m_accessManager.get(request);
	connect(m_reply, &QNetworkReply::finished, this, &TrackerClient::httpFinished);
}

void TrackerClient::httpFinished() {
	m_reply->disconnect();
	m_reply->deleteLater();

	// Check for errors
	if(m_reply->error()) {
		qDebug() << "Error in httpFinished():" << m_reply->errorString();
		announceFailed();
		return;
	}

	// Get HTTP status code
	QVariant statusCodeVariant = m_reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
	if(statusCodeVariant.isValid()) {
		int statusCode = statusCodeVariant.toInt();
		if(statusCode != 200) {
			if(statusCode >= 300 && statusCode < 400) {
				// Redirect
				QUrl redirectUrl = m_reply->attribute(QNetworkRequest::RedirectionTargetAttribute).toUrl();
				if(redirectUrl.isEmpty()) {
					qDebug() << "Redirect URL is empty";
				} else {
					qDebug() << "Redirecting to" << redirectUrl;
					m_reply = m_accessManager.get(QNetworkRequest(redirectUrl));
					connect(m_reply, &QNetworkReply::finished, this, &TrackerClient::httpFinished);
					return;
				}
			} else {
				QString reason = m_reply->attribute(QNetworkRequest::HttpReasonPhraseAttribute).toString();
				qDebug() << "Error: Status code" << statusCode << ":" << reason;
			}
			announceFailed();
			return;
		}
	}

	QByteArray announceReply = m_reply->readAll();
	BencodeParser bencodeParser;

	try {
		// No statement can catch the ChuckNorrisException
		BencodeException ex("TrackerClient::httpFinished(): ");

		// Try to parse
		if(!bencodeParser.parse(announceReply)) {
			throw ex << "Parse failed" << endl << bencodeParser.errorString();
		}

		// Main list of the response
		QList<BencodeValue*> responseMainList = bencodeParser.list();
		if(responseMainList.isEmpty()) {
			throw ex << "Tracker sent an empty response";
		} else if(responseMainList.size() > 1) {
			throw ex << "Tracker response main list has a size of " << responseMainList.size() << ". Expected 1";
		}

		BencodeDictionary* mainDict = responseMainList.first()->toBencodeDictionary();

		// Check if any errors have occured
		if(mainDict->keyExists("failure reason")) {
			qDebug() << "Error: Failure reason: " << mainDict->value("failure reason")->toByteArray();
			announceFailed();
			return;
		}

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
				 << announceReply << endl
				 << "<<<<<<<<<<<<<<<<<<<<";
		announceFailed();
		return;
	}
	announceSucceeded();
}

bool TrackerClient::nextAnnounceUrl() {
	const QList<QByteArray>& list = m_torrent->torrentInfo()->announceUrlsList();
	m_currentAnnounceListIndex++;
	if(m_currentAnnounceListIndex == list.size()) {
		m_currentAnnounceListIndex = 0;
		return false;
	}
	return true;
}

const QByteArray& TrackerClient::currentAnnounceUrl() const {
	auto& list = m_torrent->torrentInfo()->announceUrlsList();
	return list[m_currentAnnounceListIndex];
}

void TrackerClient::resetCurrentAnnounceUrl() {
	m_currentAnnounceListIndex = 0;
}

void TrackerClient::announceFailed() {
	if(nextAnnounceUrl()) {
		announce(m_lastEvent);
	} else {
		resetCurrentAnnounceUrl();
		qDebug() << "No more backup URLs";
	}
}

void TrackerClient::announceSucceeded() {
	resetCurrentAnnounceUrl();
	m_numberOfAnnounces++;
	if(m_lastEvent == Started) {
		m_hasAnnouncedStarted = true;
	}
	m_torrent->successfullyAnnounced(m_lastEvent);
}

int TrackerClient::numberOfAnnounces() const {
	return m_numberOfAnnounces;
}

bool TrackerClient::hasAnnouncedStarted() const {
	return m_hasAnnouncedStarted;
}
