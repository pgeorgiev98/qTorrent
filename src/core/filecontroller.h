/* qTorrent - An open-source, cross-platform BitTorrent client
 * Copyright (C) 2017 Petko Georgiev
 *
 * filecontroller.h
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

#ifndef FILECONTROLLER_H
#define FILECONTROLLER_H

#include <QObject>

class QThread;
class Torrent;
class Piece;

class FileControllerWorker : public QObject
{
	Q_OBJECT

public:
	FileControllerWorker(Torrent *torrent);

public slots:
	void checkTorrent();

signals:
	void torrentChecked();
	void pieceAvailable(Piece* piece, bool available);

private:
	Torrent *m_torrent;
};

class FileController : public QObject
{
	Q_OBJECT

public:
	FileController(Torrent *torrent);
	~FileController();

signals:
	void checkTorrent();
	void torrentChecked();

private:
	Torrent *m_torrent;
	QThread *m_workerThread;
};

#endif // FILECONTROLLER_H
