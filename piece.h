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
	QList<Block*> m_blocksDownloaded;
	QMutex m_accessPieceMutex;
	void addBlock(Block* block);
	bool checkIfDownloaded();
public:
	Piece(Torrent* torrent, int pieceNumber, int size);
	~Piece();
	bool downloading() const;
	bool downloaded() const;
	int pieceNumber() const;
	Block* requestBlock(int size);
	void updateInfo();
	void deleteBlock(Block* block);
	QByteArray data() const;
	int size() const;
};

#endif // PIECE_H
