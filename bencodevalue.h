#ifndef BENCODEVALUE_H
#define BENCODEVALUE_H

#include <QByteArray>
#include <QTextStream>
#include <QString>
#include <QList>

class BencodeInteger;
class BencodeString;
class BencodeList;
class BencodeDictionary;

class BencodeValue {
public:
	enum class Type { Integer, String, List, Dictionary };
protected:
	Type m_type;
	QString m_errorString;
	int m_dataPosBegin;
	int m_dataPosEnd;
	void setErrorString(QString errorString);
	void clearErrorString();
public:
	BencodeValue(Type type);
	virtual ~BencodeValue();
	Type type() const;
	QString errorString() const;
	virtual bool loadFromByteArray(const QByteArray& data, int& position) = 0;
	static BencodeValue* createFromByteArray(const QByteArray& data, int& position);
	virtual void print(QTextStream& out) const = 0;
	virtual bool equalTo(BencodeValue* other) const = 0;

	template<typename T>
	T* convertTo() {
		return dynamic_cast<T*>(this);
	}
};

class BencodeInteger : public BencodeValue {
protected:
	int m_value;
public:
	BencodeInteger();
	BencodeInteger(int value);
	~BencodeInteger();
	int value() const;
	bool loadFromByteArray(const QByteArray& data, int& position);
	void print(QTextStream& out) const;
	bool equalTo(BencodeValue *other) const;
};

class BencodeString : public BencodeValue {
protected:
	QByteArray m_value;
public:
	BencodeString();
	BencodeString(const QByteArray& value);
	~BencodeString();
	const QByteArray& value() const;
	QByteArray value();
	bool loadFromByteArray(const QByteArray& data, int& position);
	void print(QTextStream& out) const;
	bool equalTo(BencodeValue *other) const;
};

class BencodeList : public BencodeValue {
protected:
	QList<BencodeValue*> m_values;
public:
	BencodeList();
	~BencodeList();
	const QList<BencodeValue*>& values() const;
	QList<BencodeValue*> values();
	bool loadFromByteArray(const QByteArray& data, int& position);
	void print(QTextStream& out) const;
	bool equalTo(BencodeValue *other) const;

	template<typename T>
	QList<T*> values() const {
		QList<T*> values;
		for(auto value : m_values) {
			T* v = value -> convertTo<T>();
			if(v != nullptr) {
				values.push_back(v);
			}
		}
		return values;
	}
};

class BencodeDictionary : public BencodeValue {
protected:
	QList< QPair<BencodeValue*, BencodeValue*> > m_values;
public:
	BencodeDictionary();
	~BencodeDictionary();
	const QList< QPair<BencodeValue*, BencodeValue*> >& values() const;
	QList< QPair<BencodeValue*, BencodeValue*> > values();
	bool loadFromByteArray(const QByteArray& data, int& position);
	void print(QTextStream& out) const;
	QList<BencodeValue*> keys() const;
	BencodeValue* value(BencodeValue* key) const;
	bool equalTo(BencodeValue *other) const;

	template<typename T>
	QList<T*> keys() const {
		QList<T*> keys;
		for(auto pair : m_values) {
			T* key = pair.first -> convertTo<T>();
			if(key != nullptr) {
				keys.push_back(key);
			}
		}
		return keys;
	}
};

#endif // BENCODEVALUE_H
