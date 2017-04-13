/* qTorrent - An open-source, cross-platform BitTorrent client
 * Copyright (C) 2017 Petko Georgiev
 *
 * trackerclient.h
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

#ifndef TRACKERCLIENT_H
#define TRACKERCLIENT_H

#include "torrentinfo.h"
#include <QNetworkAccessManager>
#include <QTimer>

class BencodeParser;
class Torrent;

/*
 * This class is used to communicate with the tracker
 * Can announce and automatically reannounce to the tracker
 * and fetch a list of peers
 */
class TrackerClient : public QObject
{
public:
	/* The 'announce' event types */
	enum Event {
		Started,   /* First request to the tracker */
		Stopped,   /* When shutting down */
		Completed, /* When download completes */
		None       /* Just a normal announce */
	};

	/* Constructor and destructor */
	TrackerClient(Torrent *torrent);
	~TrackerClient();

	/* Used to send 'announce' to the tracker */
	void announce(Event event);

	/* Returns the number of successfull announces */
	int numberOfAnnounces() const;

	bool hasAnnouncedStarted() const;

public slots:
	/* For QNetworkAccessManager */
	void httpFinished();

	/* Called by a timer every m_interval seconds to reannounce */
	void reannounce();

private:
	Q_OBJECT

	Torrent *m_torrent;
	QNetworkAccessManager m_accessManager;
	QNetworkReply *m_reply;

	// For reannouncing
	int m_reannounceInterval;
	QTimer m_reannounceTimer;

	// The current index in the TorrentInfo::announceUrlsList()
	// This is incremented if connecting to the tracker failed
	int m_currentAnnounceListIndex;

	// Set the current announce url to the next one
	// Returns false when it reaches the end
	bool nextAnnounceUrl();

	// Have we sent a 'Started' announce to the tracker
	bool m_hasAnnouncedStarted;

	// Returns the current announce url
	const QByteArray &currentAnnounceUrl() const;

	// Resets the current announce url
	void resetCurrentAnnounceUrl();

	// This will be called when announce failed
	void announceFailed();

	// This will be called when announce succeeded
	void announceSucceeded();

	// Holds the number of successfull announces
	int m_numberOfAnnounces;

	// Last event with which was announce() called
	Event m_lastEvent;
};

#endif // TRACKERCLIENT_H
