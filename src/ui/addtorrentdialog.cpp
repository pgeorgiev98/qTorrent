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
#include "global.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QCheckBox>
#include <QFileDialog>
#include <QGroupBox>
#include <QDir>
#include <QStandardPaths>
#include <QMessageBox>
#include <QGuiApplication>
#include <QScreen>

AddTorrentDialog::AddTorrentDialog(QWidget *parent)
	: QDialog(parent)
	, m_torrentInfo(nullptr)
{
	QVBoxLayout *layout = new QVBoxLayout;

	QGroupBox *downloadLocationBox = new QGroupBox("Download location");
	QHBoxLayout *downloadLocationLayout = new QHBoxLayout;
	downloadLocationLayout->addWidget(m_downloadLocation = new QLineEdit);
	downloadLocationLayout->addWidget(m_browseDownloadLocation= new QPushButton("Browse"));
	downloadLocationBox->setLayout(downloadLocationLayout);
	layout->addWidget(downloadLocationBox);

	QGroupBox *settingsBox = new QGroupBox("Torrent settings");
	QGridLayout *settingsLayout = new QGridLayout;
	settingsLayout->addWidget(m_startImmediately = new QCheckBox("Start Immediately"), 0, 0);
	settingsLayout->addWidget(m_skipHashCheck = new QCheckBox("Skip hash check"), 1, 0);
	m_startImmediately->setChecked(true);
	m_skipHashCheck->setChecked(false);
	settingsBox->setLayout(settingsLayout);
	layout->addWidget(settingsBox);

	QGroupBox *infoBox = new QGroupBox("Torrent info");
	QGridLayout *infoLayout = new QGridLayout;

	infoLayout->addWidget(new QLabel("Name: "), 0, 0, 1, 1);
	infoLayout->addWidget(m_name = new QLabel(), 0, 1, 1, 1);
	infoLayout->addWidget(new QLabel("Size: "), 1, 0, 1, 1);
	infoLayout->addWidget(m_size = new QLabel(), 1, 1, 1, 1);
	infoLayout->addWidget(new QLabel("Info Hash: "), 2, 0, 1, 1);
	infoLayout->addWidget(m_infoHash = new QLabel(), 2, 1, 1, 1);
	infoLayout->addWidget(new QLabel("Creation date: "), 3, 0, 1, 1);
	infoLayout->addWidget(m_creationDate = new QLabel(), 3, 1, 1, 1);
	infoLayout->addWidget(new QLabel("Created by: "), 4, 0 , 1, 1);
	infoLayout->addWidget(m_createdBy = new QLabel(), 4, 1, 1, 1);
	infoLayout->addWidget(new QLabel("Comment: "), 5, 0, 1, 1);
	infoLayout->addWidget(m_comment = new QLabel(), 5, 1, 1, 1);

	infoBox->setLayout(infoLayout);
	layout->addWidget(infoBox);

	QHBoxLayout *bottomLayout = new QHBoxLayout;
	bottomLayout->addWidget(m_ok = new QPushButton("Ok"), 0, Qt::AlignRight);
	bottomLayout->addWidget(m_cancel = new QPushButton("Cancel"), Qt::AlignRight);
	m_ok->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
	m_cancel->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
	layout->addLayout(bottomLayout);

	setLayout(layout);

	int screenWidth = QGuiApplication::primaryScreen()->size().width();
	setFixedWidth(screenWidth / 3);

	connectAll();

	// Default download location
	m_downloadLocation->setText(QStandardPaths::writableLocation(QStandardPaths::DownloadLocation));
}

AddTorrentDialog::~AddTorrentDialog()
{
	if (m_torrentInfo) {
		delete m_torrentInfo;
	}
}

void AddTorrentDialog::connectAll()
{
	connect(m_browseDownloadLocation, SIGNAL(clicked()), this, SLOT(browseDownloadLocation()));
	connect(m_ok, SIGNAL(clicked()), this, SLOT(ok()));
	connect(m_cancel, SIGNAL(clicked()), this, SLOT(cancel()));
}

bool AddTorrentDialog::setTorrentUrl(QUrl url)
{
	if (url.isLocalFile()) {
		QString filePath = url.toLocalFile();

		// If TorrentInfo is loaded - delete it
		if (m_torrentInfo) {
			delete m_torrentInfo;
			m_torrentInfo = nullptr;
		}

		// Load the torrent
		if (loadTorrent(filePath)) {
			updateInfo();
			return true;
		}
	}
	return false;
}

bool AddTorrentDialog::browseFilePath(QWidget *parent)
{
	QString filePath;
	for (;;) {
		// If TorrentInfo is loaded - delete it
		if (m_torrentInfo) {
			delete m_torrentInfo;
			m_torrentInfo = nullptr;
		}

		// Open a dialog box that accepts only torrent files
		filePath = QFileDialog::getOpenFileName(parent, tr("Open torrent"),
												QDir::homePath(), tr("Torrent Files (*.torrent)"));
		// Stop if the user clicked 'cancel'
		if (filePath.isEmpty()) {
			return false;
		}

		// Load the torrent
		if (loadTorrent(filePath)) {
			updateInfo();
			break;
		}
	}
	return true;
}

void AddTorrentDialog::browseDownloadLocation()
{
	// Open a dialog box to select the download directory
	QString downloadLocation = m_downloadLocation->text();
	downloadLocation = QFileDialog::getExistingDirectory(this, tr("Select download loaction"),
					   downloadLocation, QFileDialog::ShowDirsOnly);

	if (downloadLocation.isEmpty()) {
		return;
	}

	m_downloadLocation->setText(downloadLocation);
}

bool AddTorrentDialog::loadTorrent(const QString &filePath)
{
	if (!filePath.endsWith(".torrent", Qt::CaseInsensitive)) {
		// Accept only files with a '.torrent' extention
		QMessageBox::warning(this, tr("Add torrent"),
							 tr("Please select a valid torrent file"));
		return false;
	}

	// Check if file exists
	if (!QFile::exists(filePath)) {
		QMessageBox::warning(this, tr("Add torrent"),
							 tr("File '%1' not found")
							 .arg(filePath));
		return false;
	}

	// Delete existing TorrentInfo object
	if (m_torrentInfo) {
		delete m_torrentInfo;
	}

	// Load torrent file
	m_torrentInfo = new TorrentInfo;
	if (!m_torrentInfo->loadFromTorrentFile(filePath)) {
		// Failed to load torrent
		QMessageBox::warning(this, tr("Add torrent"),
							 tr("Failed to add torrent file\n\nReason: %1")
							 .arg(m_torrentInfo->errorString()));
		return false;
	}

	setWindowTitle(m_torrentInfo->torrentName());
	show();
	return true;
}

void AddTorrentDialog::ok()
{
	// If torrent is not loaded
	if (!m_torrentInfo) {
		QMessageBox::warning(this, tr("Add torrent"),
							 tr("No torrent file chosen. Please choose a torrent file."));
		return;
	}

	QString downloadLocation = m_downloadLocation->text();
	QDir dir(downloadLocation);
	if (!dir.exists()) {
		QMessageBox::warning(this, tr("Add torrent"),
							 tr("Please select a valid directory"));
		return;
	}

	TorrentSettings settings;
	settings.setDownloadLocation(downloadLocation);
	settings.setStartImmediately(m_startImmediately->isChecked());
	settings.setSkipHashCheck(m_skipHashCheck->isChecked());

	TorrentManager *manager = QTorrent::instance()->torrentManager();
	Torrent *torrent = manager->addTorrentFromInfo(m_torrentInfo, settings);
	if (torrent == nullptr) {
		QMessageBox::warning(this, tr("Add torrent"),
							 tr("Failed to add torrent: %1").arg(manager->errorString()));
		return;
	}
	if (!manager->saveTorrentsResumeInfo()) {
		QMessageBox::critical(this, tr("Add torrent"),
							  tr("Failed to save torrents resume info: %1").arg(manager->errorString()));
	}
	m_torrentInfo = nullptr;
	close();
}

void AddTorrentDialog::cancel()
{
	if (m_torrentInfo) {
		delete m_torrentInfo;
		m_torrentInfo = nullptr;
	}
	close();
}

void AddTorrentDialog::updateInfo()
{
	if (m_torrentInfo) {
		m_name->setText(m_torrentInfo->torrentName());
		m_size->setText(tr("%1 (%2 bytes)")
						.arg(formatSize(m_torrentInfo->length()))
						.arg(QString::number(m_torrentInfo->length())));
		m_infoHash->setText(m_torrentInfo->infoHash().toHex());
		m_creationDate->setText(m_torrentInfo->creationDate() ? m_torrentInfo->creationDate()->toString() : "N/A");
		m_createdBy->setText(m_torrentInfo->createdBy() ? *m_torrentInfo->createdBy() : "N/A");
		m_comment->setText(m_torrentInfo->comment() ? *m_torrentInfo->comment() : "N/A");
	} else {
		m_name->clear();
		m_size->clear();
		m_infoHash->clear();
		m_creationDate->clear();
		m_createdBy->clear();
		m_comment->clear();
	}
}
