/* qTorrent - An open-source, cross-platform BitTorrent client
 * Copyright (C) 2017 Petko Georgiev
 *
 * piece.h
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

#ifndef PIECE_H
#define PIECE_H

#include <QList>
#include <QByteArray>

class Torrent;
class Block;

class Piece {
	Torrent* m_torrent;
	int m_pieceNumber;
	int m_size;
	bool m_isDownloaded;
	char* m_pieceData;
	QList<Block*> m_blocks;

	void addBlock(Block* block);
	bool checkIfFullyDownloaded();
public:
	Piece(Torrent* torrent, int pieceNumber, int size);
	~Piece();

	/* Getters */

	bool isDownloaded() const;
	int pieceNumber() const;
	char* data() const;
	int size() const;

	/* Commands */

	Block* requestBlock(int size);
	// Update the block state: check if it's fully downloaded
	void updateState();
	void deleteBlock(Block* block);
	void unloadFromMemory();
	void setDownloaded(bool isDownloaded);
	// Gets data for a block. Reads from files if needed
	bool getBlockData(int begin, int size, QByteArray& blockData);
	bool getPieceData(QByteArray& pieceData);
	// Returns a pointer to an existing block or nullptr if no such block exists
	Block* getBlock(int begin, int size) const;
};

#endif // PIECE_H
