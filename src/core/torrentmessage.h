/* qTorrent - An open-source, cross-platform BitTorrent client
 * Copyright (C) 2017 Petko Georgiev
 *
 * torrentmessage.h
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef TORRENTMESSAGE_H
#define TORRENTMESSAGE_H

#include <QByteArray>
#include <QVector>

class QAbstractSocket;

/* A class, used to generate BitTorrent messages */

class TorrentMessage
{
	QByteArray m_data;

public:
	enum Type {
		Choke = 0, Unchoke = 1,
		Interested = 2, NotInterested = 3,
		Have = 4, Bitfield = 5, Request = 6,
		Piece = 7, Cancel = 8, Port = 9
	};

	TorrentMessage(Type type);
	QByteArray &getMessage();
	void addByte(unsigned char value);
	void addInt32(qint32 value);
	void addByteArray(QByteArray value);

	/* Ready-to-use functions for generating messages */
	static void keepAlive(QAbstractSocket *socket);
	static void choke(QAbstractSocket *socket);
	static void unchoke(QAbstractSocket *socket);
	static void interested(QAbstractSocket *socket);
	static void notInterested(QAbstractSocket *socket);
	static void have(QAbstractSocket *socket, int pieceIndex);
	static void bitfield(QAbstractSocket *socket, const QVector<bool> &bitfield);
	static void request(QAbstractSocket *socket, int index, int begin, int length);
	static void piece(QAbstractSocket *socket, int index, int begin, const QByteArray &block);
	static void cancel(QAbstractSocket *socket, int index, int begin, int length);
	static void port(QAbstractSocket *socket, int listenPort);
};

#endif // TORRENTMESSAGE_H
