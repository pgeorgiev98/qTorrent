#ifndef BENCODEVALUE_H
#define BENCODEVALUE_H

#include <QByteArray>
#include <QString>
#include <QList>

class BencodeValue {
public:
	enum class Type { Integer, String, List, Dictionary };
private:
	Type m_type;
	QString m_errorString;
	void setErrorString(QString errorString);
	void clearErrorString();
public:
	BencodeValue(Type type);
	virtual ~BencodeValue();
	Type type() const;
	QString errorString() const;
	virtual bool loadFromByteArray(const QByteArray& data, int& position) = 0;
	static BencodeValue* createFromByteArray(const QByteArray& data, int& position);
};

class BencodeInteger : public BencodeValue {
	int m_value;
public:
	BencodeInteger();
	~BencodeInteger();
	int value() const;
	bool loadFromByteArray(const QByteArray& data, int& position);
};

class BencodeString : public BencodeValue {
	QByteArray m_value;
public:
	BencodeString();
	~BencodeString();
	const QByteArray& value() const;
	QByteArray value();
	bool loadFromByteArray(const QByteArray& data, int& position);
};

class BencodeList : public BencodeValue {
	QList<BencodeValue*> m_values;
public:
	BencodeList();
	~BencodeList();
	const QList<BencodeValue*>& values() const;
	QList<BencodeValue*> values();
	bool loadFromByteArray(const QByteArray& data, int& position);
};

class BencodeDictionary : public BencodeValue {
	QList< QPair<BencodeValue*, BencodeValue*> > m_values;
public:
	BencodeDictionary();
	~BencodeDictionary();
	const QList< QPair<BencodeValue*, BencodeValue*> >& values() const;
	QList< QPair<BencodeValue*, BencodeValue*> > values();
	bool loadFromByteArray(const QByteArray& data, int& position);
};

#endif // BENCODEVALUE_H
