#include "torrentmessage.h"

TorrentMessage::TorrentMessage(Type type) {
	for(int i = 0; i < 4; i++) {
		m_data.push_back((char)0);
	}
	m_data.push_back(type);
}

QByteArray& TorrentMessage::getMessage() {
	int len = m_data.size() - 4;
	for(int i = 0; i < 4; i++) {
		m_data[i] = (unsigned char)(len % 256);
		len /= 256;
	}
	return m_data;
}

void TorrentMessage::addByte(unsigned char value) {
	m_data.push_back(value);
}

void TorrentMessage::addInt32(qint32 value) {
	for(int i = 0; i < 4; i++) {
		m_data.push_back((unsigned char)(value % 256));
		value /= 256;
	}
}

void TorrentMessage::addByteArray(QByteArray value) {
	m_data.push_back(value);
}

QByteArray TorrentMessage::keepAlive() {
	QByteArray arr;
	for(int i = 0; i < 4; i++) {
		arr.push_back((char)0);
	}
	return arr;
}
