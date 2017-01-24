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
