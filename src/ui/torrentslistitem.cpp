/* qTorrent - An open-source, cross-platform BitTorrent client
 * Copyright (C) 2017 Petko Georgiev
 *
 * torrentslistitem.cpp
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

#include "torrentslistitem.h"
#include "qtorrent.h"
#include "mainwindow.h"
#include "panel.h"
#include "core/torrent.h"
#include "core/torrentinfo.h"
#include "global.h"
#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QCheckBox>
#include <QFileInfo>
#include <QFile>
#include <QDesktopServices>

TorrentsListItem::TorrentsListItem(QTreeWidget *view, Torrent *torrent)
	: QTreeWidgetItem(view)
	, m_torrent(torrent)
{
}

void TorrentsListItem::setSortData(int column, QVariant data) {
	m_sortData[column] = data;
}

bool TorrentsListItem::operator<(const QTreeWidgetItem& other) const {
	int column = treeWidget()->sortColumn();
	const TorrentsListItem* otherCasted = dynamic_cast<const TorrentsListItem*>(&other);
	if(otherCasted) {
		if(m_sortData.contains(column) && otherCasted->m_sortData.contains(column)) {
			return m_sortData[column] < otherCasted->m_sortData[column];
		}
	}
	return text(column) < other.text(column);
}

void TorrentsListItem::refresh() {
	Torrent* t = torrent();
	TorrentInfo* info = t->torrentInfo();
	setName(info->torrentName());
	setSize(info->length());
	setPeers(t->connectedPeersCount(), t->allPeersCount());
	setState(t->stateString());
	setProgress(t->percentDownloaded());
	setAvailable(t->bytesAvailable());
	setLeft(t->bytesLeft());
	setTotalDownloaded(t->totalBytesDownloaded());
	setTotalUploaded(t->totalBytesUploaded());
	float ratio = (double)t->totalBytesUploaded() / (double)t->totalBytesDownloaded();
	if(t->totalBytesDownloaded() == 0) {
		ratio = 0.0f;
	}
	setRatio(ratio);
	setDownloaded(t->bytesDownloaded());
	setUploaded(t->bytesUploaded());
	setHidden(!belongsToSection());
}

bool TorrentsListItem::belongsToSection() {
	Panel::Section section = QTorrent::instance()->mainWindow()->panel()->getCurrentSection();
	switch (section) {
	case Panel::All:
		return true;
	case Panel::Completed:
		return torrent()->isDownloaded();
	case Panel::Downloading:
		return !torrent()->isDownloaded();
	case Panel::Uploading:
		return true; // TODO
	}
	Q_ASSERT(false);
	return true;
}

Torrent* TorrentsListItem::torrent() const {
	return m_torrent;
}


void TorrentsListItem::setName(const QString &value) {
	setText(Name, value);
}

void TorrentsListItem::setSize(qint64 value) {
	setText(Size, formatSize(value));
	setSortData(Size, value);
}

void TorrentsListItem::setPeers(int connected, int all) {
	setText(Peers, tr("%1/%2").arg(connected).arg(all));
	setSortData(Peers, connected);
}

void TorrentsListItem::setState(const QString &state) {
	setText(State, state);
	setSortData(State, state);
}

void TorrentsListItem::setProgress(float value) {
	setText(Progress, QString::number(value, 'f', 2) + "%");
	setSortData(Progress, value);
}

void TorrentsListItem::setAvailable(qint64 value) {
	setText(Available, formatSize(value));
	setSortData(Available, value);
}

void TorrentsListItem::setLeft(qint64 value) {
	setText(Left, formatSize(value));
	setSortData(Left, value);
}

void TorrentsListItem::setTotalDownloaded(qint64 value) {
	setText(TotalDownloaded, formatSize(value));
	setSortData(TotalDownloaded, value);
}

void TorrentsListItem::setTotalUploaded(qint64 value) {
	setText(TotalUploaded, formatSize(value));
	setSortData(TotalUploaded, value);
}

void TorrentsListItem::setRatio(float value) {
	setText(Ratio, QString::number(value, 'f', 3));
	setSortData(Ratio, value);
}

void TorrentsListItem::setDownloaded(qint64 value) {
	setText(Downloaded, formatSize(value));
	setSortData(Downloaded, value);
}

void TorrentsListItem::setUploaded(qint64 value) {
	setText(Uploaded, formatSize(value));
	setSortData(Uploaded, value);
}

void TorrentsListItem::onOpenAction() {
	QFileInfo info(*m_torrent->files().first());
	QUrl url = QUrl::fromLocalFile(info.absoluteFilePath());
	QDesktopServices::openUrl(url);
}

void TorrentsListItem::onOpenLocationAction() {
	QUrl url = QUrl::fromLocalFile(m_torrent->downloadLocation());
	QDesktopServices::openUrl(url);
}

void TorrentsListItem::onPauseAction() {
	m_torrent->pause();
}

void TorrentsListItem::onStartAction() {
	m_torrent->start();
}

void TorrentsListItem::onStopAction() {
	m_torrent->stop();
}

void TorrentsListItem::onRecheckAction() {
	m_torrent->check();
}

void TorrentsListItem::onRemoveAction() {
	QDialog dialog(QTorrent::instance()->mainWindow());
	QVBoxLayout* layout = new QVBoxLayout;
	layout->addWidget(new QLabel(tr("Are you sure you want to remove this\ntorrent from the list of torrents?")));
	QCheckBox* deleteData = new QCheckBox("Delete downloaded data");
	layout->addWidget(deleteData);
	QHBoxLayout* bottomLayout = new QHBoxLayout;
	QPushButton* yes = new QPushButton("Yes");
	QPushButton* no = new QPushButton("No");
	connect(yes, SIGNAL(clicked()), &dialog, SLOT(accept()));
	connect(no, SIGNAL(clicked()), &dialog, SLOT(reject()));
	bottomLayout->addWidget(yes);
	bottomLayout->addWidget(no);
	layout->addLayout(bottomLayout);
	dialog.setLayout(layout);
	if(dialog.exec()) {
		QTorrent::instance()->removeTorrent(m_torrent, deleteData->isChecked());
	}
}
