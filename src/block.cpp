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
	bool tmp = m_downloaded;
	return tmp;
}

void Block::setDownloaded(bool downloaded) {
	m_downloaded = downloaded;
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


/* Assignee operations */

void Block::addAssignee(TorrentClient *peer) {
	m_assignees.push_back(peer);
}

void Block::removeAssignee(TorrentClient *peer) {
	for(int i = m_assignees.size() - 1; i >= 0; i--) {
		if(m_assignees[i] == peer) {
			m_assignees.removeAt(i);
		}
	}
}

void Block::clearAssignees() {
	m_assignees.clear();
}

QList<TorrentClient*>& Block::assignees() {
	return m_assignees;
}
