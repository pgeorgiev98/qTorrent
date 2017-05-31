/* qTorrent - An open-source, cross-platform BitTorrent client
 * Copyright (C) 2017 Petko Georgiev
 *
 * settingswindow.cpp
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

#include "settingswindow.h"
#include "qtorrent.h"
#include "core/torrentserver.h"
#include "core/ratecontroller.h"
#include <QPushButton>
#include <QLineEdit>
#include <QLabel>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QIntValidator>
#include <QSettings>
#include <QMessageBox>
#include <QGuiApplication>

SettingsWindow::SettingsWindow(QWidget *parent)
	: QWidget(parent)
{
	QVBoxLayout *mainLayout = new QVBoxLayout;
	setLayout(mainLayout);

	m_serverStartPort = new QLineEdit;
	m_serverEndPort = new QLineEdit;
	m_serverStartPort->setValidator(new QIntValidator(0, 49151));
	m_serverEndPort->setValidator(new QIntValidator(0, 49151));
	m_serverStartPort->setMaximumWidth(200);
	m_serverEndPort->setMaximumWidth(200);

	QHBoxLayout *serverPortLayout = new QHBoxLayout;
	serverPortLayout->addWidget(new QLabel(tr("Server port range: ")));
	serverPortLayout->addWidget(m_serverStartPort);
	serverPortLayout->addWidget(new QLabel(tr(" to ")));
	serverPortLayout->addWidget(m_serverEndPort);
	serverPortLayout->addStretch();

	mainLayout->addLayout(serverPortLayout);

	m_uploadLimit = new QLineEdit;
	m_uploadLimit->setMaximumWidth(200);
	m_uploadLimit->setValidator(new QIntValidator);
	QHBoxLayout *uploadLimitLayout = new QHBoxLayout;
	uploadLimitLayout->addWidget(new QLabel(tr("Upload limit: ")));
	uploadLimitLayout->addWidget(m_uploadLimit);
	uploadLimitLayout->addWidget(new QLabel(tr("KiB/s")));
	uploadLimitLayout->addStretch();

	mainLayout->addLayout(uploadLimitLayout);

	m_downloadLimit = new QLineEdit;
	m_downloadLimit->setMaximumWidth(200);
	m_downloadLimit->setValidator(new QIntValidator);
	QHBoxLayout *downloadLimitLayout = new QHBoxLayout;
	downloadLimitLayout->addWidget(new QLabel(tr("Download limit: ")));
	downloadLimitLayout->addWidget(m_downloadLimit);
	downloadLimitLayout->addWidget(new QLabel(tr("KiB/s")));
	downloadLimitLayout->addStretch();

	mainLayout->addLayout(downloadLimitLayout);

	QPushButton *applyButton = new QPushButton(tr("Apply"));
	QPushButton *cancelButton = new QPushButton(tr("Cancel"));
	applyButton->setMaximumWidth(200);
	cancelButton->setMaximumWidth(200);

	QHBoxLayout *buttonsLayout = new QHBoxLayout;
	buttonsLayout->addWidget(applyButton);
	buttonsLayout->addWidget(cancelButton);
	buttonsLayout->addStretch();

	mainLayout->addLayout(buttonsLayout);

	mainLayout->addStretch();

	connect(applyButton, &QPushButton::clicked, this, &SettingsWindow::apply);
	connect(cancelButton, &QPushButton::clicked, this, &SettingsWindow::reset);

	reset();
}

void SettingsWindow::apply()
{
	bool ok;

	quint16 serverStartPort = m_serverStartPort->text().toInt(&ok);
	if (!ok) {
		QMessageBox::warning(this, QGuiApplication::applicationDisplayName(), tr("Please enter a valid start port"));
		return;
	}

	quint16 serverEndPort = m_serverEndPort->text().toInt(&ok);
	if (!ok) {
		QMessageBox::warning(this, QGuiApplication::applicationDisplayName(), tr("Please enter a valid end port"));
		return;
	}

	qint64 uploadLimit = m_uploadLimit->text().toLongLong(&ok) * 1024;
	if (!ok) {
		QMessageBox::warning(this, QGuiApplication::applicationDisplayName(), tr("Please enter a valid upload limit"));
		return;
	}

	qint64 downloadLimit = m_downloadLimit->text().toLongLong(&ok) * 1024;
	if (!ok) {
		QMessageBox::warning(this, QGuiApplication::applicationDisplayName(), tr("Please enter a valid download limit"));
		return;
	}

	QSettings settings;
	settings.setValue("ServerStartPort", serverStartPort);
	settings.setValue("ServerEndPort", serverEndPort);
	settings.setValue("UploadLimit", uploadLimit);
	settings.setValue("DownloadLimit", downloadLimit);

	// Restart the server
	QTorrent::instance()->server()->startServer();

	RateController::instance()->setUploadLimit(uploadLimit);
	RateController::instance()->setDownloadLimit(downloadLimit);
}

void SettingsWindow::reset()
{
	QSettings settings;
	m_serverStartPort->setText(settings.value("ServerStartPort").toString());
	m_serverEndPort->setText(settings.value("ServerEndPort").toString());
	m_uploadLimit->setText(QString::number(settings.value("UploadLimit").toLongLong() / 1024));
	m_downloadLimit->setText(QString::number(settings.value("DownloadLimit").toLongLong() / 1024));
}
