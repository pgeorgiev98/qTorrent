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
public:
	Bencode();
	~Bencode();
	bool loadFromFile(QString fileName);
	bool loadFromByteArray(const QByteArray& data);
	QString errorString() const;
	const QByteArray& bencodeData() const;
	void print(QTextStream& out) const;
};

#endif // BENCODE_H
