#ifndef BENCODEVALUE_H
#define BENCODEVALUE_H

#include <QByteArray>
#include <QTextStream>
#include <QString>
#include <QList>

class Bencode;
class BencodeInteger;
class BencodeString;
class BencodeList;
class BencodeDictionary;

class BencodeException {
	QString m_errorString;
public:
	BencodeException(const QString& errorString) : m_errorString(errorString) {
	}
	const QString& what() const {
		return m_errorString;
	}
};

class BencodeValue {
public:
	enum class Type { Integer, String, List, Dictionary };
protected:
	Type m_type;
	QString m_errorString;
	int m_dataPosBegin;
	int m_dataPosEnd;
	const QByteArray* m_bencodeData;
	void setErrorString(QString errorString);
	void clearErrorString();
public:
	BencodeValue(Type type);
	virtual ~BencodeValue();
	Type type() const;
	QString errorString() const;
	QByteArray getBencodeData(bool includeBeginAndEnd = true);
	virtual bool loadFromByteArray(const QByteArray& data, int& position) = 0;
	static BencodeValue* createFromByteArray(const QByteArray& data, int& position);
	virtual void print(QTextStream& out) const = 0;
	virtual bool equalTo(BencodeValue* other) const = 0;

	void loadFromByteArrayEx(const QByteArray &data, int &position) {
		if(!loadFromByteArray(data, position)) {
			throw BencodeException(m_errorString);
		}
	}

	template<typename T>
	T* castTo() {
		return dynamic_cast<T*>(this);
	}

	template<typename T>
	T* castToEx() {
		auto casted = dynamic_cast<T*>(this);
		if(casted == nullptr) {
			throw BencodeException("Dynamic cast failed");
		}
		return casted;
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
	BencodeValue* getValue(int index);
	BencodeValue* getValueEx(int index) {
		auto val = getValue(index);
		if(val == nullptr) {
			throw BencodeException("Out of range");
		}
		return val;
	}

	template<typename T>
	QList<T*> values() const {
		QList<T*> values;
		for(auto value : m_values) {
			T* v = value -> castTo<T>();
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
	bool keyExists(BencodeValue* key) const;
	bool keyExists(const QByteArray& key) const;
	BencodeValue* value(BencodeValue* key) const;
	BencodeValue* value(const QByteArray& key) const;
	bool equalTo(BencodeValue *other) const;


	BencodeValue* valueEx(BencodeValue* key) const {
		auto val = value(key);
		if(val == nullptr) {
			throw BencodeException("Key does not exist in dictionary");
		}
		return val;
	}
	BencodeValue* valueEx(const QByteArray& key) const {
		auto val = value(key);
		if(val == nullptr) {
			throw BencodeException("Key '" + key + "' does not exist in dictionary");
		}
		return val;
	}
	template<typename T>
	QList<T*> keys() const {
		QList<T*> keys;
		for(auto pair : m_values) {
			T* key = pair.first -> castTo<T>();
			if(key != nullptr) {
				keys.push_back(key);
			}
		}
		return keys;
	}
};

#endif // BENCODEVALUE_H
