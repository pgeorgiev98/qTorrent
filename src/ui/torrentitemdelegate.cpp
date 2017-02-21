/* qTorrent - An open-source, cross-platform BitTorrent client
 * Copyright (C) 2017 Petko Georgiev
 *
 * torrentitemdelegate.cpp
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
