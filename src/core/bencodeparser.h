/* qTorrent - An open-source, cross-platform BitTorrent client
 * Copyright (C) 2017 Petko Georgiev
 *
 * bencodeparser.h
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

#ifndef BENCODE_H
#define BENCODE_H

#include "bencodevalue.h"
#include <QByteArray>
#include <QString>
#include <QList>
#include <QTextStream>

class BencodeParser {
	/* Error handling */
	QString m_errorString;
	void setError(const QString& errorString);
	void clearError();

	/* The data to be parsed is stored here */
	QByteArray m_bencodeData;

	/* The main list of bencode values will be stored here */
	QList<BencodeValue*> m_mainList;
public:
	BencodeParser();
	~BencodeParser();

	/* Returns m_errorString */
	QString errorString() const;

	/* Sets m_bencodeData to data */
	void setData(const QByteArray& data);

	/* Stores the data from fileName to m_bencodeData. Returns false on error and sets m_errorString */
	bool readFile(const QString& fileName);

	/* Parses data. Returns false on error */
	bool parse(const QByteArray& data);

	/* Parses m_bencodeData. Returns false on error */
	bool parse();

	/* Returns the raw (not parsed) bencode data as it was read from the file / manually set */
	const QByteArray& rawBencodeData() const;

	/* Returns the main bencode list */
	QList<BencodeValue*> list() const;
};

#endif // BENCODE_H
