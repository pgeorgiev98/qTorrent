#ifndef BLOCK_H
#define BLOCK_H

#include <QByteArray>
#include <QList>

class Piece;
class Peer;

class Block {
	Piece* m_piece;
	int m_begin;
	int m_size;
	bool m_downloaded;

	/* The peers to which this Block is
	 * assigned to be downloaded from. */
	QList<Peer*> m_assignees;
public:
	Block(Piece* piece, int begin, int size);
	~Block();
	Piece* piece();
	int begin() const;
	int size() const;
	bool downloaded();
	void setDownloaded(bool downloaded);
	void setData(const Peer* peer, const char* data);

	/* Assignee operations */
	void addAssignee(Peer* peer);
	void removeAssignee(Peer* peer);
	void clearAssignees();
	QList<Peer*>& assignees();
};

#endif // BLOCK_H
