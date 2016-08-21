#ifndef BENCODE_H
#define BENCODE_H

#include "bencodevalue.h"
#include <QByteArray>
#include <QString>

class Bencode {
	QList<BencodeValue*> m_values;
	QString m_errorString;
	void setError(QString errorString);
	void clearError();
public:
	Bencode();
	~Bencode();
	bool loadFromFile(QString fileName);
	bool loadFromByteArray(const QByteArray& data);
	QString errorString() const;
	const QList<BencodeValue*>& values() const;
	QList<BencodeValue*> values();
	void print(QTextStream& out) const;
};

#endif // BENCODE_H
