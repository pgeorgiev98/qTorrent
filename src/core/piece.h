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
	bool m_downloading;
	char* m_pieceData;
	QList<Block*> m_blocks;

	void addBlock(Block* block);
	bool checkIfDownloaded();
public:
	Piece(Torrent* torrent, int pieceNumber, int size);
	~Piece();

	/* Getters */

	bool downloading() const;
	bool downloaded() const;
	int pieceNumber() const;
	char* data() const;
	int size() const;

	/* Commands */

	Block* requestBlock(int size);
	void updateInfo();
	void deleteBlock(Block* block);
	void unloadFromMemory();
	/* Gets data for a block. Reads from files if needed. */
	bool getBlockData(int begin, int size, QByteArray& blockData);
};

#endif // PIECE_H
