#include "qtorrent.h"
#include "mainwindow.h"
#include "torrentslist.h"
#include "panel.h"
#include "addtorrentdialog.h"
#include <QGuiApplication>
#include <QScreen>
#include <QStackedWidget>
#include <QMenuBar>
#include <QFileDialog>
#include <QMessageBox>
#include <QStandardPaths>
#include <QCloseEvent>

MainWindow::MainWindow(QTorrent *qTorrent)
	: m_qTorrent(qTorrent)
	, m_panel(new Panel)
	, m_torrentsList(new TorrentsList(qTorrent))
{
	// Set the main window size to 3/4 of the screen size
	int width = QGuiApplication::primaryScreen()->size().width()*3/4;
	int height = QGuiApplication::primaryScreen()->size().height()*3/4;
	resize(width, height);

	addToolBar(Qt::LeftToolBarArea, m_panel);
	QStackedWidget* stackedWidget = new QStackedWidget;
	stackedWidget->addWidget(m_torrentsList);
	setCentralWidget(stackedWidget);

	connect(m_panel, SIGNAL(showAll()), m_torrentsList, SLOT(showAll()));
	connect(m_panel, SIGNAL(showCompleted()), m_torrentsList, SLOT(showCompleted()));
	connect(m_panel, SIGNAL(showDownloading()), m_torrentsList, SLOT(showDownloading()));
	connect(m_panel, SIGNAL(showUploading()), m_torrentsList, SLOT(showUploading()));

	m_panel->openAll();

	createMenus();
}

MainWindow::~MainWindow() {

}

QTorrent* MainWindow::qTorrent() {
	return m_qTorrent;
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
	QMenu* fileMenu = menuBar()->addMenu(tr("&File"));
	m_addTorrentAction = fileMenu->addAction(tr("&Add torrent"), this, &MainWindow::addTorrentAction);
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
	AddTorrentDialog dialog(this, m_qTorrent);
	dialog.exec();
}

void MainWindow::addTorrentFromUrl(QUrl url) {
	AddTorrentDialog dialog(this, m_qTorrent);
	dialog.setTorrentUrl(url);
	dialog.exec();
}

void MainWindow::closeEvent(QCloseEvent *event) {
	if(m_qTorrent->question("Are you sure you want to exit " + QGuiApplication::applicationDisplayName() + "?")) {
		event->accept();
	} else {
		event->ignore();
	}
}
