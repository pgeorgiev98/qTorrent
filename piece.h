#ifndef PIECE_H
#define PIECE_H

#include <QList>
#include <QMutex>

class Torrent;
class Block;

class Piece {
	Torrent* m_torrent;
    int m_pieceNumber;
	bool m_downloaded;
	bool m_downloading;
    QList<Block*> m_blocksDownloaded;
    QMutex m_requestBlockMutex;
    QMutex m_updateInfoMutex;
    void addBlock(Block* block);
    bool checkIfDownloaded();
public:
    Piece(Torrent* torrent, int pieceNumber);
	~Piece();
    bool downloading() const;
    bool downloaded() const;
    int pieceNumber() const;
    Block* requestBlock(int size);
    void updateInfo();
};

#endif // PIECE_H
