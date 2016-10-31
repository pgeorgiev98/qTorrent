#ifndef PIECE_H
#define PIECE_H

#include <QList>
#include <QMutex>

class Torrent;
class Block;

class Piece {
	Torrent* m_torrent;
	int m_pieceNumber;
	int m_size;
	bool m_downloaded;
	bool m_downloading;
	char* m_pieceData;
	QList<Block*> m_blocksDownloaded;
	void addBlock(Block* block);
	bool checkIfDownloaded();
private: // Accessed through Torrent and Block classes
	friend class Torrent;
	friend class Block;
	Piece(Torrent* torrent, int pieceNumber, int size);
	~Piece();
	bool downloading() const;
	bool downloaded() const;
	int pieceNumber() const;
	Block* requestBlock(int size);
	void updateInfo();
	void deleteBlock(Block* block);
	void unloadFromMemory();
	char* data() const;
	int size() const;
};

#endif // PIECE_H
