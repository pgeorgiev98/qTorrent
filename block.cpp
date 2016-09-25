#include "block.h"
#include "piece.h"

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

const QByteArray& Block::data() const {
	return m_data;
}

void Block::setData(const QByteArray &data) {
	m_data = data;
	setDownloaded(true);
	m_piece->updateInfo();
}
