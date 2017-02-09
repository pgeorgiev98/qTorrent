#include "core/resumeinfo.h"
#include "core/bencodevalue.h"
#include "core/torrentinfo.h"
#include <QDebug>

ResumeInfo::ResumeInfo(TorrentInfo *torrentInfo)
	: m_torrentInfo(torrentInfo)
	, m_totalBytesDownloaded(0)
	, m_totalBytesUploaded(0)
	, m_paused(false)
{
}


bool ResumeInfo::loadFromBencode(BencodeDictionary *dict) {
	try {
		m_downloadLocation = dict->value("downloadLocation")->toByteArray();
		m_totalBytesDownloaded = dict->value("totalBytesDownloaded")->toInt();
		m_totalBytesUploaded = dict->value("totalBytesUploaded")->toInt();
		m_paused = dict->value("paused")->toInt() ? true : false;
		m_aquiredPieces = toBitArray(dict->value("aquiredPieces")->toByteArray());

	} catch(BencodeException& ex) {
		qDebug() << "Failed to load resume info:" << ex.what();
		return false;
	}
	return true;
}

void ResumeInfo::addToBencode(BencodeDictionary *mainResumeDictionary) const {
	BencodeDictionary* dict = new BencodeDictionary;
	dict->add("downloadLocation", new BencodeString(m_downloadLocation.toUtf8()));
	dict->add("totalBytesDownloaded", new BencodeInteger(m_totalBytesDownloaded));
	dict->add("totalBytesUploaded", new BencodeInteger(m_totalBytesUploaded));
	dict->add("paused", new BencodeInteger(m_paused));
	dict->add("aquiredPieces", new BencodeString(aquiredPiecesArray()));

	mainResumeDictionary->add(m_torrentInfo->infoHash(), dict);
}


QByteArray ResumeInfo::aquiredPiecesArray() const {
	QByteArray ret;
	for(int i = 0; i < m_aquiredPieces.size()/8; i++) {
		unsigned char byte = 0;
		for(int j = 0; j < 8; j++) {
			if(m_aquiredPieces[i*8+j]) {
				byte |= (1 << (7-j));
			}
		}
		ret.push_back(byte);
	}
	if((m_aquiredPieces.size() % 8) != 0) {
		unsigned char byte = 0;
		int bits = m_aquiredPieces.size() % 8;
		int i = m_aquiredPieces.size() - bits;
		for(int j = 0; j < bits; j++) {
			if(m_aquiredPieces[i+j]) {
				byte |= (1 << (7-j));
			}
		}
		ret.push_back(byte);
	}
	return ret;
}

QVector<bool> ResumeInfo::toBitArray(const QByteArray& data) {
	QVector<bool> ret;
	for(unsigned char byte : data) {
		for(int i = 7; i >= 0; i--) {
			ret.push_back((byte & (1 << i)) ? true : false);
		}
	}
	int trailingBits = 8 - (m_torrentInfo->numberOfPieces() % 8);
	if(trailingBits) {
		ret.remove(ret.size() - trailingBits, trailingBits);
	}
	return ret;
}

/* Getters */

TorrentInfo* ResumeInfo::torrentInfo() const {
	return m_torrentInfo;
}

const QString& ResumeInfo::downloadLocation() const {
	return m_downloadLocation;
}

qint64 ResumeInfo::totalBytesDownloaded() const {
	return m_totalBytesDownloaded;
}

qint64 ResumeInfo::totalBytesUploaded() const {
	return m_totalBytesUploaded;
}

bool ResumeInfo::paused() const {
	return m_paused;
}

const QVector<bool>& ResumeInfo::aquiredPieces() const {
	return m_aquiredPieces;
}

/* Setters */

void ResumeInfo::setDownloadLocation(const QString& downloadLocation) {
	m_downloadLocation = downloadLocation;
}

void ResumeInfo::setTotalBytesDownloaded(qint64 totalBytesDownloaded) {
	m_totalBytesDownloaded = totalBytesDownloaded;
}

void ResumeInfo::setTotalBytesUploaded(qint64 totalBytesUploaded) {
	m_totalBytesUploaded = totalBytesUploaded;
}

void ResumeInfo::setPaused(bool paused) {
	m_paused = paused;
}

void ResumeInfo::setAquiredPieces(const QVector<bool>& aquiredPieces) {
	m_aquiredPieces = aquiredPieces;
}
