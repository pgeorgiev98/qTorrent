#ifndef TORRENTCLIENT_H
#define TORRENTCLIENT_H

#include "torrentinfo.h"
#include <QByteArray>
#include <QTimer>
#include <QObject>

class Peer;
class Block;
class QTcpSocket;

class TorrentClient : public QObject {
	Q_OBJECT

public:
	enum Status {
		Created, Connecting, Handshaking, ConnectionEstablished
	};

	TorrentClient(Peer* peer);
	~TorrentClient();
	void connectToPeer();
	Peer* peer();
public slots:
	void connected();
	void readyRead();
	void finished();
	void replyTimeout();
	void handshakeTimeout();
private:
	QTcpSocket* m_socket;
	QTimer m_replyTimeoutTimer;
	QTimer m_handshakeTimeoutTimer;
	Peer* m_peer;
	QByteArray m_receivedData;
	Status m_status;

	Block* m_waitingForBlock;

	bool m_amChoking;
	bool m_amInterested;
	bool m_peerChoking;
	bool m_peerInterested;

	bool readHandshakeReply();
	bool readPeerMessage();
	void requestPiece();
	void disconnect();
};

#endif // TORRENTCLIENT_H
