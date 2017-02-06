#ifndef PROGRESSDELEGATE_H
#define PROGRESSDELEGATE_H

#include <QItemDelegate>

class TorrentItemDelegate : public QItemDelegate {
	Q_OBJECT

public:
	TorrentItemDelegate(QWidget* parent);
	void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const;
};

#endif // PROGRESSDELEGATE_H
