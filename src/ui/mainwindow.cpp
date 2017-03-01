/* qTorrent - An open-source, cross-platform BitTorrent client
 * Copyright (C) 2017 Petko Georgiev
 *
 * mainwindow.cpp
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

#include "qtorrent.h"
#include "mainwindow.h"
#include "torrentslist.h"
#include "panel.h"
#include "torrentinfopanel.h"
#include "addtorrentdialog.h"
#include <QGuiApplication>
#include <QScreen>
#include <QStackedWidget>
#include <QMenuBar>
#include <QFileDialog>
#include <QMessageBox>
#include <QStandardPaths>
#include <QCloseEvent>
#include <QApplication>
#include <QVBoxLayout>

const int UI_REFRESH_INTERVAL = 300;

MainWindow::MainWindow()
	: m_panel(new Panel)
	, m_torrentsList(new TorrentsList())
	, m_infoPanel(new TorrentInfoPanel)
{
	// Set the main window size to 3/4 of the screen size
	int width = QGuiApplication::primaryScreen()->size().width()*3/4;
	int height = QGuiApplication::primaryScreen()->size().height()*3/4;
	resize(width, height);

	addToolBar(Qt::LeftToolBarArea, m_panel);

	QStackedWidget* stackedWidget = new QStackedWidget;
	QWidget* torrentsListWidget = new QWidget;
	QVBoxLayout* torrentsListLayout = new QVBoxLayout;
	torrentsListLayout->addWidget(m_torrentsList);
	torrentsListLayout->addWidget(m_infoPanel);
	torrentsListWidget->setLayout(torrentsListLayout);
	stackedWidget->addWidget(torrentsListWidget);
	setCentralWidget(stackedWidget);

	connect(m_panel, SIGNAL(showAll()), m_torrentsList, SLOT(showAll()));
	connect(m_panel, SIGNAL(showCompleted()), m_torrentsList, SLOT(showCompleted()));
	connect(m_panel, SIGNAL(showDownloading()), m_torrentsList, SLOT(showDownloading()));
	connect(m_panel, SIGNAL(showUploading()), m_torrentsList, SLOT(showUploading()));

	m_panel->openAll();

	createMenus();

	m_refreshTimer.setInterval(UI_REFRESH_INTERVAL);
	m_refreshTimer.start();
	connect(&m_refreshTimer, SIGNAL(timeout()), m_torrentsList, SLOT(refresh()));
	connect(&m_refreshTimer, SIGNAL(timeout()), m_infoPanel, SLOT(refresh()));

	m_trayIconMenu = new QMenu(this);

	QAction* hideClientAction = new QAction(tr("Hide/Show qTorrent"), this);
	connect(hideClientAction, &QAction::triggered, this, &MainWindow::toggleHideShow);
	m_trayIconMenu->addAction(hideClientAction);

	QAction* exitAction = new QAction(tr("Exit"), this);
	connect(exitAction, &QAction::triggered, this, &MainWindow::exitAction);
	m_trayIconMenu->addAction(exitAction);

	m_trayIcon = new QSystemTrayIcon(this);
	m_trayIcon->setContextMenu(m_trayIconMenu);

	m_trayIcon->show();

	connect(m_trayIcon, SIGNAL(activated(QSystemTrayIcon::ActivationReason)),
			this, SLOT(trayIconActivated(QSystemTrayIcon::ActivationReason)));
}

MainWindow::~MainWindow() {

}

Panel* MainWindow::panel() {
	return m_panel;
}

TorrentsList* MainWindow::torrentsList() {
	return m_torrentsList;
}


void MainWindow::addTorrent(Torrent *torrent) {
	m_torrentsList->addTorrent(torrent);
}

void MainWindow::removeTorrent(Torrent *torrent) {
	m_torrentsList->removeTorrent(torrent);
}


void MainWindow::createMenus() {
	menuBar()->show();

	// Menus
	QMenu* fileMenu = menuBar()->addMenu(tr("&File"));
	QMenu* viewMenu = menuBar()->addMenu(tr("&View"));
	QMenu* helpMenu = menuBar()->addMenu(tr("&Help"));

	// Actions
	QAction* addTorrentAction = new QAction(tr("&Add torrent"), this);
	QAction* exitAction = new QAction(tr("&Exit"), this);
	QAction* hideClientAction = new QAction(tr("Hide qTorrent"), this);
	m_viewTorrentsFilterPanel = new QAction(tr("Torrents filter panel"), this);
	m_viewTorrentInfoPanel = new QAction(tr("Torrents info panel"), this);
	QAction* aboutAction = new QAction(tr("&About"), this);

	// Connect actions
	connect(addTorrentAction, &QAction::triggered, this, &MainWindow::addTorrentAction);
	connect(exitAction, &QAction::triggered, this, &MainWindow::exitAction);
	connect(hideClientAction, &QAction::triggered, this, &MainWindow::hide);
	connect(m_viewTorrentsFilterPanel, &QAction::triggered, this, &MainWindow::toggleHideShowTorrentsFilterPanel);
	connect(m_viewTorrentInfoPanel, &QAction::triggered, this, &MainWindow::toggleHideShowTorrentInfoPanel);
	connect(aboutAction, &QAction::triggered, this, &MainWindow::aboutAction);

	// Action shortcuts
	addTorrentAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_O));
	exitAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_E));

	// Add actions to menus
	fileMenu->addAction(addTorrentAction);
	fileMenu->addAction(exitAction);

	viewMenu->addAction(hideClientAction);
	viewMenu->addSeparator();
	viewMenu->addAction(m_viewTorrentsFilterPanel);
	viewMenu->addAction(m_viewTorrentInfoPanel);

	helpMenu->addAction(aboutAction);

	// Configure actions
	m_viewTorrentsFilterPanel->setCheckable(true);
	m_viewTorrentInfoPanel->setCheckable(true);
	m_viewTorrentsFilterPanel->setChecked(true);
	m_viewTorrentInfoPanel->setChecked(true);
	connect(m_infoPanel, SIGNAL(visibilityChanged(bool)), m_viewTorrentInfoPanel, SLOT(setChecked(bool)));
	connect(m_panel, SIGNAL(visibilityChanged(bool)), m_viewTorrentsFilterPanel, SLOT(setChecked(bool)));
}


QString MainWindow::getDownloadLocation() {
	// Open a dialog box to select the download directory
	QString downloadPath;
	downloadPath = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
	downloadPath = QFileDialog::getExistingDirectory(this, tr("Select download directory"), downloadPath);
	// String is empty if user canceled the dialog box
	return downloadPath;
}

void MainWindow::addTorrentAction() {
	AddTorrentDialog dialog(this);
	dialog.exec();
}

void MainWindow::exitAction() {
	if(QTorrent::instance()->question("Are you sure you want to exit " + QGuiApplication::applicationDisplayName() + "?")) {
		QApplication::quit();
	}
}

void MainWindow::addTorrentFromUrl(QUrl url) {
	AddTorrentDialog dialog(this);
	dialog.setTorrentUrl(url);
	dialog.exec();
}

void MainWindow::closeEvent(QCloseEvent *event) {
	event->ignore();
	hide();
}

void MainWindow::toggleHideShowTorrentsFilterPanel() {
	if(m_panel->isHidden()) {
		m_panel->show();
	} else {
		m_panel->hide();
	}
}

void MainWindow::toggleHideShowTorrentInfoPanel() {
	if(m_infoPanel->isHidden()) {
		m_infoPanel->show();
	} else {
		m_infoPanel->hide();
	}
}

void MainWindow::toggleHideShow() {
	if(isHidden()) {
		show();
	} else {
		hide();
	}
}

void MainWindow::aboutAction() {
	QMessageBox::about(this, tr("About qTorrent"),
					   tr("<p><b>qTorrent</b> is a simple BitTorrent client, written from "
						  "scratch in C++ with Qt5. qTorrent aims to be a good, lightweight "
						  "alternative to all the other BitTorrent clients.</p>"
						  "<p>It is licensed under the GNU General Public License v3.0</p>"));
}

void MainWindow::trayIconActivated(QSystemTrayIcon::ActivationReason reason) {
	if(reason == QSystemTrayIcon::DoubleClick) {
		show();
	}
}
