#ifndef BLOCK_H
#define BLOCK_H

#include <QByteArray>
#include <QMutex>

class Piece;

class Block {
	Piece* m_piece;
	int m_begin;
	int m_size;
	bool m_downloaded;
	QMutex m_downloadedMutex;
public:
	Block(Piece* piece, int begin, int size);
	~Block();
	Piece* piece();
	int begin() const;
	int size() const;
	bool downloaded();
	void setDownloaded(bool downloaded);
	void setData(const QByteArray& data);
};

#endif // BLOCK_H
