/* qTorrent - An open-source, cross-platform BitTorrent client
 * Copyright (C) 2017 Petko Georgiev
 *
 * torrentinfo.h
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

#ifndef TORRENTINFO_H
#define TORRENTINFO_H

#include <QList>
#include <QString>
#include <QDateTime>

struct FileInfo {
	QList<QString> path;
	qint64 length;
};

class TorrentInfo {
	QString m_errorString;
	void clearError();
	void setError(QString errorString);

	QList<QByteArray> m_announceUrlsList;

	qint64 m_length;
	QByteArray m_torrentName;
	qint64 m_pieceLength;
	QList<QByteArray> m_pieces;

	QDateTime* m_creationDate;
	QString* m_comment;
	QString* m_createdBy;
	QString* m_encoding;

	QList<FileInfo> m_fileInfos;

	QByteArray m_infoHash;

	int m_numberOfPieces;
public:
	QString errorString() const;
	bool loadFromTorrentFile(QString filename);

	const QList<QByteArray>& announceUrlsList() const;

	qint64 length() const;
	const QByteArray& torrentName() const;
	qint64 pieceLength() const;
	const QList<QByteArray>& pieces() const;
	const QByteArray& piece(int pieceIndex) const;

	const QDateTime* creationDate() const;
	const QString* comment() const;
	const QString* createdBy() const;
	const QString* encoding() const;

	const QList<FileInfo>& fileInfos() const;

	const QByteArray& infoHash() const;
	QByteArray infoHashPercentEncoded() const;

	int numberOfPieces() const;
	int bitfieldSize() const;

	TorrentInfo();
	~TorrentInfo();
};

#endif // TORRENTINFO_H
