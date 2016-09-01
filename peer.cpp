#include "peer.h"

Peer::Peer(const QByteArray &address, int port) :
	m_address(address),
	m_port(port)
{
}

Peer::~Peer() {
}
