#ifndef TORRENTINFO_H
#define TORRENTINFO_H

#include <QString>
#include <QDateTime>

class FileInfo {
public:
	QString path;
	qint64 length;
};

class TorrentInfo {
	QString m_errorString;
	void clearError();
	void setError(QString errorString);

	QByteArray m_announceUrl;
	QDateTime m_creationDate;
	QString m_encoding;

	qint64 m_length;
	QByteArray m_torrentName;
	qint64 m_pieceLength;
	QByteArray m_pieces;

	QList<FileInfo> m_fileInfos;

	QByteArray m_infoHash;
public:
	QString errorString() const;
	bool loadTorrentFile(QString filename);

	const QByteArray& announceUrl() const;
	QDateTime creationDate() const;
	QString encoding() const;

	qint64 length() const;
	const QByteArray& torrentName() const;
	qint64 pieceLength() const;
	const QByteArray& pieces() const;

	const QByteArray& infoHash() const;
	QByteArray infoHashPercentEncoded() const;

	TorrentInfo();
	~TorrentInfo();
};

#endif // TORRENTINFO_H
