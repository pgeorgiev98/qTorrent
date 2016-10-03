#include "block.h"
#include "piece.h"
#include <QDebug>

Block::Block(Piece* piece, int begin, int size) :
	m_piece(piece),
	m_begin(begin),
	m_size(size),
	m_downloaded(false)
{
}

Block::~Block() {
}

Piece* Block::piece() {
	return m_piece;
}

int Block::begin() const {
	return m_begin;
}

int Block::size() const {
	return m_size;
}

bool Block::downloaded() {
	m_downloadedMutex.lock();
	bool tmp = m_downloaded;
	m_downloadedMutex.unlock();
	return tmp;
}

void Block::setDownloaded(bool downloaded) {
	m_downloadedMutex.lock();
	m_downloaded = downloaded;
	m_downloadedMutex.unlock();
}

void Block::setData(const char* data, int length) {
	if(length != m_size) {
		qDebug() << "Error: Block::setData() - Data length " << length << ", expected " << m_size;
		exit(1);
	}
	char* p = m_piece->data() + m_begin;
	for(int i = 0; i < length; i++) {
		p[i] = data[i];
	}
	setDownloaded(true);
	m_piece->updateInfo();
}
