#include "bencodevalue.h"
#include <QDebug>

BencodeValue::BencodeValue(Type type)
	: m_type(type)
	, m_dataPosBegin(0)
	, m_dataPosEnd(0)
	, m_bencodeData(nullptr)
{
}

BencodeValue::~BencodeValue() {
}


BencodeValue::Type BencodeValue::type() const {
	return m_type;
}


bool BencodeValue::isInteger() const {
	return m_type == Type::Integer;
}

bool BencodeValue::isString() const {
	return m_type == Type::String;
}

bool BencodeValue::isList() const {
	return m_type == Type::List;
}

bool BencodeValue::isDictionary() const {
	return m_type == Type::Dictionary;
}


BencodeInteger* BencodeValue::toBencodeInteger() {
	if(!isInteger()) {
		QString errorString;
		QTextStream err(&errorString);
		err << "BencodeValue::toBencodeInteger(): Value is not an integer: ";
		print(err);
		throw BencodeException(errorString);
	}
	return static_cast<BencodeInteger*>(this);
}

BencodeString* BencodeValue::toBencodeString() {
	if(!isString()) {
		QString errorString;
		QTextStream err(&errorString);
		err << "bencodeValue::toBencodeString(): Value is not an string: ";
		print(err);
		throw BencodeException(errorString);
	}
	return static_cast<BencodeString*>(this);
}

BencodeList* BencodeValue::toBencodeList() {
	if(!isList()) {
		QString errorString;
		QTextStream err(&errorString);
		err << "BencodeValue::toBencodeList(): Value is not an list: ";
		print(err);
		throw BencodeException(errorString);
	}
	return static_cast<BencodeList*>(this);
}

BencodeDictionary* BencodeValue::toBencodeDictionary() {
	if(!isDictionary()) {
		QString errorString;
		QTextStream err(&errorString);
		err << "BencodeValue::toBencodeDictionary(): Value is not an dictionary";
		print(err);
		throw BencodeException(errorString);
	}
	return static_cast<BencodeDictionary*>(this);
}


qint64 BencodeValue::toInt() {
	return toBencodeInteger()->toInt();
}

QByteArray BencodeValue::toByteArray() {
	return toBencodeString()->toByteArray();
}

QList<BencodeValue*> BencodeValue::toList() {
	return toBencodeList()->toList();
}


QByteArray BencodeValue::getRawBencodeData(bool includeBeginAndEnd) {
	QByteArray returnData;
	int begin = m_dataPosBegin;
	int end = m_dataPosEnd;

	if(!includeBeginAndEnd) {
		begin++;
		end--;
	}

	for(int i = begin; i < end; i++) {
		returnData.push_back(m_bencodeData->at(i));
	}

	return returnData;
}


BencodeValue* BencodeValue::createFromByteArray(const QByteArray &data, int &position) {
	BencodeException ex("BencodeValue::createFromByteArray(): ");

	if(position >= data.size()) {
		throw ex << "Unexpectedly reached end of the data stream";
	}

	BencodeValue *value;
	char firstByte = data[position];
	if(firstByte == 'i') {
		value = new BencodeInteger;
	} else if(firstByte >= '0' && firstByte <= '9') {
		value = new BencodeString;
	} else if(firstByte == 'l') {
		value = new BencodeList;
	} else if(firstByte == 'd') {
		value = new BencodeDictionary;
	} else {
		throw ex << "Invalid begining character for bencode value: "
				 << "'" << firstByte << "'."
				 << "Expected 'i', 'l', 'd' or a digit.";
	}
	try {
		value->loadFromByteArray(data, position);
	} catch(BencodeException& ex2) {
		delete value;
		throw ex << "Failed to load value" << endl
				 << ex2.what();
	}
	return value;
}



BencodeInteger::BencodeInteger() : BencodeValue(Type::Integer) {
}

BencodeInteger::BencodeInteger(qint64 value)
	: BencodeValue(Type::Integer)
	, m_value(value)
{
}

BencodeInteger::~BencodeInteger() {
}

qint64 BencodeInteger::toInt() {
	return m_value;
}

void BencodeInteger::loadFromByteArray(const QByteArray &data, int &position) {
	BencodeException ex("BencodeInteger::loadFromByteArray(): ");

	m_bencodeData = &data;
	m_dataPosBegin = position;
	int &i = position;
	if(i >= data.size()) {
		throw ex << "Unexpectedly reached end of the data stream";
	}

	char firstByte = data[i++];
	if(firstByte != 'i') {
		throw ex << "First byte of Integer must be 'i', insted got '" << firstByte << "'";
	}

	QString valueString;
	for(;;) {
		if(i == data.size()) {
			throw ex << "Unexpectedly reached end of the data stream";
		}
		char byte = data[i++];
		if(byte == 'e') {
			break;
		}
		if((byte < '0' || byte > '9') && byte != '-') {
			throw ex << "Illegal character: '" << byte << "'";
		}
		valueString += byte;
	}
	bool ok;
	m_value = valueString.toLongLong(&ok);
	m_dataPosEnd = i;
	if(!ok) {
		throw ex << "Value not an integer: '" << valueString << "'";
	}
}



BencodeString::BencodeString() : BencodeValue(Type::String) {
}

BencodeString::BencodeString(const QByteArray& value)
	: BencodeValue(Type::String)
	, m_value(value)
{
}

BencodeString::~BencodeString() {
}

QByteArray BencodeString::toByteArray() {
	return m_value;
}

void BencodeString::loadFromByteArray(const QByteArray &data, int &position) {
	BencodeException ex("BencodeString::loadFromByteArray(): ");

	m_bencodeData = &data;
	m_dataPosBegin = position;
	int& i = position;
	if(i >= data.size()) {
		throw ex << "Unexpectedly reached end of the data stream";
	}

	char firstByte = data[i];
	if(firstByte < '0' || firstByte > '9') {
		throw ex << "First byte must be a digit, but got '" << firstByte << "'";
	}

	QString lengthString;
	for(;;) {
		if(i == data.size()) {
			throw ex << "Unexpectedly reached end of the data stream";
		}
		char byte = data[i++];
		if(byte == ':') {
			break;
		}
		if((byte < '0' || byte > '9') && byte != '-') {
			throw ex << "Illegal character: '" << byte << "'";
		}
		lengthString += byte;
	}
	bool ok;
	int length = lengthString.toInt(&ok);
	if(!ok) {
		throw ex << "Length not an integer: '" << lengthString << "'";
	}

	for(int j = 0; j < length; j++) {
		if(i == data.size()) {
			throw ex << "Unexpectedly reached end of the data stream";
		}
		char byte = data[i++];
		m_value += byte;
	}
	m_dataPosEnd = i;
}



BencodeList::BencodeList() : BencodeValue(Type::List) {
}

BencodeList::~BencodeList() {
}

QList<BencodeValue*> BencodeList::toList() {
	return m_values;
}

void BencodeList::loadFromByteArray(const QByteArray &data, int &position) {
	BencodeException ex("BencodeList::loadFromByteArray(): ");

	m_bencodeData = &data;
	m_dataPosBegin = position;
	int& i = position;
	if(i >= data.size()) {
		throw ex << "Unexpectedly reached end of the data stream";
	}

	char firstByte = data[i++];
	if(firstByte != 'l') {
		throw ex << "First byte of list must be 'l', instead got '" << firstByte << "'";
	}

	for(;;) {
		if(i >= data.size()) {
			throw ex << "Unexpectedly reached end of the data stream";
		}
		if(data[i] == 'e') {
			i++;
			break;
		}

		BencodeValue* element;
		try {
			element = BencodeValue::createFromByteArray(data, i);
		} catch(BencodeException& ex2) {
			throw ex << "Failed to create element" << endl << ex2.what();
		}

		m_values.push_back(element);
	}
	m_dataPosEnd = i;
}



BencodeDictionary::BencodeDictionary() : BencodeValue(Type::Dictionary) {
}

BencodeDictionary::~BencodeDictionary() {
}

QList<BencodeValue*> BencodeDictionary::keys() const {
	QList<BencodeValue*> keys;
	for(auto pair : m_values) {
		keys.push_back(pair.first);
	}
	return keys;
}

bool BencodeDictionary::keyExists(BencodeValue* key) const {
	for(auto pair : m_values) {
		if(pair.first == key) {
			return true;
		}
		if(pair.first->equalTo(key)) {
			return true;
		}
	}
	return false;
}

bool BencodeDictionary::keyExists(const QByteArray& key) const {
	BencodeString convertedKey(key);
	return keyExists(&convertedKey);
}

BencodeValue* BencodeDictionary::value(BencodeValue *key) const {
	for(auto pair : m_values) {
		if(pair.first == key) {
			return pair.second;
		}
		if(pair.first -> equalTo(key)) {
			return pair.second;
		}
	}
	throw BencodeException("BencodeDictionary::value(): No such key");
}

BencodeValue* BencodeDictionary::value(const QByteArray& key) const {
	BencodeString convertedKey(key);
	return value(&convertedKey);
}

void BencodeDictionary::loadFromByteArray(const QByteArray &data, int &position) {
	BencodeException ex("BencodeDictionary::loadFromByteArray(): ");

	m_bencodeData = &data;
	m_dataPosBegin = position;
	int& i = position;
	if(i == data.size()) {
		throw ex << "Unexpectedly reached end of the data stream";
	}
	char firstByte = data[i++];
	if(firstByte != 'd') {
		throw ex << "First byte of a dictionary must be 'd', instead got '" << firstByte << "'";
	}
	for(;;) {
		if(i >= data.size()) {
			throw ex << "Unexpectedly reached end of the data stream";
		}
		if(data[i] == 'e') {
			i++;
			break;
		}
		BencodeValue* first;
		BencodeValue* second;
		try {
			first = BencodeValue::createFromByteArray(data, i);
		} catch(BencodeException& ex2) {
			throw ex << "Failed to load first value" << endl << ex2.what();
		}

		try {
			second = BencodeValue::createFromByteArray(data, i);
		} catch(BencodeException& ex2) {
			throw ex << "Failed to load second value" << endl << ex2.what();
		}

		QPair<BencodeValue*, BencodeValue*> pair(first, second);
		m_values.push_back(pair);
	}
	m_dataPosEnd = i;
}


void BencodeInteger::print(QTextStream& out) const {
	out << m_value;
}

void BencodeString::print(QTextStream& out) const {
	out << m_value;
}

void BencodeList::print(QTextStream& out) const {
	out << "List {" << endl;
	for(auto v : m_values) {
		QString s;
		QTextStream stream(&s);
		v -> print(stream);
		while(!stream.atEnd()) {
			QString line = stream.readLine();
			out << '\t' << line << endl;
		}
	}
	out << "}";
}

void BencodeDictionary::print(QTextStream& out) const {
	out << "Dictionary {" << endl;
	for(auto v : m_values) {
		QString s;
		QTextStream stream(&s);
		(v.first) -> print(stream);
		while(!stream.atEnd()) {
			out << '\t' << stream.readLine();
		}
		(v.second) -> print(stream);
		out << " : ";
		out << stream.readLine() << endl;
		while(!stream.atEnd()) {
			out << '\t' << stream.readLine() << endl;
		}
	}
	out << "}";
}


bool BencodeInteger::equalTo(BencodeValue *other) const {
	try {
		return other->toInt() == m_value;
	} catch(BencodeException& ex) {
		return false;
	}
}

bool BencodeString::equalTo(BencodeValue *other) const {
	try {
		return other->toByteArray() == m_value;
	} catch(BencodeException& ex) {
		return false;
	}
}

bool BencodeList::equalTo(BencodeValue *other) const {
	try {
		auto list = other->toList();
		if(list.size() != m_values.size()) {
			return false;
		}
		for(int i = 0; i < list.size(); i++) {
			if(!list[i]->equalTo(m_values[i])) {
				return false;
			}
		}
		return true;
	} catch(BencodeException& ex) {
		return false;
	}
}

bool BencodeDictionary::equalTo(BencodeValue *other) const {
	try {
		BencodeDictionary* otherDict = other->toBencodeDictionary();
		if(m_values.size() != otherDict->m_values.size()) {
			return false;
		}
		for(int i = 0; i < m_values.size(); i++) {
			if(!m_values[i].first->equalTo(otherDict->m_values[i].first) ||
				!m_values[i].second->equalTo(otherDict->m_values[i].second)) {
				return false;
			}
		}
		return true;
	} catch(BencodeException& ex) {
		return false;
	}
}
