#ifndef TORRENTSTATUSBAR_H
#define TORRENTSTATUSBAR_H

#include <QToolBar>

class QLabel;

class TorrentInfoPanel : public QToolBar {
	Q_OBJECT

public:
	TorrentInfoPanel();

	void refreshInfoTab();

public slots:
	void refresh();

private:
	QLabel* m_torrentName;
	QLabel* m_torrentSize;
	QLabel* m_pieceSize;
	QLabel* m_infoHash;
	QLabel* m_creationDate;
	QLabel* m_createdBy;
	QLabel* m_comment;
};

#endif // TORRENTSTATUSBAR_H
