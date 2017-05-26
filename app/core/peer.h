/* qTorrent - An open-source, cross-platform BitTorrent client
 * Copyright (C) 2017 Petko Georgiev
 *
 * peer.h
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

#ifndef PEER_H
#define PEER_H

#include <QByteArray>
#include <QHostAddress>
#include <QTimer>
#include <QObject>
#include <QAbstractSocket>

class Torrent;
class Piece;
class Block;
class QTcpSocket;

/*
 * This class is used to connect to a peer and communicate with him.
 * It contains all kinds of information about the peer.
 * Is receives and processes messages from the peer.
 * And sends messages/requests to that peer.
 */
class Peer : public QObject
{
	Q_OBJECT

public:
	/* The connection statuses */
	enum State {
		Created, /* Object was just created. Not connecting to the peer */
		Connecting, /* Connecting. Waiting for the connected() slot. */
		Handshaking, /* Connected. In the process of handshaking. */
		ConnectionEstablished, /* Handshaking completed. Exchanging messages */
		Disconnected, /* Disconnected from the peer */
		Error /* An error has occurred */
	};

	/* The type of this peer */
	enum class ConnectionInitiator
	{
		Client, /* We connected to the peer */
		Peer    /* The peeer connected to us */
	};

	/* Getter functions */
	Torrent *torrent();

	QHostAddress address();
	int port();
	int piecesDownloaded();
	bool *bitfield();
	QByteArray &protocol();
	QByteArray &reserved();
	QByteArray &infoHash();
	QByteArray &peerId();

	State state();
	ConnectionInitiator connectionInitiator();
	bool amChoking();
	bool amInterested();
	bool peerChoking();
	bool peerInterested();

	QTcpSocket *socket();
	bool hasTimedOut();
	QList<Block *> &blocksQueue();
	bool isPaused() const;

	QString addressPort();
	bool isDownloaded();
	bool hasPiece(Piece *piece);
	bool isConnected();
	bool isInteresting();

private:
	Torrent *m_torrent;

	/* Peer-specific information */
	QHostAddress m_address;
	int m_port;
	int m_piecesDownloaded;
	bool *m_bitfield;
	QByteArray m_protocol;
	QByteArray m_reserved;
	QByteArray m_infoHash;
	QByteArray m_peerId;

	/* Connection information */
	State m_state;
	ConnectionInitiator m_connectionInitiator;
	bool m_amChoking;
	bool m_amInterested;
	bool m_peerChoking;
	bool m_peerInterested;

	/* Networking */
	QTcpSocket *m_socket;
	QByteArray m_receivedDataBuffer;
	QTimer m_replyTimeoutTimer;
	QTimer m_handshakeTimeoutTimer;
	QTimer m_reconnectTimer;

	/* Used to make sure that sendMessages() will ce called at
	 * least every SEND_MESSAGES_INTERVAL milliseconds */
	QTimer m_sendMessagesTimer;

	/* This flag will be set when the peer hasn't
	 * responded to a request in a certain amount of time */
	bool m_hasTimedOut;

	/* The blocks that we have requested */
	QList<Block *> m_blocksQueue;

	/* Is downloading/uploading paused */
	bool m_isPaused;

	/* Try to read handshake reply from the buffer
	 * Returns true on successful message parse, false on
	 * error or incomplete message.
	 * On error, ok is set to false, otherwise - to true */
	bool readHandshakeReply(bool *ok);

	/* Reads all peer messages in the buffer
	 * Returns true on successful message parse, false on
	 * error or incomplete message.
	 * On error, ok is set to false, otherwise - to true */
	bool readPeerMessage(bool *ok);

	/* Connects all needed SIGNALs (from m_socket and for the timeouts) to the public slots */
	void connectAll();

	/* Creates the bitfield array and initializes it */
	void initBitfield();

	/* Initializes variables for client peer (ConnectionInitiator::Peer) */
	void initClient();

	/* Initializes variables for server peer (ConnectionInitiator::Client) */
	void initServer(Torrent *torrent, QHostAddress address, int port);

public:
	/* Constructor and destructor */
	Peer(ConnectionInitiator connectionInitiator, QTcpSocket *socket);
	~Peer();

	/* Returns a newly-created peer object with peerType = Client (He downloads from us) */
	static Peer *createClient(QTcpSocket *socket);

	/* Returns a newly-created peer object with peerType = Server (We download from him) */
	static Peer* createServer(Torrent *torrent, QHostAddress address, int port);

signals:
	void uploadedData(qint64 bytes);
	void downloadedData(qint64 bytes);

public slots:
	/* Attempt to connect to the peer */
	void startConnection();

	/* Start downloading/uploading */
	void start();
	/* Pause download/upload */
	void pause();

	/* Send message */
	void sendHandshake();
	void sendChoke();
	void sendUnchoke();
	void sendInterested();
	void sendNotInterested();
	void sendHave(int index);
	void sendBitfield();
	void sendRequest(Block *block);
	void sendPiece(int index, int begin, const QByteArray &blockData);
	void sendCancel(Block *block);

	/* Attempt to request a block from the Torrent object
	 * and send that request to the peer */
	bool requestBlock();

	/* Does the opposite of requesting a block */
	void releaseBlock(Block *block);
	void releaseAllBlocks();

	/* Drops the connection */
	void disconnect();

	/* An fatal error has occurred; drops the connection */
	void fatalError();
	/* Attempts to send messages to the peer */
	void sendMessages();

	void connected();
	void readyRead();
	void finished();
	void error(QAbstractSocket::SocketError socketError);
	void replyTimeout();
	void handshakeTimeout();
	void reconnect();
};

#endif // PEER_H
