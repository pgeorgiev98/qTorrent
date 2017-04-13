/* qTorrent - An open-source, cross-platform BitTorrent client
 * Copyright (C) 2017 Petko Georgiev
 *
 * block.h
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

#ifndef BLOCK_H
#define BLOCK_H

#include <QObject>
#include <QByteArray>
#include <QList>

class Piece;
class Peer;

class Block : public QObject
{
	Q_OBJECT

private:
	Piece *m_piece;
	int m_begin;
	int m_size;
	bool m_isDownloaded;

	/* The peers to which this Block is
	 * assigned to be downloaded from. */
	QList<Peer *> m_assignees;

public:
	Block(Piece *piece, int begin, int size);
	~Block();
	Piece *piece();
	int begin() const;
	int size() const;
	bool isDownloaded();
	QList<Peer *> &assignees();
	bool hasAssignees() const;

signals:
	void downloaded(Block *block);

public slots:
	void setDownloaded(bool isDownloaded);
	void setData(const Peer *peer, const char *data);
	void addAssignee(Peer *peer);
	void removeAssignee(Peer *peer);
	void clearAssignees();
};

#endif // BLOCK_H
