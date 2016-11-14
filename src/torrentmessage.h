#ifndef TORRENTMESSAGE_H
#define TORRENTMESSAGE_H

#include <QByteArray>

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
	static QByteArray keepAlive();
};

#endif // TORRENTMESSAGE_H
