#include "piece.h"
#include "block.h"
#include "torrent.h"
#include "torrentinfo.h"
#include <QCryptographicHash>
#include <QDebug>

Piece::Piece(Torrent* torrent, int pieceNumber) :
	m_torrent(torrent),
	m_pieceNumber(pieceNumber),
	m_downloaded(false),
	m_downloading(false)
{
}

Piece::~Piece() {
	for(auto b : m_blocksDownloaded) {
		delete b;
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


void Piece::addBlock(Block *block) {
	int insertPos = 0;
	for(int i = 0; i < m_blocksDownloaded.size(); i++) {
		if(block->begin() < m_blocksDownloaded[i]->begin()) {
			break;
		}
		insertPos++;
	}
	m_blocksDownloaded.insert(insertPos, block);
}

bool Piece::checkIfDownloaded() {
	int pos = 0;
	for(auto b : m_blocksDownloaded) {
		if(b->begin() == pos && b->downloaded()) {
			pos += b->size();
		} else {
			m_downloaded = false;
			return false;
		}
	}
	int size = m_torrent->torrentInfo()->pieceLength();
	m_downloaded = (size == pos);
	return m_downloaded;
}

void Piece::updateInfo() {
	m_updateInfoMutex.lock();
	if(checkIfDownloaded()) {
		QCryptographicHash hash(QCryptographicHash::Sha1);
		for(auto b : m_blocksDownloaded) {
			hash.addData(b->data());
		}
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
			for(auto b : m_blocksDownloaded) {
				delete b;
			}
			m_blocksDownloaded.clear();
			m_downloaded = false;
			qDebug() << "Piece" << m_pieceNumber << "failed SHA1 validation";
		} else {
			qDebug() << "Received piece" << m_pieceNumber << "/" << m_torrent->torrentInfo()->numberOfPieces();
			m_downloaded = true;
			m_downloading = false;
		}
	}
	m_updateInfoMutex.unlock();
}

Block* Piece::requestBlock(int size) {
	m_requestBlockMutex.lock();

	int tmp = 0;
	int s = size;
	Block* block = nullptr;

	for(auto b : m_blocksDownloaded) {
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
		int pieceSize = m_torrent->torrentInfo()->pieceLength();
		if(pieceSize - tmp > size) {
			block = new Block(this, tmp, size);
		} else if(pieceSize - tmp > 0) {
			block = new Block(this, tmp, pieceSize - tmp);
		}
	}
	if(block != nullptr) {
		addBlock(block);
	}

	m_requestBlockMutex.unlock();
	return block;
}
