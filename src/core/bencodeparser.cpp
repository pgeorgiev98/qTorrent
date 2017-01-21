#include "bencodeparser.h"
#include "bencodevalue.h"
#include <QFile>

void BencodeParser::setError(const QString& errorString) {
	m_errorString = errorString;
}

void BencodeParser::clearError() {
	m_errorString.clear();
}


BencodeParser::BencodeParser() {
}

BencodeParser::~BencodeParser() {
}

void BencodeParser::setData(const QByteArray &data) {
	m_bencodeData = data;
}

bool BencodeParser::readFile(const QString& fileName) {
	clearError();

	QFile file(fileName);
	if(!file.open(QIODevice::ReadOnly)) {
		setError(file.errorString());
		return false;
	}
	m_bencodeData = file.readAll();
	file.close();
	return true;
}

bool BencodeParser::parse(const QByteArray &data) {
	setData(data);
	return parse();
}

bool BencodeParser::parse() {
	clearError();

	int i = 0;
	while(i < m_bencodeData.size()) {
		BencodeValue *value;
		try {
			value = BencodeValue::createFromByteArray(m_bencodeData, i);
		} catch(BencodeException& ex) {
			setError(ex.what());
			return false;
		}

		m_values.push_back(value);
	}

	return true;
}

void BencodeParser::print(QTextStream &out) const {
	for(auto v : m_values) {
		v -> print(out);
		out << endl;
	}
}

QString BencodeParser::errorString() const {
	return m_errorString;
}

const QByteArray& BencodeParser::rawBencodeData() const {
	return m_bencodeData;
}
