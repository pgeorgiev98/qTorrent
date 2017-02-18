#ifndef PIECE_H
#define PIECE_H

#include <QList>
#include <QByteArray>

class Torrent;
class Block;

class Piece {
	Torrent* m_torrent;
	int m_pieceNumber;
	int m_size;
	bool m_downloaded;
	char* m_pieceData;
	QList<Block*> m_blocks;

	void addBlock(Block* block);
	bool checkIfFullyDownloaded();
public:
	Piece(Torrent* torrent, int pieceNumber, int size);
	~Piece();

	/* Getters */

	bool downloaded() const;
	int pieceNumber() const;
	char* data() const;
	int size() const;

	/* Commands */

	Block* requestBlock(int size);
	// Update the block state: check if it's fully downloaded
	void updateState();
	void deleteBlock(Block* block);
	void unloadFromMemory();
	void setDownloaded(bool downloaded);
	// Gets data for a block. Reads from files if needed
	bool getBlockData(int begin, int size, QByteArray& blockData);
	// Returns a pointer to an existing block or nullptr if no such block exists
	Block* getBlock(int begin, int size) const;
};

#endif // PIECE_H
