#include "torrentmessage.h"
#include <QAbstractSocket>

TorrentMessage::TorrentMessage(Type type) {
	for(int i = 0; i < 4; i++) {
		m_data.push_back((char)0);
	}
	m_data.push_back(type);
}

QByteArray& TorrentMessage::getMessage() {
	int len = m_data.size() - 4;
	for(int i = 3; i >= 0; i--) {
		m_data[i] = (unsigned char)(len % 256);
		len /= 256;
	}
	return m_data;
}

void TorrentMessage::addByte(unsigned char value) {
	m_data.push_back(value);
}

void TorrentMessage::addInt32(qint32 value) {
	unsigned char bytes[4];
	for(int i = 3; i >= 0; i--) {
		bytes[i] = (unsigned char)(value % 256);
		value /= 256;
	}
	for(int i = 0; i < 4; i++) {
		m_data.push_back(bytes[i]);
	}
}

void TorrentMessage::addByteArray(QByteArray value) {
	m_data.push_back(value);
}


void TorrentMessage::keepAlive(QAbstractSocket* socket) {
	QByteArray arr;
	for(int i = 0; i < 4; i++) {
		arr.push_back((char)0);
	}
	socket->write(arr);
}

void TorrentMessage::choke(QAbstractSocket* socket) {
	TorrentMessage msg(Choke);
	socket->write(msg.getMessage());
}

void TorrentMessage::unchoke(QAbstractSocket* socket) {
	TorrentMessage msg(Unchoke);
	socket->write(msg.getMessage());
}

void TorrentMessage::interested(QAbstractSocket* socket) {
	TorrentMessage msg(Interested);
	socket->write(msg.getMessage());
}

void TorrentMessage::notInterested(QAbstractSocket* socket) {
	TorrentMessage msg(NotInterested);
	socket->write(msg.getMessage());
}

void TorrentMessage::have(QAbstractSocket* socket, int pieceIndex) {
	TorrentMessage msg(Have);
	msg.addInt32(pieceIndex);
	socket->write(msg.getMessage());
}

void TorrentMessage::bitfield(QAbstractSocket *socket, const QVector<bool> &bitfield) {
	TorrentMessage msg(Bitfield);
	for(int i = 0; i < bitfield.size(); ) {
		unsigned char byte = 0;
		for(int j = 7; j >= 0; j--) {
			int state = bitfield[i++];
			byte ^= (-state ^ byte) & (1 << j);
		}
		msg.addByte(byte);
	}
	socket->write(msg.getMessage());
}

void TorrentMessage::request(QAbstractSocket* socket, int index, int begin, int length) {
	TorrentMessage msg(Request);
	msg.addInt32(index);
	msg.addInt32(begin);
	msg.addInt32(length);
	socket->write(msg.getMessage());
}

void TorrentMessage::piece(QAbstractSocket* socket, int index, int begin, const QByteArray& block) {
	TorrentMessage msg(Piece);
	msg.addInt32(index);
	msg.addInt32(begin);
	msg.addByteArray(block);
	socket->write(msg.getMessage());
}

void TorrentMessage::cancel(QAbstractSocket* socket, int index, int begin, int length) {
	TorrentMessage msg(Cancel);
	msg.addInt32(index);
	msg.addInt32(begin);
	msg.addInt32(length);
	socket->write(msg.getMessage());
}

void TorrentMessage::port(QAbstractSocket* socket, int listenPort) {
	TorrentMessage msg(Port);
	msg.addInt32(listenPort);
	socket->write(msg.getMessage());
}
