#ifndef PANEL_H
#define PANEL_H

#include <QToolBar>
#include <QIcon>
#include <QList>

class QToolButton;

class Panel : public QToolBar {
	Q_OBJECT

public:
	enum Section {
		All, Completed, Downloading, Uploading
	};

	Panel();
	~Panel();
	Section getCurrentSection();

public slots:
	void openAll();
	void openCompleted();
	void openDownloading();
	void openUploading();

signals:
	void showAll();
	void showCompleted();
	void showDownloading();
	void showUploading();

private:
	void resetButtons();

	QIcon m_allIcon;
	QIcon m_completedIcon;
	QIcon m_downloadingIcon;
	QIcon m_uploadingIcon;

	QIcon m_allIconActive;
	QIcon m_completedIconActive;
	QIcon m_downloadingIconActive;
	QIcon m_uploadingIconActive;

	QList<QToolButton*> m_toolButtons;

	QToolButton* m_all;
	QToolButton* m_completed;
	QToolButton* m_downloading;
	QToolButton* m_uploading;

};

#endif // PANEL_H
