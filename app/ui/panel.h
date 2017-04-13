/* qTorrent - An open-source, cross-platform BitTorrent client
 * Copyright (C) 2017 Petko Georgiev
 *
 * panel.h
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

#ifndef PANEL_H
#define PANEL_H

#include <QToolBar>
#include <QIcon>
#include <QList>

class QToolButton;

class Panel : public QToolBar
{
	Q_OBJECT

public:
	enum Section {
		All, Completed, Downloading, Uploading
	};

	Panel(QWidget *parent = nullptr);
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

	QList<QToolButton *> m_toolButtons;

	QToolButton *m_all;
	QToolButton *m_completed;
	QToolButton *m_downloading;
	QToolButton *m_uploading;

};

#endif // PANEL_H
