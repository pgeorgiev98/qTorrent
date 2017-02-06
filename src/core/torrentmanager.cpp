#include "torrentmanager.h"
#include "torrentinfo.h"
#include "torrent.h"
#include "resumeinfo.h"
#include "bencodeparser.h"
#include "ui/mainwindow.h"
#include "qtorrent.h"
#include <QDir>
#include <QStandardPaths>

TorrentManager::TorrentManager(QTorrent *qTorrent)
	: m_qTorrent(qTorrent)
{
}

TorrentManager::~TorrentManager() {
	for(Torrent* torrent : m_torrents) {
		delete torrent;
	}
}

Torrent* TorrentManager::addTorrentFromLocalFile(const QString& filename, const QString& downloadLocation) {
	// Load the torrent file
	TorrentInfo* torrentInfo = new TorrentInfo;
	if(!torrentInfo->loadFromTorrentFile(filename)) {
		m_qTorrent->warning("Invalid torrent file");
		delete torrentInfo;
		return nullptr;
	}

	// Check if torrent already added to list
	for(Torrent* t : m_torrents) {
		if(t->torrentInfo()->infoHash() == torrentInfo->infoHash()) {
			m_qTorrent->warning("The torrent you're trying to add is already in the torrents list.");
			delete torrentInfo;
			return nullptr;
		}
	}

	// Create the torrent
	Torrent* torrent = new Torrent(m_qTorrent);
	if(!torrent->createNew(torrentInfo, downloadLocation)) {
		m_qTorrent->warning(torrent->errorString());
		delete torrent;
		return nullptr;
	}

	// Save the torrent
	if(!saveTorrentFile(filename, torrentInfo)) {
		m_qTorrent->warning("Failed to save the torrent");
		delete torrent;
		return nullptr;
	}

	m_torrents.push_back(torrent);
	m_qTorrent->mainWindow()->addTorrent(torrent);

	torrent->start();

	return torrent;
}

bool TorrentManager::resumeTorrents() {
	QString dataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
	QDir dir(dataPath);
	if(!dir.exists()) {
		if(!dir.mkpath(dataPath)) {
			return false;
		}
	}
	if(!dir.exists("resume")) {
		if(!dir.mkdir("resume")) {
			return false;
		}
	}
	dir.cd("resume");
	QFile resumeFile(dir.path() + "/resume.dat");
	if(!resumeFile.exists()) {
		return false;
	}
	if(!resumeFile.open(QIODevice::ReadOnly)) {
		m_qTorrent->warning("Failed to open resume file: " + resumeFile.errorString());
		return false;
	}
	QByteArray resumeData = resumeFile.readAll();
	BencodeParser parser;
	try {
		if(!parser.parse(resumeData)) {
			throw BencodeException(parser.errorString());
		}
		if(parser.list().isEmpty()) {
			throw BencodeException("Main bencode list is empty");
		}
		if(parser.list().size() != 1) {
			throw BencodeException("Main bencode list has size of "
								   + QString::number(parser.list().size()) + ", expected 1");
		}

		BencodeDictionary* mainDict = parser.list().first()->toBencodeDictionary();
		for(BencodeValue* key : mainDict->keys()) {
			QByteArray infoHash = key->toByteArray();
			QFile file(dir.path() + "/" + infoHash.toHex() + ".torrent");
			if(!file.exists()) {
				m_qTorrent->critical("Failed to resume torrent: file " + file.fileName() + " does not exist");
				continue;
			}

			TorrentInfo* torrentInfo = new TorrentInfo;
			if(!torrentInfo->loadFromTorrentFile(file.fileName())) {
				m_qTorrent->critical("Failed to parse resume data while loading "
									 + file.fileName() + ": " + torrentInfo->errorString());
				delete torrentInfo;
				continue;
			}

			ResumeInfo resumeInfo(torrentInfo);

			BencodeValue* value = mainDict->value(infoHash);
			if(!value->isDictionary()) {
				m_qTorrent->critical("Failed to parse resume data while loading "
									 + file.fileName() + ": value for infohash is not a dictionary");
				delete torrentInfo;
				continue;
			}
			if(!resumeInfo.loadFromBencode(value->toBencodeDictionary())) {
				delete torrentInfo;
				m_qTorrent->critical("Failed to load resume data while loading"
									 + file.fileName());
				continue;
			}

			Torrent* torrent = new Torrent(m_qTorrent);
			if(!torrent->createFromResumeInfo(torrentInfo, &resumeInfo)) {
				m_qTorrent->critical("Failed to load torrent from resume data: " + torrent->errorString());
				delete torrent;
				continue;
			}

			m_torrents.push_back(torrent);
			m_qTorrent->mainWindow()->addTorrent(torrent);

		}

	} catch(BencodeException& ex) {
		m_qTorrent->critical("Failed to read resume.dat file: " + ex.what());
		return false;
	}

	return true;
}

Torrent* TorrentManager::addTorrentFromMagnetLink(QUrl url) {
	Torrent* torrent = new Torrent(m_qTorrent);
	if(!torrent->createFromMagnetLink(url)) {
		m_qTorrent->warning("Failed to load torrent from magnet link\n" + torrent->errorString());
		delete torrent;
		return nullptr;
	}

	m_torrents.push_back(torrent);
	m_qTorrent->mainWindow()->addTorrent(torrent);

	torrent->start();

	return torrent;
}

bool TorrentManager::saveTorrentsResumeInfo() {
	QString dataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
	QDir dir(dataPath);
	if(!dir.exists()) {
		if(!dir.mkpath(dataPath)) {
			return false;
		}
	}
	if(!dir.exists("resume")) {
		if(!dir.mkdir("resume")) {
			return false;
		}
	}
	dir.cd("resume");
	QFile resumeFile(dir.path() + "/resume.dat");
	if(!resumeFile.open(QIODevice::WriteOnly)) {
		m_qTorrent->critical("Failed to open resume.dat file: " + resumeFile.errorString());
		return false;
	}
	BencodeDictionary* mainDict = new BencodeDictionary;
	for(Torrent* torrent : m_torrents) {
		torrent->getResumeInfo().addToBencode(mainDict);
	}
	QByteArray bencodedData = mainDict->bencode(true);
	resumeFile.write(bencodedData);
	resumeFile.close();
	delete mainDict;
	return true;
}

bool TorrentManager::saveTorrentFile(const QString &filename, TorrentInfo *torrentInfo) {
	QString dataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
	QDir dir(dataPath);
	if(!dir.exists()) {
		if(!dir.mkpath(dataPath)) {
			return false;
		}
	}
	if(!dir.exists("resume")) {
		if(!dir.mkdir("resume")) {
			return false;
		}
	}
	dir.cd("resume");

	QString newTorrentPath = dir.absoluteFilePath(torrentInfo->infoHash().toHex() + ".torrent");
	if(QFile::exists(newTorrentPath)) {
		if(!QFile::remove(newTorrentPath)) {
			return false;
		}
	}

	if(!QFile::copy(filename, newTorrentPath)) {
		return false;
	}

	return true;
}

QTorrent* TorrentManager::qTorrent() {
	return m_qTorrent;
}

const QList<Torrent*>& TorrentManager::torrents() const {
	return m_torrents;
}
