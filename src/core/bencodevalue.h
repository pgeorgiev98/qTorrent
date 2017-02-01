#ifndef BENCODEVALUE_H
#define BENCODEVALUE_H

#include <QByteArray>
#include <QTextStream>
#include <QString>
#include <QList>

class BencodeParser;
class BencodeInteger;
class BencodeString;
class BencodeList;
class BencodeDictionary;

class BencodeException {
	QString m_errorString;
public:
	BencodeException(const QString& errorString) : m_errorString(errorString) {}
	BencodeException() {}

	template<typename T>
	BencodeException& operator<<(const T& toAppend) {
		QTextStream stream(&m_errorString);
		stream << toAppend;
		return *this;
	}

	const QString& what() const {
		return m_errorString;
	}
};

class BencodeValue {
public:
	enum class Type { Integer, String, List, Dictionary };

protected:
	// Stores the type of the value
	Type m_type;

	// The location of this value in the main byte array
	int m_dataPosBegin;
	int m_dataPosEnd;
	const QByteArray* m_bencodeData;

	// Loads this value by reading from data, starting from position index
	// Throws BencodeException on error
	virtual void loadFromByteArray(const QByteArray& data, int& position) = 0;
public:
	BencodeValue(Type type);
	virtual ~BencodeValue();

	// Returns the type of the value
	Type type() const;

	bool isInteger() const;
	bool isString() const;
	bool isList() const;
	bool isDictionary() const;

	// Conversion functions
	// All of these throw BencodeException on error

	BencodeInteger* toBencodeInteger();
	BencodeString* toBencodeString();
	BencodeList* toBencodeList();
	BencodeDictionary* toBencodeDictionary();

	virtual qint64 toInt();
	virtual QByteArray toByteArray();
	virtual QList<BencodeValue*> toList();

	// Bencodes the value
	virtual QByteArray bencode(bool includeMetadata = true) const = 0;

	// Returns the bencoded version of this value (used for calculating torrents info_hash)
	QByteArray getRawBencodeData(bool includeMetadata = true);

	// Creates a new BencodeValue by reading from data from position index
	// Throws BencodeException on error
	static BencodeValue* createFromByteArray(const QByteArray& data, int& position);

	virtual void print(QTextStream& out) const = 0;
	virtual bool equalTo(BencodeValue* other) const = 0;
};

class BencodeInteger : public BencodeValue {
protected:
	qint64 m_value;

	void loadFromByteArray(const QByteArray& data, int& position);

public:
	BencodeInteger();
	BencodeInteger(qint64 value);
	~BencodeInteger();

	qint64 toInt();
	void setValue(qint64 value);
	QByteArray bencode(bool includeMetadata) const;
	void print(QTextStream& out) const;
	bool equalTo(BencodeValue *other) const;
};

class BencodeString : public BencodeValue {
protected:
	QByteArray m_value;

	void loadFromByteArray(const QByteArray& data, int& position);

public:
	BencodeString();
	BencodeString(const QByteArray& value);
	~BencodeString();

	QByteArray toByteArray();
	void setValue(const QByteArray& value);
	QByteArray bencode(bool includeMetadata) const;
	void print(QTextStream& out) const;
	bool equalTo(BencodeValue *other) const;
};

class BencodeList : public BencodeValue {
protected:
	QList<BencodeValue*> m_values;

	void loadFromByteArray(const QByteArray& data, int& position);

public:
	BencodeList();
	~BencodeList();

	QList<BencodeValue*> toList();
	void setValues(const QList<BencodeValue*>& values);
	void add(BencodeValue* value);
	QByteArray bencode(bool includeMetadata) const;
	void print(QTextStream& out) const;
	bool equalTo(BencodeValue *other) const;
};

class BencodeDictionary : public BencodeValue {
protected:
	QList< QPair<BencodeValue*, BencodeValue*> > m_values;

	void loadFromByteArray(const QByteArray& data, int& position);

public:
	BencodeDictionary();
	~BencodeDictionary();

	void print(QTextStream& out) const;
	bool equalTo(BencodeValue *other) const;

	QList<BencodeValue*> keys() const;
	bool keyExists(BencodeValue* key) const;
	bool keyExists(const QByteArray& key) const;
	BencodeValue* value(BencodeValue* key) const;
	BencodeValue* value(const QByteArray& key) const;
	void setPairs(const QList< QPair<BencodeValue*, BencodeValue*> >& pairs);
	void add(BencodeValue* key, BencodeValue* value);
	QByteArray bencode(bool includeMetadata) const;
};

#endif // BENCODEVALUE_H
