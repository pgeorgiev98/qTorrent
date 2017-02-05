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

Piece::Piece(Torrent* torrent, int pieceNumber, int size) :
	m_torrent(torrent),
	m_pieceNumber(pieceNumber),
	m_size(size),
	m_downloaded(false),
	m_downloading(false),
	m_pieceData(new char[size])
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

bool Piece::downloading() const {
	return m_downloading;
}

bool Piece::downloaded() const {
	return m_downloaded;
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

bool Piece::checkIfDownloaded() {
	if(m_downloaded) { // If already marked as downloaded, don't do anything
		return true;
	}
	if(m_pieceData == nullptr) {
		qDebug() << "Fatal: Piece::checkIfDownloaded(" << this << ") - piece is not loaded!";
		exit(1);
	}
	int pos = 0;
	for(auto b : m_blocks) {
		if(b->begin() == pos && b->downloaded()) {
			pos += b->size();
		} else {
			m_downloaded = false;
			return false;
		}
	}
	m_downloaded = (m_size == pos);
	return m_downloaded;
}

void Piece::updateInfo() {
	if(checkIfDownloaded()) {
		if(m_pieceData == nullptr) {
			qDebug() << "Fatal error in Piece::updateInfo(" << this << ") - Piece is not loaded!";
			exit(1);
		}
		QCryptographicHash hash(QCryptographicHash::Sha1);
		hash.addData(m_pieceData, m_size);
		const QByteArray& validHash = m_torrent->torrentInfo()->pieces();
		QByteArray actualHash = hash.result();
		bool isValid = true;
		for(int i = 0, j = m_pieceNumber*actualHash.size(); i < actualHash.size(); i++, j++) {
			if(actualHash[i] != validHash[j]) {
				isValid = false;
				break;
			}
		}
		if(!isValid) {
			for(auto b : m_blocks) {
				delete b;
			}
			m_blocks.clear();
			m_downloaded = false;
			qDebug() << "Piece" << m_pieceNumber << "failed SHA1 validation";
		} else {
			m_torrent->savePiece(m_pieceNumber);
			m_downloaded = true;
			m_downloading = false;
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
		if(!b->downloaded() && b->assignees().isEmpty()) {
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
	if(m_pieceData == nullptr) {
		qDebug() << "Fatal: Tried to unload piece" << this << "(" << m_pieceNumber << "), which is not loaded!";
		exit(1);
	}
	if(!checkIfDownloaded()) {
		qDebug() << "Fatal: Tried to unload piece" << this << "(" << m_pieceNumber << "), which is not downloaded yet!";
		exit(1);
	}
	delete m_pieceData;
	m_pieceData = nullptr;
}

void Piece::setDownloaded(bool downloaded) {
	m_downloaded = downloaded;
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
	qint64 blockBegin = 0;
	for(const Piece* piece : m_torrent->pieces()) {
		if(piece == this) {
			break;
		}
		blockBegin += piece->size();
	}
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

			qint64 bytesToRead = fileInfo.length;

			// Is this the last file?
			if(fileEnd >= blockEnd) {
				// Read until the end of the block
				if(blockBegin > fileBegin) {
					bytesToRead = size;
				} else {
					bytesToRead = blockEnd - fileBegin;
				}
			}

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
