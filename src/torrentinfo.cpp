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
	if(m_creationDate != nullptr) delete m_creationDate;
	if(m_comment != nullptr) delete m_comment;
	if(m_createdBy != nullptr) delete m_createdBy;
	if(m_encoding != nullptr) delete m_encoding;
}

bool TorrentInfo::loadTorrentFile(QString filename) {
	Bencode bencodeParser;
	if(!bencodeParser.loadFromFile(filename)) {
		setError("Failed to open parse file " + filename + ": " + bencodeParser.errorString());
		return false;
	}
	try {
		/* Required parameters */

		// Main dictionary
		auto mainDict = bencodeParser.getValueEx(0)->castToEx<BencodeDictionary>();

		// The Info dictionary
		auto infoDict = mainDict->valueEx("info")->castToEx<BencodeDictionary>();

		// Announce URL
		try {
			QList<QByteArray> announceUrlsList;
			QList<BencodeList*> announceList = mainDict->valueEx("announce-list")->castToEx<BencodeList>()->values<BencodeList>();
			for(auto announceSubList : announceList) {
				QList<BencodeString*> stringsList = announceSubList->values<BencodeString>();
				for(auto announceUrl : stringsList) {
					// [TODO] Support shuffling
					// http://bittorrent.org/beps/bep_0012.html
					announceUrlsList.push_back(announceUrl->value());
				}
			}
			m_announceUrlsList = announceUrlsList;
		} catch(BencodeException& ex) {
			m_announceUrlsList.clear();
			QByteArray url = mainDict->valueEx("announce")->castToEx<BencodeString>()->value();
			m_announceUrlsList.push_back(url);
		}

		// Torrent name
		m_torrentName = infoDict->valueEx("name")->castToEx<BencodeString>()->value();

		// Piece length
		m_pieceLength = infoDict->valueEx("piece length")->castToEx<BencodeInteger>()->value();

		// SHA-1 hash sums of the pieces
		m_pieces = infoDict->valueEx("pieces")->castToEx<BencodeString>()->value();

		// Information about all files in the torrent
		if(infoDict->keyExists("length")) {
			// Single file torrent
			m_length = infoDict->valueEx("length")->castToEx<BencodeInteger>()->value();
			FileInfo fileInfo;
			fileInfo.length = m_length;
			fileInfo.path = QList<QString>({m_torrentName});
			m_fileInfos.push_back(fileInfo);
		} else {
			// Multi file torrent
			m_length = 0;
			auto filesList = infoDict->valueEx("files")->castToEx<BencodeList>();
			for(auto file : filesList->values<BencodeDictionary>()) {
				FileInfo fileInfo;
				fileInfo.length = file->valueEx("length")->castToEx<BencodeInteger>()->value();
				auto pathList = file->valueEx("path")->castToEx<BencodeList>()->values<BencodeString>();
				fileInfo.path = QList<QString>({m_torrentName});
				for(auto v : pathList) {
					fileInfo.path.push_back(v->value());
				}
				m_length += fileInfo.length;
				m_fileInfos.push_back(fileInfo);
			}
		}

		/* Optional parameters */

		// Creation date
		try {
			m_creationDate = new QDateTime;
			*m_creationDate = QDateTime::fromMSecsSinceEpoch(1000*mainDict->valueEx("creation date")->castToEx<BencodeInteger>()->value());
		} catch(BencodeException& ex) {
			delete m_creationDate;
			m_creationDate = nullptr;
		}

		// Comment
		try {
			m_comment = new QString;
			*m_comment = mainDict->valueEx("comment")->castToEx<BencodeString>()->value();
		} catch(BencodeException& ex) {
			delete m_comment;
			m_comment = nullptr;
		}

		// Created by
		try {
			m_createdBy = new QString;
			*m_createdBy = mainDict->valueEx("created by")->castToEx<BencodeString>()->value();
		} catch(BencodeException& ex) {
			delete m_createdBy;
			m_createdBy = nullptr;
		}

		// Encoding
		try {
			m_encoding = new QString(mainDict->valueEx("encoding")->castToEx<BencodeString>()->value());
		} catch(BencodeException& ex) {
			// Default to UTF-8 encoding
			m_encoding = new QString("UTF-8");
		}

		/* Calculate torrent file info hash */
		m_infoHash = QCryptographicHash::hash(infoDict->getBencodeData(), QCryptographicHash::Sha1);

		/* Calculate total number of pieces */
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


const QList<QByteArray>& TorrentInfo::announceUrlsList() const {
	return m_announceUrlsList;
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

const QList<FileInfo>& TorrentInfo::fileInfos() const {
	return m_fileInfos;
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


const QDateTime* TorrentInfo::creationDate() const {
	return m_creationDate;
}

const QString* TorrentInfo::comment() const {
	return m_comment;
}

const QString* TorrentInfo::createdBy() const {
	return m_createdBy;
}

const QString* TorrentInfo::encoding() const {
	return m_encoding;
}
