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

TorrentManager *TorrentManager::m_torrentManager = nullptr;


TorrentManager::TorrentManager()
{
	Q_ASSERT(m_torrentManager == nullptr);
	m_torrentManager = this;
}

TorrentManager::~TorrentManager()
{
	for (Torrent *torrent : m_torrents) {
		delete torrent;
	}
}

TorrentManager* TorrentManager::instance()
{
	Q_ASSERT(m_torrentManager != nullptr);
	return m_torrentManager;
}

void TorrentManager::addTorrentFromInfo(TorrentInfo *torrentInfo, const TorrentSettings &settings)
{
	// Check if torrent is already added to the list
	for (Torrent *t : m_torrents) {
		if (t->torrentInfo()->infoHash() == torrentInfo->infoHash()) {
			emit failedToAddTorrent("The torrent you're trying to add is already in the torrents list");
			delete torrentInfo;
			return;
		}
	}

	// Create the torrent
	Torrent *torrent = new Torrent();
	if (!torrent->createNew(torrentInfo, settings.downloadLocation())) {
		emit failedToAddTorrent("Failed to add torrent: " + torrent->errorString());
		torrent->deleteLater();
		return;
	}

	// Save the torrent
	if (!saveTorrentFile(torrentInfo->creationFileName(), torrentInfo)) {
		emit failedToAddTorrent("Failed to save the torrent");
		torrent->deleteLater();
		return;
	}

	m_torrents.push_back(torrent);

	if (!settings.skipHashCheck()) {
		torrent->check();
	}

	if (settings.startImmediately()) {
		torrent->start();
	} else {
		torrent->pause();
	}

	emit torrentAdded(torrent);

	saveTorrentsResumeInfo();
}

void TorrentManager::resumeTorrents()
{
	QString dataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
	QDir dir(dataPath);
	if (!dir.exists()) {
		if (!dir.mkpath(dataPath)) {
			emit failedToResumeTorrents("Failed to create directory " + dataPath);
			return;
		}
	}
	if (!dir.exists("resume")) {
		if (!dir.mkdir("resume")) {
			emit failedToResumeTorrents("Failed to create directory " + dataPath + "/resume");
			return;
		}
	}
	dir.cd("resume");
	QFile resumeFile(dir.path() + "/resume.dat");
	if (!resumeFile.exists()) {
		// No resume file => Nothing to do.
		return;
	}
	if (!resumeFile.open(QIODevice::ReadOnly)) {
		QFileInfo info(resumeFile);
		emit failedToResumeTorrents("Failed to open file " + info.absoluteFilePath() + ": " + resumeFile.errorString());
		return;
	}
	QByteArray resumeData = resumeFile.readAll();
	BencodeParser parser;
	try {
		if (!parser.parse(resumeData)) {
			throw BencodeException(parser.errorString());
		}
		if (parser.list().isEmpty()) {
			throw BencodeException("Main bencode list is empty");
		}
		if (parser.list().size() != 1) {
			throw BencodeException("Main bencode list has size of "
								   + QString::number(parser.list().size()) + ", expected 1");
		}

		BencodeDictionary *mainDict = parser.list().first()->toBencodeDictionary();
		for (QByteArray infoHash : mainDict->keys()) {
			QFile file(dir.path() + "/" + infoHash.toHex() + ".torrent");
			if (!file.exists()) {
				QFileInfo info(file);
				qDebug() << "TorrentManager::resumeTorrents(): file" << info.absoluteFilePath() << "Not found";
				continue;
			}

			TorrentInfo *torrentInfo = new TorrentInfo;
			if (!torrentInfo->loadFromTorrentFile(file.fileName())) {
				qDebug() << "TorrentManager::resumeTorrents(): Failed to parse" << file.fileName()
						 << torrentInfo->errorString();
				delete torrentInfo;
				continue;
			}

			ResumeInfo resumeInfo(torrentInfo);

			BencodeValue *value = mainDict->value(infoHash);
			if (!value->isDictionary()) {
				qDebug() << "TorrentManager::resumeTorrents(): Failed to parse" << file.fileName()
						 << ": value for infohash is not a dictionary";
				delete torrentInfo;
				continue;
			}
			if (!resumeInfo.loadFromBencode(value->toBencodeDictionary())) {
				qDebug() << "TorrentManager::resumeTorrents(): Failed to load resume info for" << file.fileName();
				delete torrentInfo;
				continue;
			}

			Torrent *torrent = new Torrent();
			if (!torrent->createFromResumeInfo(torrentInfo, &resumeInfo)) {
				qDebug() << "TorrentManager::resumeTorrents(): Failed to create torrent from resume data for"
						 << file.fileName() << torrent->errorString();
				torrent->deleteLater();
				continue;
			}

			m_torrents.push_back(torrent);
			emit torrentAdded(torrent);

		}

	} catch (BencodeException &ex) {
		emit failedToResumeTorrents("Failed to read resume file: " + ex.what());
		return;
	}
}

void TorrentManager::saveTorrentsResumeInfo()
{
	QString dataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
	QDir dir(dataPath);
	if (!dir.exists()) {
		if (!dir.mkpath(dataPath)) {
			emit error("Failed to create directory " + dataPath);
			return;
		}
	}
	if (!dir.exists("resume")) {
		if (!dir.mkdir("resume")) {
			emit error("Failed to create directory " + dataPath + "/resume");
			return;
		}
	}
	dir.cd("resume");
	QFile resumeFile(dir.path() + "/resume.dat");
	if (!resumeFile.open(QIODevice::WriteOnly)) {
		QFileInfo info(resumeFile);
		emit error("Failed to open " + info.absoluteFilePath() + ": " + resumeFile.errorString());
		return;
	}
	BencodeDictionary *mainDict = new BencodeDictionary;
	for (Torrent *torrent : m_torrents) {
		torrent->getResumeInfo().addToBencode(mainDict);
	}
	QByteArray bencodedData = mainDict->bencode(true);
	resumeFile.write(bencodedData);
	resumeFile.close();
	delete mainDict;
}

bool TorrentManager::saveTorrentFile(const QString &filename, TorrentInfo *torrentInfo)
{
	QString dataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
	QDir dir(dataPath);
	if (!dir.exists()) {
		if (!dir.mkpath(dataPath)) {
			return false;
		}
	}
	if (!dir.exists("resume")) {
		if (!dir.mkdir("resume")) {
			return false;
		}
	}
	dir.cd("resume");

	QString newTorrentPath = dir.absoluteFilePath(torrentInfo->infoHash().toHex() + ".torrent");
	if (QFile::exists(newTorrentPath)) {
		if (!QFile::remove(newTorrentPath)) {
			return false;
		}
	}

	if (!QFile::copy(filename, newTorrentPath)) {
		return false;
	}

	return true;
}

bool TorrentManager::removeTorrent(Torrent *torrent, bool deleteData)
{
	QString dataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
	QFile savedTorrentFile(dataPath + "/resume/" + torrent->torrentInfo()->infoHash().toHex() + ".torrent");
	if (savedTorrentFile.exists()) {
		savedTorrentFile.remove();
	}
	m_torrents.removeAll(torrent);
	if (deleteData) {
		for (QFile *file : torrent->files()) {
			if (file->exists()) {
				file->remove();
			}
		}
	}
	torrent->stop();
	emit torrentRemoved(torrent);
	torrent->deleteLater();
	return true;
}

const QList<Torrent *> &TorrentManager::torrents() const
{
	return m_torrents;
}
