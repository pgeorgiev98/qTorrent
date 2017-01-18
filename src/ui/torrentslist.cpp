#include "qtorrent.h"
#include "core/torrent.h"
#include "core/torrentinfo.h"
#include "torrentslist.h"
#include "torrentslistitem.h"
#include <QHeaderView>

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
