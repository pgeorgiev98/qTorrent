/* qTorrent - An open-source, cross-platform BitTorrent client
 * Copyright (C) 2017 Petko Georgiev
 *
 * torrentmanager.cpp
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

#include "torrentmanager.h"
#include "torrentinfo.h"
#include "torrent.h"
#include "resumeinfo.h"
#include "bencodeparser.h"
#include "ui/mainwindow.h"
#include "qtorrent.h"
#include <QDir>
#include <QStandardPaths>

TorrentManager::TorrentManager()
{
}

TorrentManager::~TorrentManager() {
	for(Torrent* torrent : m_torrents) {
		delete torrent;
	}
}

Torrent* TorrentManager::addTorrentFromLocalFile(const QString& filename, const TorrentSettings& settings) {
	// Load the torrent file
	TorrentInfo* torrentInfo = new TorrentInfo;
	if(!torrentInfo->loadFromTorrentFile(filename)) {
		QTorrent::instance()->warning("Invalid torrent file");
		delete torrentInfo;
		return nullptr;
	}

	// Check if torrent already added to list
	for(Torrent* t : m_torrents) {
		if(t->torrentInfo()->infoHash() == torrentInfo->infoHash()) {
			QTorrent::instance()->warning("The torrent you're trying to add is already in the torrents list.");
			delete torrentInfo;
			return nullptr;
		}
	}

	// Create the torrent
	Torrent* torrent = new Torrent();
	if(!torrent->createNew(torrentInfo, settings.downloadLocation())) {
		QTorrent::instance()->warning(torrent->errorString());
		delete torrent;
		return nullptr;
	}

	// Save the torrent
	if(!saveTorrentFile(filename, torrentInfo)) {
		QTorrent::instance()->warning("Failed to save the torrent");
		delete torrent;
		return nullptr;
	}

	m_torrents.push_back(torrent);
	QTorrent::instance()->mainWindow()->addTorrent(torrent);

	if(settings.startImmediately()) {
		torrent->start();
	} else {
		torrent->pause();
	}

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
		QTorrent::instance()->warning("Failed to open resume file: " + resumeFile.errorString());
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
		for(QByteArray infoHash : mainDict->keys()) {
			QFile file(dir.path() + "/" + infoHash.toHex() + ".torrent");
			if(!file.exists()) {
				QTorrent::instance()->critical("Failed to resume torrent: file " + file.fileName() + " does not exist");
				continue;
			}

			TorrentInfo* torrentInfo = new TorrentInfo;
			if(!torrentInfo->loadFromTorrentFile(file.fileName())) {
				QTorrent::instance()->critical("Failed to parse resume data while loading "
									 + file.fileName() + ": " + torrentInfo->errorString());
				delete torrentInfo;
				continue;
			}

			ResumeInfo resumeInfo(torrentInfo);

			BencodeValue* value = mainDict->value(infoHash);
			if(!value->isDictionary()) {
				QTorrent::instance()->critical("Failed to parse resume data while loading "
									 + file.fileName() + ": value for infohash is not a dictionary");
				delete torrentInfo;
				continue;
			}
			if(!resumeInfo.loadFromBencode(value->toBencodeDictionary())) {
				delete torrentInfo;
				QTorrent::instance()->critical("Failed to load resume data while loading"
									 + file.fileName());
				continue;
			}

			Torrent* torrent = new Torrent();
			if(!torrent->createFromResumeInfo(torrentInfo, &resumeInfo)) {
				QTorrent::instance()->critical("Failed to load torrent from resume data: " + torrent->errorString());
				delete torrent;
				continue;
			}

			m_torrents.push_back(torrent);
			QTorrent::instance()->mainWindow()->addTorrent(torrent);

		}

	} catch(BencodeException& ex) {
		QTorrent::instance()->critical("Failed to read resume.dat file: " + ex.what());
		return false;
	}

	return true;
}

Torrent* TorrentManager::addTorrentFromMagnetLink(QUrl url) {
	Torrent* torrent = new Torrent();
	if(!torrent->createFromMagnetLink(url)) {
		QTorrent::instance()->warning("Failed to load torrent from magnet link\n" + torrent->errorString());
		delete torrent;
		return nullptr;
	}

	m_torrents.push_back(torrent);
	QTorrent::instance()->mainWindow()->addTorrent(torrent);

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
		QTorrent::instance()->critical("Failed to open resume.dat file: " + resumeFile.errorString());
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

bool TorrentManager::removeTorrent(Torrent* torrent, bool deleteData) {
	QString dataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
	QFile savedTorrentFile(dataPath + "/resume/" + torrent->torrentInfo()->infoHash().toHex() + ".torrent");
	if(savedTorrentFile.exists()) {
		savedTorrentFile.remove();
	}
	m_torrents.removeAll(torrent);
	if(deleteData) {
		for(QFile* file : torrent->files()) {
			if(file->exists()) {
				file->remove();
			}
		}
	}
	torrent->trackerClient()->announce(TrackerClient::Stopped);
	QTorrent::instance()->mainWindow()->removeTorrent(torrent);
	delete torrent;
	return true;
}

const QList<Torrent*>& TorrentManager::torrents() const {
	return m_torrents;
}
