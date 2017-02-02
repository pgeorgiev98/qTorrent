#include "qtorrent.h"
#include "core/torrent.h"
#include "core/torrentinfo.h"
#include "torrentslist.h"
#include "torrentslistitem.h"
#include <QHeaderView>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QMenu>

const int UI_REFRESH_INTERVAL = 300;

TorrentsList::TorrentsList(QTorrent *qTorrent)
	: m_qTorrent(qTorrent)
{
	QFontMetrics fm = fontMetrics();
	QStringList headers;
	headers << tr("Torrent") << tr("Size") << tr("Peers") << tr("Progress") << tr("Downloaded") << tr("Uploaded");

	QHeaderView *headerView = header();

	setHeaderLabels(headers);
	setSelectionBehavior(QAbstractItemView::SelectRows);
	setAlternatingRowColors(true);
	setRootIsDecorated(false);
	setSortingEnabled(true);
	setExpandsOnDoubleClick(true);
	setAcceptDrops(true);
	setContextMenuPolicy(Qt::CustomContextMenu);

	headerView->setSectionsMovable(true);
	headerView->setSectionsClickable(true);

	headerView->resizeSection(TorrentsListItem::Name, fm.width("typical-name-for-a-torrent.torrent"));
	headerView->resizeSection(TorrentsListItem::Size, fm.width(" 987.65 MiB "));
	headerView->resizeSection(TorrentsListItem::Peers, fm.width(headers.at(TorrentsListItem::Peers) + "  "));
	headerView->resizeSection(TorrentsListItem::Progress, fm.width(headers.at(TorrentsListItem::Progress) + "  "));
	headerView->resizeSection(TorrentsListItem::Downloaded, fm.width(headers.at(TorrentsListItem::Downloaded) + "  "));
	headerView->resizeSection(TorrentsListItem::Uploaded, fm.width(headers.at(TorrentsListItem::Uploaded) + "  "));

	for(Torrent* torrent : m_qTorrent->torrents()) {
		addTorrent(torrent);
	}

	m_refreshTimer.setInterval(UI_REFRESH_INTERVAL);
	m_refreshTimer.start();

	connect(&m_refreshTimer, SIGNAL(timeout()), this, SLOT(refresh()));
	connect(this, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(openContextMenu(QPoint)));
}

TorrentsList::~TorrentsList() {

}

void TorrentsList::addTorrent(Torrent *torrent) {
	TorrentsListItem* item = new TorrentsListItem(m_qTorrent, this, torrent);
	m_items.append(item);
	item->refresh();
}

void TorrentsList::removeTorrent(Torrent *torrent) {
	TorrentsListItem* item = torrentItem(torrent);
	removeItemWidget(item, 0);
}

void TorrentsList::refresh() {
	for(TorrentsListItem* item : m_items) {
		item->refresh();
	}
}

void TorrentsList::openContextMenu(const QPoint &pos) {
	TorrentsListItem* item = dynamic_cast<TorrentsListItem*>(itemAt(pos));

	if(item == nullptr) {
		// This shouldn't happen at all
		return;
	}

	QAction* pauseAct = new QAction(tr("&Pause"), this);
	QAction* startAct = new QAction(tr("&Start"), this);

	if(item->torrent()->isPaused()) {
		pauseAct->setEnabled(false);
	}

	QMenu menu(this);

	menu.addAction(pauseAct);
	menu.addAction(startAct);

	connect(pauseAct, SIGNAL(triggered()), item, SLOT(onPauseAction()));
	connect(startAct, SIGNAL(triggered()), item, SLOT(onStartAction()));

	menu.exec(mapToGlobal(pos));
}

void TorrentsList::showAll() {
	for(TorrentsListItem* item : m_items) {
		item->setHidden(false);
	}
}

void TorrentsList::showCompleted() {
	for(TorrentsListItem* item : m_items) {
		bool downloaded = item->torrent()->downloaded();
		item->setHidden(!downloaded);
	}
}

void TorrentsList::showDownloading() {
	for(TorrentsListItem* item : m_items) {
		bool downloaded = item->torrent()->downloaded();
		item->setHidden(downloaded);
	}
}

void TorrentsList::showUploading() {
	for(TorrentsListItem* item : m_items) {
		item->setHidden(false);
	}
}


TorrentsListItem* TorrentsList::torrentItem(Torrent *torrent) {
	for(TorrentsListItem* item : m_items) {
		if(item->torrent() == torrent) {
			return item;
		}
	}
	return nullptr;
}

TorrentsListItem* TorrentsList::torrentItem(const QString &name) {
	for(TorrentsListItem* item : m_items) {
		if(item->text(TorrentsListItem::Name) == name) {
			return item;
		}
	}
	return nullptr;
}

void TorrentsList::dragEnterEvent(QDragEnterEvent *event) {
	event->acceptProposedAction();
}

void TorrentsList::dragMoveEvent(QDragMoveEvent *event) {
	event->acceptProposedAction();
}

void TorrentsList::dropEvent(QDropEvent *event) {
	const QMimeData* mimeData = event->mimeData();
	if(mimeData->hasUrls()) {
		QList<QUrl> urlList = mimeData->urls();
		for(QUrl url : urlList) {
			m_qTorrent->addTorrentFromUrl(url);
		}
	}
}
