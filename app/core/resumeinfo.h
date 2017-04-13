/* qTorrent - An open-source, cross-platform BitTorrent client
 * Copyright (C) 2017 Petko Georgiev
 *
 * resumeinfo.h
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

#ifndef RESUMEINFO_H
#define RESUMEINFO_H

#include <QtGlobal>
#include <QVector>
#include <QByteArray>

class TorrentInfo;
class BencodeDictionary;

/*
 * This class contains the information required to resume a
 * torrent after the application was shut down.
 */
class ResumeInfo
{
public:
	ResumeInfo(TorrentInfo *torrentInfo);

	bool loadFromBencode(BencodeDictionary *dict);

	/* Adds the resume data to the dictionary in the argument */
	void addToBencode(BencodeDictionary *mainResumeDictionary) const;

	/* Getters */
	TorrentInfo *torrentInfo() const;
	const QString &downloadLocation() const;
	qint64 totalBytesDownloaded() const;
	qint64 totalBytesUploaded() const;
	bool paused() const;
	const QVector<bool> &aquiredPieces() const;
	QByteArray aquiredPiecesArray() const;

	/* Setters */
	void setDownloadLocation(const QString &downloadLocation);
	void setTotalBytesDownloaded(qint64 totalBytesDownloaded);
	void setTotalBytesUploaded(qint64 totalBytesUploaded);
	void setPaused(bool paused);
	void setAquiredPieces(const QVector<bool> &aquiredPieces);

private:
	TorrentInfo *m_torrentInfo;
	QString m_downloadLocation;
	qint64 m_totalBytesDownloaded;
	qint64 m_totalBytesUploaded;
	bool m_paused;
	QVector<bool> m_aquiredPieces;

	QVector<bool> toBitArray(const QByteArray &data);
};

#endif // RESUMEINFO_H
