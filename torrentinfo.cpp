#include "torrentinfo.h"
#include "bencode.h"
#include <QFile>
#include <QString>
#include <QCryptographicHash>
#include <QDebug>

void TorrentInfo::clearError() {
	m_errorString.clear();
}

void TorrentInfo::setError(QString errorString) {
	m_errorString = errorString;
}

QString TorrentInfo::errorString() const {
	return m_errorString;
}

TorrentInfo::TorrentInfo() {
}

TorrentInfo::~TorrentInfo() {
}

bool TorrentInfo::loadTorrentFile(QString filename) {
	Bencode bencodeParser;
	if(!bencodeParser.loadFromFile(filename)) {
		setError("Failed to open parse file " + filename + ": " + bencodeParser.errorString());
		return false;
	}
	try {
		auto mainDict = bencodeParser.getValueEx(0)->castToEx<BencodeDictionary>();
		auto infoDict = mainDict->valueEx("info")->castToEx<BencodeDictionary>();

		m_announceUrl = mainDict->valueEx("announce")->castToEx<BencodeString>()->value();
		m_creationDate = QDateTime::fromMSecsSinceEpoch(1000*mainDict->valueEx("creation date")->castToEx<BencodeInteger>()->value());
		m_encoding = mainDict->valueEx("encoding")->castToEx<BencodeString>()->value();

		m_torrentName = infoDict->valueEx("name")->castToEx<BencodeString>()->value();
		m_pieceLength = infoDict->valueEx("piece length")->castToEx<BencodeInteger>()->value();
		m_pieces = infoDict->valueEx("pieces")->castToEx<BencodeString>()->value();

		if(infoDict->keyExists("length")) {
			// Single file torrent
			m_length = infoDict->valueEx("length")->castToEx<BencodeInteger>()->value();
			FileInfo fileInfo;
			fileInfo.length = m_length;
			fileInfo.path = m_torrentName;
		} else {
			// Multi file torrent
			m_length = 0;
			auto filesList = infoDict->valueEx("files")->castToEx<BencodeList>();
			for(auto file : filesList->values<BencodeDictionary>()) {
				FileInfo fileInfo;
				fileInfo.length = file->valueEx("length")->castToEx<BencodeInteger>()->value();
				auto pathList = file->valueEx("path")->castToEx<BencodeList>();
				fileInfo.path = pathList->getValueEx(0)->castToEx<BencodeString>()->value();
				m_length += fileInfo.length;
			}
		}

		m_infoHash = QCryptographicHash::hash(infoDict->getBencodeData(), QCryptographicHash::Sha1);

		m_numberOfPieces = m_length / m_pieceLength;
		if(m_length % m_pieceLength != 0) {
			m_numberOfPieces++;
		}
	} catch(BencodeException& ex) {
		setError(ex.what());
		return false;
	}
	return true;
}


const QByteArray& TorrentInfo::announceUrl() const {
	return m_announceUrl;
}

QDateTime TorrentInfo::creationDate() const {
	return m_creationDate;
}

QString TorrentInfo::encoding() const {
	return m_encoding;
}

qint64 TorrentInfo::length() const {
	return m_length;
}

const QByteArray& TorrentInfo::torrentName() const {
	return m_torrentName;
}

qint64 TorrentInfo::pieceLength() const {
	return m_pieceLength;
}

const QByteArray& TorrentInfo::pieces() const {
	return m_pieces;
}

const QByteArray& TorrentInfo::infoHash() const {
	return m_infoHash;
}

QByteArray TorrentInfo::infoHashPercentEncoded() const {
	QByteArray encoded;
	for(char b : m_infoHash) {
		encoded += '%';
		encoded += QByteArray::number(b, 16).right(2).rightJustified(2, '0');
	}
	return encoded;
}

int TorrentInfo::numberOfPieces() const {
	return m_numberOfPieces;
}
