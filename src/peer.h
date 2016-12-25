#ifndef PEER_H
#define PEER_H

#include <QByteArray>
#include <QTimer>
#include <QObject>

class Torrent;
class Block;
class QTcpSocket;

/*
 * This class is used to connect to a peer and communicate with him.
 * It contains all kinds of information about the peer.
 * Is receives and processes messages from the peer.
 * And sends messages/requests to that peer.
 */
class Peer : public QObject {
	Q_OBJECT

public:
	/* The connection statuses */
	enum Status {
		Created, /* Object was just created. Not connecting to the peer */
		Connecting, /* Connecting. Waiting for the connected() slot. */
		Handshaking, /* Connected. In the process of handshaking. */
		ConnectionEstablished /* Handshaking completed. Exchanging messages */
	};

	/* The type of this peer */
	enum PeerType {
		Client, /* This peer is a client, I will upload to him */
		Server  /* This peer is a server, I will download from him */
	};

	/* Getter functions */
	Torrent* torrent();

	QByteArray& address();
	int port();
	bool* bitfield();
	QByteArray& protocol();
	QByteArray& reserved();
	QByteArray& infoHash();
	QByteArray& peerId();

	Status status();
	PeerType peerType();
	bool amChoking();
	bool amInterested();
	bool peerChoking();
	bool peerInterested();

	QTcpSocket* socket();
	bool timedOut();
	QList<Block*>& blocksQueue();

	QString addressPort();
	bool hasPiece(int index);

private:
	Torrent* m_torrent;

	/* Peer-specific information */
	QByteArray m_address;
	int m_port;
	bool* m_bitfield;
	QByteArray m_protocol;
	QByteArray m_reserved;
	QByteArray m_infoHash;
	QByteArray m_peerId;

	/* Connection information */
	Status m_status;
	PeerType m_peerType;
	bool m_amChoking;
	bool m_amInterested;
	bool m_peerChoking;
	bool m_peerInterested;

	/* Networking */
	QTcpSocket* m_socket;
	QByteArray m_receivedDataBuffer;
	QTimer m_replyTimeoutTimer;
	QTimer m_handshakeTimeoutTimer;

	/* This flag will be set when the peer hasn't
	 * responded to a request in a certain amount of time */
	bool m_timedOut;

	/*
	 * The blocks waiting in the queue
	 * When m_peerType is Client: The blocks we should send to him
	 * When m_peerType is Server: The blocks we are waiting to receive
	 */
	QList<Block*> m_blocksQueue;

private:
	/* Try to read handshake reply from the buffer */
	bool readHandshakeReply();

	/* Reads all peer messages in the buffer */
	bool readPeerMessage();

public:
	/* Constructor and destructor */
	Peer(Torrent* torrent, PeerType peerType, const QByteArray& address, int port);
	~Peer();

	/* Attempt to connect to the peer */
	void startConnection();

	/* Cancel request for block */
	void cancelBlock(Block* block);

	/* Attempt to request a block from the Torrent object
	 * and send that request to the peer */
	bool requestBlock();

	/* Drops the connection */
	void disconnect();

public slots:
	void connected();
	void readyRead();
	void finished();
	void replyTimeout();
	void handshakeTimeout();
};

#endif // PEER_H
