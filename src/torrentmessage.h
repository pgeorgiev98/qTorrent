#ifndef TORRENTMESSAGE_H
#define TORRENTMESSAGE_H

#include <QByteArray>
#include <QVector>

class QAbstractSocket;

/* A class, used to generate BitTorrent messages */

class TorrentMessage {
	QByteArray m_data;
public:
	enum Type {
		Choke = 0, Unchoke = 1,
		Interested = 2, NotInterested = 3,
		Have = 4, Bitfield = 5, Request = 6,
		Piece = 7, Cancel = 8, Port = 9
	};

	TorrentMessage(Type type);
	QByteArray& getMessage();
	void addByte(unsigned char value);
	void addInt32(qint32 value);
	void addByteArray(QByteArray value);

	/* Ready-to-use functions for generating messages */
	static void keepAlive(QAbstractSocket* socket);
	static void choke(QAbstractSocket* socket);
	static void unchoke(QAbstractSocket* socket);
	static void interested(QAbstractSocket* socket);
	static void notInterested(QAbstractSocket* socket);
	static void have(QAbstractSocket* socket, int pieceIndex);
	static void bitfield(QAbstractSocket* socket, const QVector<bool>& bitfield);
	static void request(QAbstractSocket* socket, int index, int begin, int length);
	static void piece(QAbstractSocket* socket, int index, int begin, const QByteArray& block);
	static void cancel(QAbstractSocket* socket, int index, int begin, int length);
	static void port(QAbstractSocket* socket, int listenPort);
};

#endif // TORRENTMESSAGE_H
