#include "bencode.h"
#include <QFile>

void Bencode::setError(QString errorString) {
	m_errorString = errorString;
}

void Bencode::clearError() {
	m_errorString.clear();
}


Bencode::Bencode() {
}

Bencode::~Bencode() {
}

bool Bencode::loadFromFile(QString fileName) {
	clearError();

	QFile file(fileName);
	if(!file.open(QIODevice::ReadOnly)) {
		setError(file.errorString());
		return false;
	}
	QByteArray fileData = file.readAll();
	file.close();
	return loadFromByteArray(fileData);
}

bool Bencode::loadFromByteArray(const QByteArray &data) {
	clearError();
	int i = 0;
	for(;;) {
		if(i == data.size()) {
			break;
		}
		BencodeValue *value = BencodeValue::createFromByteArray(data, i);
		if(value == nullptr) {
			setError("Invalid bencode");
			return false;
		}
		m_values.push_back(value);
	}
	return true;
}

void Bencode::print(QTextStream &out) const {
	for(auto v : m_values) {
		v -> print(out);
		out << endl;
	}
}

QString Bencode::errorString() const {
	return m_errorString;
}

const QList<BencodeValue*>& Bencode::values() const {
	return m_values;
}

QList<BencodeValue*> Bencode::values() {
	return m_values;
}
