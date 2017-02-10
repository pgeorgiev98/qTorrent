#include "torrentslistitem.h"
#include "qtorrent.h"
#include "mainwindow.h"
#include "panel.h"
#include "core/torrent.h"
#include "core/torrentinfo.h"
#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QCheckBox>

TorrentsListItem::TorrentsListItem(QTorrent* qTorrent, QTreeWidget *view, Torrent *torrent)
	: QTreeWidgetItem(view)
	, m_qTorrent(qTorrent)
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
	Panel::Section section = m_qTorrent->mainWindow()->panel()->getCurrentSection();
	switch (section) {
	case Panel::All:
		return true;
	case Panel::Completed:
		return torrent()->downloaded();
	case Panel::Downloading:
		return !torrent()->downloaded();
	case Panel::Uploading:
		return true; // TODO
	}
	Q_ASSERT(false);
	return true;
}

Torrent* TorrentsListItem::torrent() const {
	return m_torrent;
}


QString TorrentsListItem::toPrettySize(qint64 bytes) {
	int i;
	qint64 nw = bytes, mul = 1;
	for(i = 0; nw >= 1024 && i < 7; i++, mul *= 1024, nw /= 1024);
	double nbytes = (double)bytes/mul;


	QString str = QString::number(nbytes, 'f', 2);
	str += ' ';

	switch(i) {
	case 0: return str + "B";
	case 1: return str + "KiB";
	case 2: return str + "MiB";
	case 3: return str + "GiB";
	case 4: return str + "TiB";
	case 5: return str + "PiB";
	case 6: return str + "EiB";
	default: return str + "ZiB";
	}
}


void TorrentsListItem::setName(const QString &value) {
	setText(Name, value);
}

void TorrentsListItem::setSize(qint64 value) {
	setText(Size, toPrettySize(value));
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
	setText(Available, toPrettySize(value));
	setSortData(Available, value);
}

void TorrentsListItem::setLeft(qint64 value) {
	setText(Left, toPrettySize(value));
	setSortData(Left, value);
}

void TorrentsListItem::setTotalDownloaded(qint64 value) {
	setText(TotalDownloaded, toPrettySize(value));
	setSortData(TotalDownloaded, value);
}

void TorrentsListItem::setTotalUploaded(qint64 value) {
	setText(TotalUploaded, toPrettySize(value));
	setSortData(TotalUploaded, value);
}

void TorrentsListItem::setRatio(float value) {
	setText(Ratio, QString::number(value, 'f', 3));
	setSortData(Ratio, value);
}

void TorrentsListItem::setDownloaded(qint64 value) {
	setText(Downloaded, toPrettySize(value));
	setSortData(Downloaded, value);
}

void TorrentsListItem::setUploaded(qint64 value) {
	setText(Uploaded, toPrettySize(value));
	setSortData(Uploaded, value);
}

void TorrentsListItem::onPauseAction() {
	m_torrent->pause();
}

void TorrentsListItem::onStartAction() {
	m_torrent->start();
}

void TorrentsListItem::onRemoveAction() {
	QDialog dialog(m_qTorrent->mainWindow());
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
		m_qTorrent->removeTorrent(m_torrent, deleteData->isChecked());
	}
}
