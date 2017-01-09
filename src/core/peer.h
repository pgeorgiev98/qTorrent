#ifndef PEER_H
#define PEER_H

#include <QByteArray>
#include <QTimer>
#include <QObject>
#include <QAbstractSocket>

class QTorrent;
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
		ConnectionEstablished, /* Handshaking completed. Exchanging messages */
		Disconnected, /* Disconnected from the peer */
		Error /* An error has occurred */
	};

	/* The type of this peer */
	enum PeerType {
		Client, /* This peer is a client, I will upload to him */
		Server  /* This peer is a server, I will download from him */
	};

	/* Getter functions */
	QTorrent* qTorrent();
	Torrent* torrent();

	QByteArray& address();
	int port();
	int piecesDownloaded();
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
	bool downloaded();
	bool hasPiece(int index);
	bool isConnected();

private:
	QTorrent* m_qTorrent;
	Torrent* m_torrent;

	/* Peer-specific information */
	QByteArray m_address;
	int m_port;
	int m_piecesDownloaded;
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
	QTimer m_reconnectTimer;

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
	/* Try to read handshake reply from the buffer
	 * Returns true on successful message parse, false on
	 * error or incomplete message.
	 * On error, ok is set to false, otherwise - to true */
	bool readHandshakeReply(bool* ok);

	/* Reads all peer messages in the buffer
	 * Returns true on successful message parse, false on
	 * error or incomplete message.
	 * On error, ok is set to false, otherwise - to true */
	bool readPeerMessage(bool* ok);

	/* Connects all needed SIGNALs (from m_socket and for the timeouts) to the public slots */
	void connectAll();

	/* Creates the bitfield array and initializes it */
	void initBitfield();

	/* Initializes/Reinitializes connection with client */
	void initClient(QTorrent* qTorrent);

	/* Initializes/Reinitializes connection with server */
	void initServer(Torrent* torrent, const QByteArray& address, int port);

public:
	/* Constructor and destructor */
	Peer(PeerType peerType, QTcpSocket* socket);
	~Peer();

	/* Attempt to connect to the peer */
	void startConnection();

	/* Send message */
	void sendHandshake();
	void sendChoke();
	void sendUnchoke();
	void sendInterested();
	void sendNotInterested();
	void sendHave(int index);
	void sendBitfield();
	void sendRequest(Block* block);
	void sendPiece(int index, int begin, const QByteArray& blockData);
	void sendCancel(Block* block);

	/* Attempt to request a block from the Torrent object
	 * and send that request to the peer */
	bool requestBlock();

	/* Drops the connection */
	void disconnect();

	/* An fatal error has occurred; drops the connection */
	void fatalError();

	/* Returns a newly-created peer object with peerType = Client (He downloads from us) */
	static Peer* createClient(QTorrent* qTorrent, QTcpSocket* socket);

	/* Returns a newly-created peer object with peerType = Server (We download from him) */
	static Peer* createServer(Torrent* torrent, const QByteArray& address, int port);

	/* Attempts to send messages to the peer */
	void sendMessages();

public slots:
	void connected();
	void readyRead();
	void finished();
	void error(QAbstractSocket::SocketError socketError);
	void replyTimeout();
	void handshakeTimeout();
	void reconnect();
};

#endif // PEER_H
