#ifndef BENCODE_H
#define BENCODE_H

#include "bencodevalue.h"
#include <QByteArray>
#include <QString>
#include <QList>
#include <QTextStream>

class BencodeParser : public BencodeList {
	/* Error handling */
	QString m_errorString;
	void setError(const QString& errorString);
	void clearError();

	/* The data to be parsed is stored here */
	QByteArray m_bencodeData;
public:
	BencodeParser();
	~BencodeParser();

	/* Sets m_bencodeData to data */
	void setData(const QByteArray& data);

	/* Stores the data from fileName to m_bencodeData. Returns false on error and sets m_errorString */
	bool readFile(const QString& fileName);

	/* Parses data. Returns false on error */
	bool parse(const QByteArray& data);

	/* Parses m_bencodeData. Returns false on error */
	bool parse();

	QString errorString() const;
	const QByteArray& rawBencodeData() const;
	void print(QTextStream& out) const;
};

#endif // BENCODE_H
