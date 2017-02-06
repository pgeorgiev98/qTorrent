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
class ResumeInfo {
public:
	ResumeInfo(TorrentInfo* torrentInfo);

	bool loadFromBencode(BencodeDictionary* dict);

	/* Adds the resume data to the dictionary in the argument */
	void addToBencode(BencodeDictionary* mainResumeDictionary) const;

	/* Getters */
	TorrentInfo* torrentInfo() const;
	const QString& downloadLocation() const;
	qint64 totalBytesDownloaded() const;
	qint64 totalBytesUploaded() const;
	bool paused() const;
	const QVector<bool>& aquiredPieces() const;
	QByteArray aquiredPiecesArray() const;

	/* Setters */
	void setDownloadLocation(const QString& downloadLocation);
	void setTotalBytesDownloaded(qint64 totalBytesDownloaded);
	void setTotalBytesUploaded(qint64 totalBytesUploaded);
	void setPaused(bool paused);
	void setAquiredPieces(const QVector<bool>& aquiredPieces);

private:
	TorrentInfo* m_torrentInfo;
	QString m_downloadLocation;
	qint64 m_totalBytesDownloaded;
	qint64 m_totalBytesUploaded;
	bool m_paused;
	QVector<bool> m_aquiredPieces;

	QVector<bool> toBitArray(const QByteArray& data);
};

#endif // RESUMEINFO_H
