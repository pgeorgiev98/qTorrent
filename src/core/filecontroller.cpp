/* qTorrent - An open-source, cross-platform BitTorrent client
 * Copyright (C) 2017 Petko Georgiev
 *
 * filecontroller.cpp
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

#include "filecontroller.h"
#include "torrent.h"
#include "torrentinfo.h"
#include "piece.h"
#include <QCryptographicHash>
#include <QThread>

FileController::FileController(Torrent *torrent)
	: m_torrent(torrent)
	, m_workerThread(new QThread)
{
	FileControllerWorker *worker = new FileControllerWorker(torrent);
	worker->moveToThread(m_workerThread);
	connect(m_workerThread, &QThread::finished, worker, &FileControllerWorker::deleteLater);

	m_workerThread->start();

	// For torrent-checking
	connect(this, &FileController::checkTorrent, worker, &FileControllerWorker::checkTorrent);
	connect(worker, &FileControllerWorker::torrentChecked, this, &FileController::torrentChecked);
}

FileController::~FileController()
{
	m_workerThread->quit();
	m_workerThread->wait();
	delete m_workerThread;
}


FileControllerWorker::FileControllerWorker(Torrent *torrent)
	: m_torrent(torrent)
{
}

void FileControllerWorker::checkTorrent()
{
	TorrentInfo *info = m_torrent->torrentInfo();
	QList<Piece *> &pieces = m_torrent->pieces();
	for(Piece *piece : pieces) {
		m_torrent->setPieceAvailable(piece, false);
	}
	for(Piece *piece : pieces) {
		QByteArray pieceData, pieceHash;
		if(!piece->getPieceData(pieceData)) {
			m_torrent->setPieceAvailable(piece, false);
			continue;
		}
		pieceHash = QCryptographicHash::hash(pieceData, QCryptographicHash::Sha1);
		bool pieceIsValid = (pieceHash == info->piece(piece->pieceNumber()));
		m_torrent->setPieceAvailable(piece, pieceIsValid);
		// TODO report some kind of percentage
	}
	emit torrentChecked();
}
