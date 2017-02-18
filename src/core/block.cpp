#include "block.h"
#include "piece.h"
#include "peer.h"
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
	return m_downloaded;
}

void Block::setDownloaded(bool downloaded) {
	m_downloaded = downloaded;
}

void Block::setData(const Peer* peer, const char* data) {
	if(downloaded()) {
		return;
	}

	char* p = m_piece->data() + m_begin;
	for(int i = 0; i < m_size; i++) {
		p[i] = data[i];
	}
	setDownloaded(true);
	for(auto p : m_assignees) {
		if(p != peer) {
			p->sendCancel(this);
		}
		p->releaseBlock(this);
	}
	m_piece->updateState();
}


/* Assignee operations */

void Block::addAssignee(Peer *peer) {
	m_assignees.push_back(peer);
}

void Block::removeAssignee(Peer *peer) {
	for(int i = m_assignees.size() - 1; i >= 0; i--) {
		if(m_assignees[i] == peer) {
			m_assignees.removeAt(i);
		}
	}
}

void Block::clearAssignees() {
	m_assignees.clear();
}

QList<Peer*>& Block::assignees() {
	return m_assignees;
}
