/* qTorrent - An open-source, cross-platform BitTorrent client
 * Copyright (C) 2017 Petko Georgiev
 *
 * addtorrentdialog.h
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

#ifndef ADDTORRENTDIALOG_H
#define ADDTORRENTDIALOG_H

#include <QDialog>
#include <QUrl>

class QLineEdit;
class QPushButton;
class QCheckBox;
class QLabel;
class TorrentInfo;

class AddTorrentDialog : public QDialog
{
	Q_OBJECT

public:
	AddTorrentDialog(QWidget *parent);
	~AddTorrentDialog();

	bool setTorrentUrl(QUrl url);
	bool browseFilePath(QWidget *parent);

	void updateInfo();

public slots:
	void browseDownloadLocation();
	void ok();
	void cancel();

private:
	QLineEdit *m_downloadLocation;
	QPushButton *m_browseDownloadLocation;
	QCheckBox *m_startImmediately;
	QPushButton *m_ok;
	QPushButton *m_cancel;

	QLabel *m_name;
	QLabel *m_size;
	QLabel *m_infoHash;
	QLabel *m_creationDate;
	QLabel *m_createdBy;
	QLabel *m_comment;

	TorrentInfo *m_torrentInfo;

	void connectAll();
	bool loadTorrent(const QString &filePath);
};

#endif // ADDTORRENTDIALOG_H
