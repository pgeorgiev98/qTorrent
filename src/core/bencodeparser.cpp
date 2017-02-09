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
	for(BencodeValue* value : m_mainList) {
		delete value;
	}
}


QString BencodeParser::errorString() const {
	return m_errorString;
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

	for(BencodeValue* value : m_mainList) {
		delete value;
	}
	m_mainList.clear();

	int i = 0;
	while(i < m_bencodeData.size()) {
		BencodeValue *value;
		try {
			value = BencodeValue::createFromByteArray(m_bencodeData, i);
		} catch(BencodeException& ex) {
			setError(ex.what());
			return false;
		}

		m_mainList.push_back(value);
	}

	return true;
}


const QByteArray& BencodeParser::rawBencodeData() const {
	return m_bencodeData;
}


QList<BencodeValue*> BencodeParser::list() const {
	return m_mainList;
}
