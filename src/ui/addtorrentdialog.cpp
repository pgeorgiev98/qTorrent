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
}

void AddTorrentDialog::connectAll() {
	connect(m_browseFilePath, SIGNAL(clicked()), this, SLOT(browseFilePath()));
	connect(m_browseDownloadLocation, SIGNAL(clicked()), this, SLOT(browseDownloadLocation()));
	connect(m_ok, SIGNAL(clicked()), this, SLOT(ok()));
	connect(m_cancel, SIGNAL(clicked()), this, SLOT(cancel()));
}

void AddTorrentDialog::setTorrentUrl(QUrl url) {
	if(url.isLocalFile()) {
		m_filePath->setText(url.path());
	}
}

void AddTorrentDialog::browseFilePath() {
	// Open a dialog box that accepts only torrent files
	QString filePath = QFileDialog::getOpenFileName(this, tr("Open torrent"),
													QDir::homePath(), tr("Torrent Files (*.torrent)"));
	if(filePath.isEmpty()) {
		return;
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

void AddTorrentDialog::ok() {
	QString filePath = m_filePath->text();
	QFile file(filePath);
	if(!file.exists() || !filePath.endsWith(".torrent", Qt::CaseInsensitive)) {
		QMessageBox::warning(this, QGuiApplication::applicationDisplayName(),
							 "Please select a valid torrent file");
		return;
	}
	QString downloadLocation = m_downloadLocation->text();
	QDir dir(downloadLocation);
	if(!dir.exists()) {
		QMessageBox::warning(this, QGuiApplication::applicationDisplayName(),
							 "Please select a valid directory");
		return;
	}

	TorrentSettings settings;
	settings.setDownloadLocation(downloadLocation);
	settings.setStartImmediately(m_startImmediately->isChecked());

	QTorrent::instance()->addTorrentFromLocalFile(filePath, settings);
	close();
}

void AddTorrentDialog::cancel() {
	close();
}
