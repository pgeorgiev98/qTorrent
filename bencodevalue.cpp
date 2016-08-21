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

BencodeInteger::~BencodeInteger() {
}

int BencodeInteger::value() const {
	return m_value;
}

bool BencodeInteger::loadFromByteArray(const QByteArray &data, int &position) {
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
	return ok;
}



BencodeString::BencodeString() : BencodeValue(Type::String) {
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
