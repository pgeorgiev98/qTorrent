#include "peer.h"
#include "block.h"
#include "piece.h"
#include "qtorrent.h"
#include "torrent.h"
#include "torrentinfo.h"
#include "torrentmessage.h"
#include <QTcpSocket>
#include <QHostAddress>
#include <QTimer>
#include <QDebug>

const int BLOCK_REQUEST_SIZE = 16384;
const int REPLY_TIMEOUT_MSEC = 10000;
const int HANDSHAKE_TIMEOUT_MSEC = 20000;
const int BLOCKS_TO_REQUEST = 5;
const int MAX_MESSAGE_LENGTH = 65536;
const int RECONNECT_INTERVAL_MSEC = 30000;

Peer::Peer(PeerType peerType, QTcpSocket* socket)
	: m_torrent(nullptr)
	, m_bitfield(nullptr)
	, m_status(Created)
	, m_peerType(peerType)
	, m_socket(socket)
	, m_paused(false)
{
	connectAll();
}

Peer::~Peer() {
	delete[] m_bitfield;
	delete m_socket;
}


void Peer::startConnection() {
	if(m_peerType == Client) {
		// That's not how it works
		// It's a client's duty to connect to you, not yours!
		qDebug() << "Error in Peer::startConnection(): called on Client Peer";
		return;
	}
	if(m_socket->isOpen()) {
		// Already connected/connecting
		m_socket->close();
	}

	m_piecesDownloaded = 0;
	for(int i = 0; i < m_torrent->torrentInfo()->bitfieldSize() * 8; i++) {
		m_bitfield[i] = false;
	}
	m_protocol.clear();
	m_reserved.clear();
	m_infoHash.clear();
	m_peerId.clear();

	m_status = Connecting;
	m_amChoking = true;
	m_amInterested = false;
	m_peerChoking = true;
	m_peerInterested = false;

	m_receivedDataBuffer.clear();
	m_replyTimeoutTimer.stop();
	m_handshakeTimeoutTimer.stop();
	m_reconnectTimer.stop();

	m_timedOut = false;
	m_blocksQueue.clear();

	qDebug() << "Connecting to" << addressPort();
	m_socket->connectToHost(m_address, m_port);
}

void Peer::start() {
	m_paused = false;
	if(m_status == ConnectionEstablished) {
		sendMessages();
	} else if(m_socket->state() == QAbstractSocket::UnconnectedState) {
		// UnconnectedState - Not connected/connecting
		startConnection();
	}
}

void Peer::pause() {
	m_paused = true;
	sendMessages();
}

void Peer::sendHandshake() {
	QByteArray dataToWrite;
	dataToWrite.push_back(char(19));
	dataToWrite.push_back("BitTorrent protocol");
	for(int i = 0; i < 8; i++) {
		dataToWrite.push_back(char(0));
	}
	dataToWrite.push_back(m_torrent->torrentInfo()->infoHash());
	dataToWrite.push_back(m_qTorrent->peerId());
	m_socket->write(dataToWrite);
}

void Peer::sendChoke() {
	if(m_status != ConnectionEstablished) {
		return;
	}
	m_amChoking = true;
	TorrentMessage::choke(m_socket);
}

void Peer::sendUnchoke() {
	if(m_status != ConnectionEstablished) {
		return;
	}
	m_amChoking = false;
	TorrentMessage::unchoke(m_socket);
}

void Peer::sendInterested() {
	if(m_status != ConnectionEstablished) {
		return;
	}
	m_amInterested = true;
	TorrentMessage::interested(m_socket);
}

void Peer::sendNotInterested() {
	if(m_status != ConnectionEstablished) {
		return;
	}
	m_amInterested = false;
	TorrentMessage::notInterested(m_socket);
}

void Peer::sendHave(int index) {
	if(m_status != ConnectionEstablished) {
		return;
	}
	TorrentMessage::have(m_socket, index);
}

void Peer::sendBitfield() {
	if(m_status != ConnectionEstablished) {
		return;
	}
	TorrentMessage::bitfield(m_socket, m_torrent->bitfield());
}

void Peer::sendRequest(Block* block) {
	if(m_status != ConnectionEstablished) {
		return;
	}

	// Assign this block to yourself
	block->addAssignee(this);

	// Send request
	int index = block->piece()->pieceNumber();
	int begin = block->begin();
	int length = block->size();
	qDebug() << "Request" << index << begin << length << "from" << addressPort();
	TorrentMessage::request(m_socket, index, begin, length);

	// Start/Reset the replyTimeoutTimer
	m_replyTimeoutTimer.start();

	// Insert requested block into the queue
	m_blocksQueue.push_back(block);
}

void Peer::sendPiece(int index, int begin, const QByteArray &blockData) {
	if(m_status != ConnectionEstablished) {
		return;
	}
	qDebug() << "Sending piece" << index << begin << blockData.size() << "to" << addressPort();
	TorrentMessage::piece(m_socket, index, begin, blockData);
}

void Peer::sendCancel(Block* block) {
	if(m_status != ConnectionEstablished) {
		return;
	}
	for(int i = m_blocksQueue.size() - 1; i >= 0; i--) {
		if(m_blocksQueue[i] == block) {
			m_blocksQueue.removeAt(i);
		}
	}
	int	index = block->piece()->pieceNumber();
	int begin = block->begin();
	int length = block->size();
	TorrentMessage::cancel(m_socket, index, begin, length);
}

bool Peer::requestBlock() {
	Block* block = m_torrent->requestBlock(this, BLOCK_REQUEST_SIZE);
	if(block == nullptr) {
		return false;
	}
	sendRequest(block);
	return true;
}

void Peer::disconnect() {
	qDebug() << "Disconnecting from" << addressPort();
	if(isConnected()) {
		m_socket->close();
		// The finished() slot should be called automatically
	} else {
		finished();
	}
}

void Peer::fatalError() {
	qDebug() << "Fatal error with" << addressPort() << "; Dropping connection";
	m_status = Error;
	m_socket->close();
}

Peer* Peer::createClient(QTorrent *qTorrent, QTcpSocket *socket) {
	Peer* peer = new Peer(Client, socket);
	peer->initClient(qTorrent);
	return peer;
}

Peer* Peer::createServer(Torrent *torrent, const QByteArray &address, int port) {
	Peer* peer = new Peer(Server, new QTcpSocket);
	peer->initServer(torrent, address, port);
	return peer;
}

void Peer::sendMessages() {
	// Can't do anything if not connected
	if(m_status != ConnectionEstablished) {
		return;
	}

	// If both of us have the full torrent, disconnect
	if(m_torrent->downloaded() && downloaded()) {
		disconnect();
		return;
	}

	if(m_paused) {
		/* Paused */

		// Paused means we are not interested in anybody
		if(m_amInterested) {
			sendNotInterested();
		}
		// If we're paused, choke everybody
		if(!m_amChoking) {
			sendChoke();
		}
		// Cancel all blocks
		for(Block* block : m_blocksQueue) {
			sendCancel(block);
			block->removeAssignee(this);
		}

	} else {
		/* Not Paused */

		// We are always interested [TODO]
		if(!m_amInterested) {
			sendInterested();
		}

		// Unchoke if peer is interested
		if(m_peerInterested && m_amChoking) {
			sendUnchoke();
		}

		// Request as many bloks as we can if we are not choked
		if(!m_peerChoking) {
			while(m_blocksQueue.size() < BLOCKS_TO_REQUEST) {
				if(!requestBlock()) {
					break;
				}
			}
		}

	}
}


bool Peer::readHandshakeReply(bool *ok) {
	*ok = true;

	if(m_receivedDataBuffer.isEmpty()) {
		return false;
	}
	int i = 0;
	int protocolLength = m_receivedDataBuffer[i++];
	if(m_receivedDataBuffer.size() < 49 + protocolLength) {
		return false;
	}

	for(int j = 0; j < protocolLength; j++) {
		m_protocol.push_back(m_receivedDataBuffer[i++]);
	}
	for(int j = 0; j < 8; j++) {
		m_reserved.push_back(m_receivedDataBuffer[i++]);
	}
	for(int j = 0; j < 20; j++) {
		m_infoHash.push_back(m_receivedDataBuffer[i++]);
	}
	for(int j = 0; j < 20; j++) {
		m_peerId.push_back(m_receivedDataBuffer[i++]);
	}
	m_receivedDataBuffer.remove(0, 49 + protocolLength);

	if(m_peerType == Server) {
		// Check if info hash matches expected one
		if(m_infoHash != m_torrent->torrentInfo()->infoHash()) {
			// Info hash does not match the expected one
			qDebug() << "Info hash does not match expected one from peer" << addressPort();
			*ok = false;
			return false;
		}
	} else {
		// Find torrent with correct info hash
		m_torrent = nullptr;
		for(auto torrent : m_qTorrent->torrents()) {
			if(torrent->torrentInfo()->infoHash() == m_infoHash) {
				m_torrent = torrent;
				break;
			}
		}

		if(m_torrent == nullptr) {
			qDebug() << "No torrents matching info hash" << m_infoHash.toHex() << "for" << addressPort();
			*ok = false;
			return false;
		}
	}
	return true;
}

bool Peer::readPeerMessage(bool* ok) {
	*ok = true;

	// Smallest message (keep-alive) has a size of 4
	if(m_receivedDataBuffer.size() < 4) {
		return false;
	}

	// Counter for m_receivedDataBuffer
	int i = 0;

	// Calculate message length
	int length = 0;
	for(int j = 0; j < 4; j++) {
		length *= 256;
		length += (unsigned char)m_receivedDataBuffer[i++];
	}

	// Check for errors
	if(length > MAX_MESSAGE_LENGTH || length < 0) {
		*ok = false;
		return false;
	}

	if(length == 0) { // keep-alive
		qDebug() << addressPort() << ": keep-alive";
		m_receivedDataBuffer.remove(0, i);
		return true;
	}

	// Have we received the whole message?
	if(m_receivedDataBuffer.size() < 4 + length) {
		return false;
	}

	int messageId = m_receivedDataBuffer[i++];
	switch(messageId) {
	case TorrentMessage::Choke:
	{
		qDebug() << addressPort() << ": choke";
		m_peerChoking = true;
		for(auto block : m_blocksQueue) {
			m_torrent->releaseBlock(this, block);
		}
		m_blocksQueue.clear();
		m_replyTimeoutTimer.stop();
		m_timedOut = false;
		break;
	}
	case TorrentMessage::Unchoke:
	{
		qDebug() << addressPort() << ": unchoke";
		m_peerChoking = false;
		break;
	}
	case TorrentMessage::Interested:
	{
		qDebug() << addressPort() << ": interested";
		m_peerInterested = true;
		break;
	}
	case TorrentMessage::NotInterested:
	{
		qDebug() << addressPort() << ": not interested";
		m_peerInterested = false;
		break;
	}
	case TorrentMessage::Have:
	{
		int pieceNumber = 0;
		for(int j = 0; j < 4; j++) {
			pieceNumber *= 256;
			pieceNumber += (unsigned char)m_receivedDataBuffer[i++];
		}
		if(!m_bitfield[pieceNumber]) {
			m_bitfield[pieceNumber] = true;
			m_piecesDownloaded++;
		}
		break;
	}
	case TorrentMessage::Bitfield:
	{
		int bitfieldSize = length - 1;
		if(bitfieldSize != m_torrent->torrentInfo()->bitfieldSize()) {
			qDebug() << "Error: Peer" << addressPort() << "sent bitfield of wrong size:" << bitfieldSize*8
					 << "expected" << m_torrent->torrentInfo()->bitfieldSize();
			*ok = false;
			return false;
		} else {
			// Set the bitfield
			for(int j = 0; j < bitfieldSize; j++) {
				unsigned char byte = m_receivedDataBuffer[i++];
				unsigned char pos = 0b10000000;
				for(int q = 0; q < 8; q++) {
					m_bitfield[j*8 + q] = ((byte & pos) != 0);
					pos = pos >> 1;
				}
			}

			// Recount the pieces
			m_piecesDownloaded = 0;
			for(int j = 0; j < bitfieldSize*8; j++) {
				if(m_bitfield[j]) {
					m_piecesDownloaded++;
				}
			}
		}
		break;
	}
	case TorrentMessage::Request:
	{
		unsigned int index = 0;
		unsigned int begin = 0;
		unsigned int blockLength = 0;

		for(int j = 0; j < 4; j++) {
			index *= 256;
			index += (unsigned char)m_receivedDataBuffer[i++];
		}
		for(int j = 0; j < 4; j++) {
			begin *= 256;
			begin += (unsigned char)m_receivedDataBuffer[i++];
		}
		for(int j = 0; j < 4; j++) {
			blockLength *= 256;
			blockLength += (unsigned char)m_receivedDataBuffer[i++];
		}

		QList<Piece*>& pieces = m_torrent->pieces();

		// Check for invalid ranges
		if(index >= (unsigned)pieces.size() || blockLength > MAX_MESSAGE_LENGTH) {
			qDebug() << "Invalid request (" << index << begin << blockLength << ")"
					 << "from" << addressPort();
			disconnect();
			*ok = false;
			return false;
		}

		Piece* piece = pieces[index];

		// Check for invalid begin + blockLength combination
		if(begin + blockLength > (unsigned)piece->size() || begin > (unsigned)piece->size()) {
			qDebug() << "Invalid request (" << index << begin << blockLength << ")"
					 << "from" << addressPort();
			disconnect();
			*ok = false;
			return false;
		}

		// Get the data
		QByteArray blockData;
		if(!piece->getBlockData(begin, blockLength, blockData)) {
			qDebug() << "Failed to get block (" << index << begin << blockLength << ")"
					 << "for" << addressPort();
			disconnect();
			*ok = false;
			return false;
		}

		sendPiece(index, begin, blockData);

		break;
	}
	case TorrentMessage::Piece:
	{
		int index = 0;
		int begin = 0;
		int blockLength = length - 9;
		for(int j = 0; j < 4; j++) {
			index *= 256;
			index += (unsigned char)m_receivedDataBuffer[i++];
		}
		for(int j = 0; j < 4; j++) {
			begin *= 256;
			begin += (unsigned char)m_receivedDataBuffer[i++];
		}
		Block* block = nullptr;
		int blockIndex = 0;
		for(auto b : m_blocksQueue) {
			if(b->piece()->pieceNumber() == index
					&& b->begin() == begin
					&& b->size() == blockLength) {
				block = b;
				break;
			}
			blockIndex++;
		}

		// If we weren't waiting for this block, check if it exists
		if(block == nullptr) {
			QList<Piece*> pieces = m_torrent->pieces();
			if(index >= 0 && index < pieces.size()) {
				block = pieces[index]->getBlock(begin, blockLength);
			}
		}

		if(block == nullptr) {
			qDebug() << "Error: received unrequested block from peer" << addressPort()
					 << ". Block(" << index << begin << blockLength << ")";
		} else {
			m_timedOut = false;
			const char* blockData = m_receivedDataBuffer.data() + i;
			if(!block->downloaded()) {
				block->setData(this, blockData);
				m_blocksQueue.removeAt(blockIndex);
			}
			if(m_blocksQueue.isEmpty()) {
				m_replyTimeoutTimer.stop();
			} else {
				m_replyTimeoutTimer.start();
			}
		}
		break;
	}
	case TorrentMessage::Cancel:
	{
		// TODO
		break;
	}
	case TorrentMessage::Port:
	{
		// TODO
		break;
	}
	default:
		qDebug() << "Error: Received unknown message with id =" << messageId
				 << " and length =" << length << "from" << addressPort();
		*ok = false;
		return false;
	}
	m_receivedDataBuffer.remove(0, 4 + length);
	return true;
}

void Peer::connectAll() {
	// Connection events
	connect(m_socket, SIGNAL(connected()), this, SLOT(connected()));
	connect(m_socket, SIGNAL(readyRead()), this, SLOT(readyRead()));
	connect(m_socket, SIGNAL(disconnected()), this, SLOT(finished()));
	connect(m_socket, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(error(QAbstractSocket::SocketError)));

	// Timeout callbacks
	connect(&m_replyTimeoutTimer, SIGNAL(timeout()), this, SLOT(replyTimeout()));
	connect(&m_handshakeTimeoutTimer, SIGNAL(timeout()), this, SLOT(handshakeTimeout()));
	connect(&m_reconnectTimer, SIGNAL(timeout()), this, SLOT(reconnect()));

	// Timeout intervals
	m_replyTimeoutTimer.setInterval(REPLY_TIMEOUT_MSEC);
	m_handshakeTimeoutTimer.setInterval(HANDSHAKE_TIMEOUT_MSEC);
	m_reconnectTimer.setInterval(RECONNECT_INTERVAL_MSEC);
}

void Peer::initBitfield() {
	int bitfieldSize = m_torrent->torrentInfo()->bitfieldSize();
	m_bitfield = new bool[bitfieldSize*8];
	for(int i = 0; i < bitfieldSize*8; i++) {
		m_bitfield[i] = false;
	}
}

void Peer::initClient(QTorrent *qTorrent) {
	m_qTorrent = qTorrent;
	m_torrent = nullptr;
	m_address = QByteArray(m_socket->peerAddress().toString().toUtf8());
	m_port = m_socket->peerPort();
	m_piecesDownloaded = 0;
	m_bitfield = nullptr;
	m_status = Handshaking;
}

void Peer::initServer(Torrent *torrent, const QByteArray &address, int port) {
	m_qTorrent = torrent->qTorrent();
	m_torrent = torrent;
	m_address = address;
	m_port = port;
	m_piecesDownloaded = 0;
	initBitfield();
	m_status = Created;
}

/* Slots */

void Peer::connected() {
	m_status = Handshaking;
	m_amChoking = true;
	m_amInterested = false;
	m_peerChoking = true;
	m_peerInterested = false;
	m_blocksQueue.clear();
	m_receivedDataBuffer.clear();
	m_timedOut = false;
	qDebug() << "Connected to" << addressPort();

	sendHandshake();
	m_handshakeTimeoutTimer.start();
}

void Peer::readyRead() {
	m_receivedDataBuffer.push_back(m_socket->readAll());

	switch(m_status) {
	case Handshaking:
		bool ok;
		if(readHandshakeReply(&ok)) {
			if(m_peerType == Client) {
				// Initialize peer's bitfield array.
				// Must be done after receiving handshake
				initBitfield();

				// Send our handshake message
				sendHandshake();

				// Add this peer to the torrent object
				m_torrent->addPeer(this);
			}
		} else {
			if(!ok) {
				fatalError();
			}
			break;
		}
		m_handshakeTimeoutTimer.stop();
		qDebug() << "Handshaking completed with peer" << addressPort();
		m_status = ConnectionEstablished;
		sendBitfield();
		// Fall down
	case ConnectionEstablished:
	{
		// Read messages
		int messagesReceived = 0;
		while(readPeerMessage(&ok)) {
			messagesReceived++;
		}

		// Check if any errors occured
		if(!ok) {
			fatalError();
			break;
		}

		if(messagesReceived) {
			sendMessages();
		}
		break;
	}
	default:
		m_receivedDataBuffer.clear();
		break;
	}
}

void Peer::finished() {
	m_handshakeTimeoutTimer.stop();
	m_replyTimeoutTimer.stop();
	for(auto block : m_blocksQueue) {
		// Check if other peers have requested this block
		bool othersHaveBlock = false;
		for(Peer* peer : m_torrent->peers()) {
			if(peer == this) {
				continue;
			}
			if(peer->blocksQueue().contains(block)) {
				othersHaveBlock = true;
				break;
			}
		}
		// Delete block if nobody has requested it
		if(!othersHaveBlock) {
			block->piece()->deleteBlock(block);
		}
	}
	if(m_status != Error) {
		m_status = Disconnected;
	}

	if(m_peerType == Server) {
		// If we both have the full torrent, dont reconnect later
		if(!downloaded() || !m_torrent->downloaded()) {
			m_reconnectTimer.start();
		}
	}
	m_blocksQueue.clear();
	qDebug() << "Connection to" << addressPort() << "closed" << m_socket->errorString();
}

void Peer::error(QAbstractSocket::SocketError socketError) {
	qDebug() << "Socket error" << addressPort() << ":" << m_socket->errorString() << "(" << socketError << ")";
	disconnect();
}

void Peer::replyTimeout() {
	qDebug() << "Peer" << addressPort() << "took too long to reply";
	m_timedOut = true;
	m_replyTimeoutTimer.stop();
}

void Peer::handshakeTimeout() {
	qDebug() << "Peer" << addressPort() << "took too long to handshake";
	m_handshakeTimeoutTimer.stop();
}

void Peer::reconnect() {
	qDebug() << "Reconnecting to" << addressPort();
	m_reconnectTimer.stop();
	startConnection();
}

/* Getter functions */

Torrent* Peer::torrent() {
	return m_torrent;
}

QByteArray& Peer::address() {
	return m_address;
}

int Peer::port() {
	return m_port;
}

int Peer::piecesDownloaded() {
	return m_piecesDownloaded;
}

bool* Peer::bitfield() {
	return m_bitfield;
}

QByteArray& Peer::protocol() {
	return m_protocol;
}

QByteArray& Peer::reserved() {
	return m_reserved;
}

QByteArray& Peer::infoHash() {
	return m_infoHash;
}

QByteArray& Peer::peerId() {
	return m_peerId;
}

Peer::Status Peer::status() {
	return m_status;
}

Peer::PeerType Peer::peerType() {
	return m_peerType;
}

bool Peer::amChoking() {
	return m_amChoking;
}

bool Peer::amInterested() {
	return m_amInterested;
}

bool Peer::peerChoking() {
	return m_peerChoking;
}

bool Peer::peerInterested() {
	return m_peerInterested;
}

QTcpSocket* Peer::socket() {
	return m_socket;
}

bool Peer::timedOut() {
	return m_timedOut;
}

QList<Block*>& Peer::blocksQueue() {
	return m_blocksQueue;
}

bool Peer::isPaused() const {
	return m_paused;
}

QString Peer::addressPort() {
	return QString(m_address) + ":" + QString::number(m_port);
}

bool Peer::downloaded() {
	return m_piecesDownloaded == m_torrent->torrentInfo()->numberOfPieces();
}

bool Peer::hasPiece(int index) {
	return m_bitfield[index];
}

bool Peer::isConnected() {
	return m_socket->state() == QAbstractSocket::ConnectedState;
}
