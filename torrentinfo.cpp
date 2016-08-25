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

		m_length = infoDict->valueEx("length")->castToEx<BencodeInteger>()->value();
		m_torrentName = infoDict->valueEx("name")->castToEx<BencodeString>()->value();
		m_pieceLength = infoDict->valueEx("piece length")->castToEx<BencodeInteger>()->value();
		m_pieces = infoDict->valueEx("pieces")->castToEx<BencodeString>()->value();

		m_infoHash = QCryptographicHash::hash(infoDict->getBencodeData(), QCryptographicHash::Sha1);
	} catch(BencodeException& ex) {
		setError(ex.what());
		return false;
	}
	return true;
}
