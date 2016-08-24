#ifndef BENCODE_H
#define BENCODE_H

#include "bencodevalue.h"
#include <QByteArray>
#include <QString>
#include <QList>
#include <QTextStream>

class Bencode : public BencodeList {
	QString m_errorString;
	QByteArray m_bencodeData;
	void setError(QString errorString);
	void clearError();
	bool loadFromByteArray(const QByteArray& data);
public:
	Bencode();
	~Bencode();
	bool loadFromFile(QString fileName);
	QString errorString() const;
	const QByteArray& bencodeData() const;
	void print(QTextStream& out) const;
};

#endif // BENCODE_H
