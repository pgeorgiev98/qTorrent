/* qTorrent - An open-source, cross-platform BitTorrent client
 * Copyright (C) 2017 Petko Georgiev
 *
 * piece.cpp
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

#include "piece.h"
#include "block.h"
#include "torrent.h"
#include "torrentinfo.h"
#include "peer.h"
#include "trackerclient.h"
#include <QCryptographicHash>
#include <QTcpSocket>
#include <QFile>
#include <QDebug>

Piece::Piece(Torrent* torrent, int pieceNumber, int size)
	: m_torrent(torrent)
	, m_pieceNumber(pieceNumber)
	, m_size(size)
	, m_isDownloaded(false)
	, m_pieceData(new char[size])
{
}

Piece::~Piece() {
	for(auto b : m_blocks) {
		delete b;
	}
	if(m_pieceData != nullptr) {
		delete[] m_pieceData;
	}
}

bool Piece::isDownloaded() const {
	return m_isDownloaded;
}

int Piece::pieceNumber() const {
	return m_pieceNumber;
}

char* Piece::data() const {
	return m_pieceData;
}

int Piece::size() const {
	return m_size;
}

void Piece::addBlock(Block *block) {
	int insertPos = 0;
	for(int i = 0; i < m_blocks.size(); i++) {
		if(block->begin() < m_blocks[i]->begin()) {
			break;
		}
		insertPos++;
	}
	m_blocks.insert(insertPos, block);
}

void Piece::deleteBlock(Block* block) {
	int blockNumber = -1;
	for(int i = 0; i < m_blocks.size(); i++) {
		if(m_blocks[i] == block) {
			blockNumber = i;
			break;
		}
	}
	if(blockNumber != -1) {
		delete m_blocks[blockNumber];
		m_blocks.removeAt(blockNumber);
	}
}

bool Piece::checkIfFullyDownloaded() {
	if(m_isDownloaded) { // If already marked as downloaded, don't do anything
		return true;
	}
	Q_ASSERT_X(m_pieceData != nullptr, "Piece::checkIfFullyDownloaded()", "Piece not loaded");
	int pos = 0;
	for(auto b : m_blocks) {
		if(b->begin() == pos && b->isDownloaded()) {
			pos += b->size();
		} else {
			m_isDownloaded = false;
			return false;
		}
	}
	m_isDownloaded = (m_size == pos);
	return m_isDownloaded;
}

void Piece::updateState() {
	if(checkIfFullyDownloaded()) {
		Q_ASSERT_X(m_pieceData != nullptr, "Piece::updateState()", "Piece not loaded");
		QCryptographicHash hash(QCryptographicHash::Sha1);
		hash.addData(m_pieceData, m_size);
		QByteArray actualHash = hash.result();
		if(actualHash != m_torrent->torrentInfo()->piece(m_pieceNumber)) {
			for(auto b : m_blocks) {
				delete b;
			}
			m_blocks.clear();
			m_isDownloaded = false;
			qDebug() << "Piece" << m_pieceNumber << "failed SHA1 validation";
		} else {
			m_torrent->savePiece(this);
			m_isDownloaded = true;
			unloadFromMemory();
			m_torrent->downloadedPiece(this);
		}
	}
}

Block* Piece::requestBlock(int size) {
	int tmp = 0;
	int s = size;
	Block* block = nullptr;

	for(auto b : m_blocks) {
		if(!b->isDownloaded() && !b->hasAssignees()) {
			return b;
		}
		if(tmp < b->begin()) {
			s = b->begin() - tmp;
			if(s > size) {
				s = size;
			}
			block = new Block(this, tmp, s);
			break;
		} else {
			tmp = b->begin() + b->size();
		}
	}
	if(block == nullptr) {
		int pieceSize = m_size;
		if(pieceSize - tmp > size) {
			block = new Block(this, tmp, size);
		} else if(pieceSize - tmp > 0) {
			block = new Block(this, tmp, pieceSize - tmp);
		}
	}
	if(block != nullptr) {
		addBlock(block);
	}

	return block;
}

void Piece::unloadFromMemory() {
	Q_ASSERT_X(m_pieceData != nullptr, "Piece::unloadFromMemory()", "Piece is not loaded");
	Q_ASSERT_X(checkIfFullyDownloaded(), "Piece::unloadFromMemory()", "Piece is not fully downloaded");
	delete[] m_pieceData;
	m_pieceData = nullptr;
}

void Piece::setDownloaded(bool downloaded) {
	m_isDownloaded = downloaded;
}

bool Piece::getBlockData(int begin, int size, QByteArray& blockData) {
	blockData.clear();

	// Check if piece is loaded in memory (unlikely)
	if(m_pieceData) {
		blockData.append(m_pieceData + begin, size);
		return true;
	}

	const QList<FileInfo>& fileInfos = m_torrent->torrentInfo()->fileInfos();
	QList<QFile*>& files = m_torrent->files();

	// Find this block's absolute indexes
	qint64 blockBegin = m_torrent->torrentInfo()->pieceLength();
	blockBegin *= m_pieceNumber;
	blockBegin += begin;
	qint64 blockEnd = blockBegin + size;

	// For each file
	qint64 fileBegin = 0;
	for(int i = 0; i < fileInfos.size(); i++) {
		const FileInfo& fileInfo = fileInfos[i];
		QFile* file = files[i];

		qint64 fileEnd = fileBegin + fileInfo.length;

		// Does this file have any of the needed data?
		if(fileEnd > blockBegin && fileBegin < blockEnd) {
			qint64 seek = 0;
			if(blockBegin - fileBegin > 0) {
				seek = blockBegin - fileBegin;
			}

			// Calculate the number of bytes we have to read from this file
			qint64 bytesToRead = qMin(blockEnd, fileEnd) - qMax(blockBegin, fileBegin);

			// Open file
			if(!file->open(QFile::ReadOnly)) {
				qDebug() << "Failed to open file" << file->fileName() << ":" << file->errorString();
				return false;
			}

			// Seek in the file if needed
			if(seek) {
				if(!file->seek(seek)) {
					qDebug() << "Failed to seek" << seek << "bytes in file" << file->fileName() << ":" << file->errorString();
					file->close();
					return false;
				}
			}

			// Read bytesToRead bytes
			blockData.append(file->read(bytesToRead));

			// Close the file
			file->close();

			// Return if this is the last file
			if(fileEnd >= blockEnd) {
				return true;
			}
		}

		fileBegin += fileInfo.length;
	}
	return true;
}

Block* Piece::getBlock(int begin, int size) const {
	for(Block* block : m_blocks) {
		if(block->begin() == begin && block->size() == size) {
			return block;
		}
	}
	return nullptr;
}
