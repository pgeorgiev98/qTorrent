/* qTorrent - An open-source, cross-platform BitTorrent client
 * Copyright (C) 2017 Petko Georgiev
 *
 * block.cpp
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

#include "block.h"
#include "piece.h"
#include "peer.h"
#include <QDebug>

Block::Block(Piece* piece, int begin, int size) :
	m_piece(piece),
	m_begin(begin),
	m_size(size),
	m_isDownloaded(false)
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

bool Block::isDownloaded() {
	return m_isDownloaded;
}

void Block::setDownloaded(bool downloaded) {
	m_isDownloaded = downloaded;
}

void Block::setData(const Peer* peer, const char* data) {
	if(isDownloaded()) {
		return;
	}

	char* p = m_piece->data() + m_begin;
	for(int i = 0; i < m_size; i++) {
		p[i] = data[i];
	}
	setDownloaded(true);
	QList<Peer*> assignees = m_assignees;
	for(auto p : assignees) {
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

bool Block::hasAssignees() const {
	return !m_assignees.isEmpty();
}
