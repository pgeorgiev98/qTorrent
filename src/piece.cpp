#include "piece.h"
#include "block.h"
#include "torrent.h"
#include "torrentinfo.h"
#include "peer.h"
#include "torrentclient.h"
#include "torrentmessage.h"
#include <QCryptographicHash>
#include <QTcpSocket>
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
	for(auto b : m_blocksDownloaded) {
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
	for(int i = 0; i < m_blocksDownloaded.size(); i++) {
		if(block->begin() < m_blocksDownloaded[i]->begin()) {
			break;
		}
		insertPos++;
	}
	m_blocksDownloaded.insert(insertPos, block);
}

void Piece::deleteBlock(Block* block) {
	int blockNumber = -1;
	for(int i = 0; i < m_blocksDownloaded.size(); i++) {
		if(m_blocksDownloaded[i] == block) {
			blockNumber = i;
			break;
		}
	}
	if(blockNumber != -1) {
		delete m_blocksDownloaded[blockNumber];
		m_blocksDownloaded.removeAt(blockNumber);
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
	for(auto b : m_blocksDownloaded) {
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
			for(auto b : m_blocksDownloaded) {
				delete b;
			}
			m_blocksDownloaded.clear();
			m_downloaded = false;
			qDebug() << "Piece" << m_pieceNumber << "failed SHA1 validation";
		} else {
			//qDebug() << "Received piece" << m_pieceNumber << "/" << m_torrent->torrentInfo()->numberOfPieces();
			int downloadedPieces = m_torrent->downloadedPieces();
			int totalPieces = m_torrent->torrentInfo()->numberOfPieces();
			float percentage = downloadedPieces;
			percentage /= totalPieces;
			percentage *= 100;
			qDebug() << "Downloaded pieces" << downloadedPieces << "/" << totalPieces << "|" << percentage << "%";
			m_torrent->savePiece(m_pieceNumber);
			m_downloaded = true;
			m_downloading = false;
			m_torrent->addToBytesDownloaded(m_size);
			unloadFromMemory();

			// Send 'have' messages to all peers
			for(auto peer : m_torrent->peers()) {
				QTcpSocket* socket = peer->torrentClient()->socket();
				TorrentMessage::have(socket, m_pieceNumber);
			}
		}
	}
}

Block* Piece::requestBlock(int size) {
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
