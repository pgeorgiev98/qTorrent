#include "torrentitemdelegate.h"
#include "torrentslistitem.h"
#include <QApplication>

TorrentItemDelegate::TorrentItemDelegate(QWidget *parent)
	: QItemDelegate(parent)
{
}

void TorrentItemDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const {
	if(index.column() != TorrentsListItem::Progress) {
		QItemDelegate::paint(painter, option, index);
		return;
	}

	float progress = 0.0f;
	QString display = index.data(Qt::DisplayRole).toString();
	if(!display.isEmpty()) {
		display.remove(display.size() - 1, 1);
		progress = display.toFloat();
	}

	QStyleOptionProgressBar progressBarOption;
	progressBarOption.rect = option.rect;
	progressBarOption.minimum = 0;
	progressBarOption.maximum = 100;
	progressBarOption.progress = progress;
	progressBarOption.text = QString::number(progress) + "%";
	progressBarOption.textVisible = true;

	QApplication::style()->drawControl(QStyle::CE_ProgressBar, &progressBarOption, painter);
}
