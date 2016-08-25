#ifndef TORRENTINFO_H
#define TORRENTINFO_H

#include <QString>
#include <QDateTime>

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

	QByteArray m_infoHash;
public:
	QString errorString() const;
	bool loadTorrentFile(QString filename);
	TorrentInfo();
	~TorrentInfo();
};

#endif // TORRENTINFO_H
