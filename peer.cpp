#include "peer.h"

Peer::Peer(const QByteArray &address, int port) :
	m_address(address),
	m_port(port)
{
}

Peer::~Peer() {
}

QByteArray& Peer::address() {
	return m_address;
}

int Peer::port() {
	return m_port;
}
