#ifndef BLOCK_H
#define BLOCK_H

#include <QByteArray>
#include <QList>

class Piece;
class TorrentClient;

class Block {
	Piece* m_piece;
	int m_begin;
	int m_size;
	bool m_downloaded;

	/* The peers to which this Block is
	 * assigned to be downloaded from. */
	QList<TorrentClient*> m_assignees;
private: // Accessed through Torrent and Block classes
	friend class Torrent;
	friend class Piece;
	Block(Piece* piece, int begin, int size);
	~Block();
	Piece* piece();
	int begin() const;
	int size() const;
	bool downloaded();
	void setDownloaded(bool downloaded);
	void setData(const char* data, int length);

	/* Assignee operations */
	void addAssignee(TorrentClient* peer);
	void removeAssignee(TorrentClient* peer);
	void clearAssignees();
	QList<TorrentClient*>& assignees();
};

#endif // BLOCK_H
