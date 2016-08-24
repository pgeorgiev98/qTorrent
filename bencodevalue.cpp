#include "bencodevalue.h"
#include <QDebug>

BencodeValue::BencodeValue(Type type) : m_type(type) {
}

BencodeValue::~BencodeValue() {
}

QString BencodeValue::errorString() const {
	return m_errorString;
}

BencodeValue::Type BencodeValue::type() const {
	return m_type;
}

void BencodeValue::setErrorString(QString errorString) {
	m_errorString = errorString;
}

void BencodeValue::clearErrorString() {
	m_errorString.clear();
}

QByteArray BencodeValue::getBencodeData(bool includeBeginAndEnd) {
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
	if(position >= data.size()) {
		return nullptr;
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
		return nullptr;
	}
	if(!value -> loadFromByteArray(data, position)) {
		delete value;
		return nullptr;
	}
	return value;
}



BencodeInteger::BencodeInteger() : BencodeValue(Type::Integer) {
}

BencodeInteger::BencodeInteger(int value) : BencodeValue(Type::Integer), m_value(value) {
}

BencodeInteger::~BencodeInteger() {
}

int BencodeInteger::value() const {
	return m_value;
}

bool BencodeInteger::loadFromByteArray(const QByteArray &data, int &position) {
	m_bencodeData = &data;
	m_dataPosBegin = position;
	int &i = position;
	if(i >= data.size()) {
		return false;
	}
	char firstByte = data[i++];
	if(firstByte != 'i') {
		return false;
	}
	QString valueString;
	for(;;) {
		if(i == data.size()) {
			return false;
		}
		char byte = data[i++];
		if(byte == 'e') {
			break;
		}
		if((byte < '0' || byte > '9') && byte != '-') {
			return false;
		}
		valueString += byte;
	}
	bool ok;
	m_value = valueString.toInt(&ok);
	m_dataPosEnd = i;
	return ok;
}



BencodeString::BencodeString() : BencodeValue(Type::String) {
}

BencodeString::BencodeString(const QByteArray& value) : BencodeValue(Type::String), m_value(value) {
}

BencodeString::~BencodeString() {
}

const QByteArray& BencodeString::value() const {
	return m_value;
}

QByteArray BencodeString::value() {
	return m_value;
}

bool BencodeString::loadFromByteArray(const QByteArray &data, int &position) {
	m_bencodeData = &data;
	m_dataPosBegin = position;
	int& i = position;
	if(i >= data.size()) {
		return false;
	}
	char firstByte = data[i];
	if(firstByte < '0' || firstByte > '9') {
		return false;
	}
	QString lengthString;
	for(;;) {
		if(i == data.size()) {
			return false;
		}
		char byte = data[i++];
		if(byte == ':') {
			break;
		}
		if((byte < '0' || byte > '9') && byte != '-') {
			return false;
		}
		lengthString += byte;
	}
	bool ok;
	int length = lengthString.toInt(&ok);
	if(!ok) {
		return false;
	}

	for(int j = 0; j < length; j++) {
		if(i == data.size()) {
			return false;
		}
		char byte = data[i++];
		m_value += byte;
	}
	m_dataPosEnd = i;
	return true;
}



BencodeList::BencodeList() : BencodeValue(Type::List) {
}

BencodeList::~BencodeList() {
}

const QList<BencodeValue*>& BencodeList::values() const {
	return m_values;
}

QList<BencodeValue*> BencodeList::values() {
	return m_values;
}

bool BencodeList::loadFromByteArray(const QByteArray &data, int &position) {
	m_bencodeData = &data;
	m_dataPosBegin = position;
	int& i = position;
	if(i >= data.size()) {
		return false;
	}
	char firstByte = data[i++];
	if(firstByte != 'l') {
		return false;
	}
	for(;;) {
		if(i >= data.size()) {
			return false;
		}
		if(data[i] == 'e') {
			i++;
			break;
		}
		BencodeValue* element = BencodeValue::createFromByteArray(data, i);
		if(element == nullptr) {
			return false;
		}
		m_values.push_back(element);
	}
	m_dataPosEnd = i;
	return true;
}



BencodeDictionary::BencodeDictionary() : BencodeValue(Type::Dictionary) {
}

BencodeDictionary::~BencodeDictionary() {
}

const QList< QPair<BencodeValue*, BencodeValue*> >& BencodeDictionary::values() const {
	return m_values;
}

QList< QPair<BencodeValue*, BencodeValue*> > BencodeDictionary::values() {
	return m_values;
}

bool BencodeDictionary::loadFromByteArray(const QByteArray &data, int &position) {
	m_bencodeData = &data;
	m_dataPosBegin = position;
	int& i = position;
	if(i == data.size()) {
		return false;
	}
	char firstByte = data[i++];
	if(firstByte != 'd') {
		return false;
	}
	for(;;) {
		if(i >= data.size()) {
			return false;
		}
		if(data[i] == 'e') {
			i++;
			break;
		}
		BencodeValue* first = BencodeValue::createFromByteArray(data, i);
		if(first == nullptr) {
			return false;
		}
		BencodeValue* second = BencodeValue::createFromByteArray(data, i);
		if(second == nullptr) {
			return false;
		}
		QPair<BencodeValue*, BencodeValue*> pair(first, second);
		m_values.push_back(pair);
	}
	m_dataPosEnd = i;
	return true;
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

QList<BencodeValue*> BencodeDictionary::keys() const {
	QList<BencodeValue*> keys;
	for(auto pair : m_values) {
		keys.push_back(pair.first);
	}
	return keys;
}

bool BencodeDictionary::keyExists(BencodeValue* key) const {
	return (value(key) != nullptr);
}

bool BencodeDictionary::keyExists(const QByteArray& key) const {
	return (value(key) != nullptr);
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
	return nullptr;
}

BencodeValue* BencodeDictionary::value(const QByteArray& key) const {
	BencodeString convertedKey(key);
	return value(&convertedKey);
}



bool BencodeInteger::equalTo(BencodeValue *other) const {
	BencodeInteger* otherInt = other -> convertTo<BencodeInteger>();
	if(otherInt == nullptr) {
		return false;
	}
	return m_value == otherInt -> m_value;
}

bool BencodeString::equalTo(BencodeValue *other) const {
	BencodeString* otherString = other -> convertTo<BencodeString>();
	if(otherString == nullptr) {
		return false;
	}
	return m_value == otherString -> m_value;
}

bool BencodeList::equalTo(BencodeValue *other) const {
	BencodeList* otherList = other -> convertTo<BencodeList>();
	if(otherList == nullptr) {
		return false;
	}
	if(m_values.size() != otherList -> m_values.size()) {
		return false;
	}
	for(int i = 0; i < m_values.size(); i++) {
		if(!m_values[i]->equalTo(otherList->m_values[i])) {
			return false;
		}
	}
	return true;
}

bool BencodeDictionary::equalTo(BencodeValue *other) const {
	BencodeDictionary* otherDictionary = other -> convertTo<BencodeDictionary>();
	if(otherDictionary == nullptr) {
		return false;
	}
	if(m_values.size() != otherDictionary -> m_values.size()) {
		return false;
	}
	for(int i = 0; i < m_values.size(); i++) {
		if(!m_values[i].first->equalTo(otherDictionary->m_values[i].first) ||
			!m_values[i].second->equalTo(otherDictionary->m_values[i].second)) {
			return false;
		}
	}
	return true;
}
