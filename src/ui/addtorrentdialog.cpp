/* qTorrent - An open-source, cross-platform BitTorrent client
 * Copyright (C) 2017 Petko Georgiev
 *
 * addtorrentdialog.cpp
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

#include "addtorrentdialog.h"
#include "qtorrent.h"
#include "core/torrentinfo.h"
#include "core/torrentmanager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QCheckBox>
#include <QFileDialog>
#include <QDir>
#include <QStandardPaths>
#include <QMessageBox>
#include <QGuiApplication>

AddTorrentDialog::AddTorrentDialog(QWidget *parent)
	: QDialog(parent)
	, m_torrentInfo(nullptr)
{
	QVBoxLayout* layout = new QVBoxLayout;

	layout->addWidget(new QLabel("File Path:"));
	QHBoxLayout* filePathLayout = new QHBoxLayout;
	filePathLayout->addWidget(m_filePath = new QLineEdit);
	filePathLayout->addWidget(m_browseFilePath= new QPushButton("Browse"));
	layout->addLayout(filePathLayout);

	layout->addWidget(new QLabel("Download Location:"));
	QHBoxLayout* downloadLocationLayout = new QHBoxLayout;
	downloadLocationLayout->addWidget(m_downloadLocation = new QLineEdit);
	downloadLocationLayout->addWidget(m_browseDownloadLocation= new QPushButton("Browse"));
	layout->addLayout(downloadLocationLayout);

	layout->addWidget(m_startImmediately = new QCheckBox("Start Immediately"));
	m_startImmediately->setChecked(true);

	QHBoxLayout* bottomLayout = new QHBoxLayout;
	bottomLayout->addWidget(m_ok = new QPushButton("Ok"));
	bottomLayout->addWidget(m_cancel = new QPushButton("Cancel"));
	layout->addLayout(bottomLayout);

	setLayout(layout);

	setFixedWidth(parent->width()/3);

	connectAll();

	// Default download location
	m_downloadLocation->setText(QStandardPaths::writableLocation(QStandardPaths::DownloadLocation));

	// ok must be disabled
	m_ok->setEnabled(false);
}

void AddTorrentDialog::connectAll() {
	connect(m_browseFilePath, SIGNAL(clicked()), this, SLOT(browseFilePath()));
	connect(m_browseDownloadLocation, SIGNAL(clicked()), this, SLOT(browseDownloadLocation()));
	connect(m_ok, SIGNAL(clicked()), this, SLOT(ok()));
	connect(m_cancel, SIGNAL(clicked()), this, SLOT(cancel()));
}

bool AddTorrentDialog::setTorrentUrl(QUrl url) {
	if(url.isLocalFile()) {
		QString filePath = url.path();

		// If TorrentInfo is loaded - delete it
		if(m_torrentInfo) {
			delete m_torrentInfo;
			m_torrentInfo = nullptr;
		}

		// Load the torrent
		if(loadTorrent(filePath)) {
			m_filePath->setText(filePath);
			m_ok->setEnabled(true);
			return true;
		}
	}
	return false;
}

void AddTorrentDialog::browseFilePath() {
	QString filePath;
	for(;;) {
		// If TorrentInfo is loaded - delete it
		if(m_torrentInfo) {
			delete m_torrentInfo;
			m_torrentInfo = nullptr;
		}

		// Open a dialog box that accepts only torrent files
		filePath = QFileDialog::getOpenFileName(this, tr("Open torrent"),
														QDir::homePath(), tr("Torrent Files (*.torrent)"));
		// Stop if the user clicked 'cancel'
		if(filePath.isEmpty()) {
			m_ok->setEnabled(false);
			break;
		}

		// Load the torrent
		if(loadTorrent(filePath)) {
			m_ok->setEnabled(true);
			break;
		}
	}

	m_filePath->setText(filePath);
}

void AddTorrentDialog::browseDownloadLocation() {
	// Open a dialog box to select the download directory
	QString downloadLocation = m_downloadLocation->text();
	downloadLocation = QFileDialog::getExistingDirectory(this, tr("Select download loaction"),
														 downloadLocation, QFileDialog::ShowDirsOnly);

	if(downloadLocation.isEmpty()) {
		return;
	}

	m_downloadLocation->setText(downloadLocation);
}

bool AddTorrentDialog::loadTorrent(const QString &filePath) {
	if(!filePath.endsWith(".torrent", Qt::CaseInsensitive)) {
		// Accept only files with a '.torrent' extention
		QMessageBox::warning(this, tr("Add torrent"),
							 tr("Please select a valid torrent file"));
		return false;
	}

	// Check if file exists
	if(!QFile::exists(filePath)) {
		QMessageBox::warning(this, tr("Add torrent"),
							 tr("File '%1' not found")
							 .arg(filePath));
		return false;
	}

	// Delete existing TorrentInfo object
	if(m_torrentInfo) {
		delete m_torrentInfo;
	}

	// Load torrent file
	m_torrentInfo = new TorrentInfo;
	if(!m_torrentInfo->loadFromTorrentFile(filePath)) {
		// Failed to load torrent
		QMessageBox::warning(this, tr("Add torrent"),
							 tr("Failed to add torrent file\n\nReason: %1")
							 .arg(m_torrentInfo->errorString()));
		return false;
	}
	return true;
}

void AddTorrentDialog::ok() {
	// If torrent is not loaded
	if(!m_torrentInfo) {
		QMessageBox::warning(this, tr("Add torrent"),
							 tr("No torrent file chosen. Please choose a torrent file."));
		return;
	}

	QString downloadLocation = m_downloadLocation->text();
	QDir dir(downloadLocation);
	if(!dir.exists()) {
		QMessageBox::warning(this, tr("Add torrent"),
							 tr("Please select a valid directory"));
		return;
	}

	TorrentSettings settings;
	settings.setDownloadLocation(downloadLocation);
	settings.setStartImmediately(m_startImmediately->isChecked());

	TorrentManager* manager = QTorrent::instance()->torrentManager();
	Torrent* torrent = manager->addTorrentFromInfo(m_torrentInfo, settings);
	if(torrent == nullptr) {
		QMessageBox::warning(this, tr("Add torrent"),
							 tr("Failed to add torrent: %1").arg(manager->errorString()));
		return;
	}
	if(!manager->saveTorrentsResumeInfo()) {
		QMessageBox::critical(this, tr("Add torrent"),
							  tr("Failed to save torrents resume info: %1").arg(manager->errorString()));
	}
	close();
}

void AddTorrentDialog::cancel() {
	if(m_torrentInfo) {
		delete m_torrentInfo;
	}
	close();
}
