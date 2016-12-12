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
	QTcpSocket* socket();
	Peer* peer();
	bool timedOut();
	QList<Block*>& blocksQueue();
	void cancelBlock(Block* block);
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

	QList<Block*> m_blocksQueue;

	bool m_amChoking;
	bool m_amInterested;
	bool m_peerChoking;
	bool m_peerInterested;

	/* This flag will be set when the peer hasn't
	 * responded to a request in a certain amount of time */
	bool m_timedOut;

	bool readHandshakeReply();
	bool readPeerMessage();
	bool requestPiece();
	void disconnect();
};

#endif // TORRENTCLIENT_H
