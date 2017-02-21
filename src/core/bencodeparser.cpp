/* qTorrent - An open-source, cross-platform BitTorrent client
 * Copyright (C) 2017 Petko Georgiev
 *
 * bencodeparser.cpp
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

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
