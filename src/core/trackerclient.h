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
class TrackerClient : public QObject {
public:
	/* The 'announce' event types */
	enum Event {
		Started,   /* First request to the tracker */
		Stopped,   /* When shutting down */
		Completed, /* When download completes */
		None       /* Just a normal announce */
	};

	/* Constructor and destructor */
	TrackerClient(Torrent* torrent, TorrentInfo* torrentInfo);
	~TrackerClient();

	/* Used to send 'announce' to the tracker */
	void announce(Event event);

	/* Returns the number of successfull announces */
	int numberOfAnnounces() const;

public slots:
	/* For QNetworkAccessManager */
	void httpFinished();
	void httpReadyRead();

	/* Called by a timer every m_interval seconds to reannounce */
	void reannounce();
private:
	Q_OBJECT

	Torrent* m_torrent;
	TorrentInfo* m_torrentInfo;
	QNetworkAccessManager m_accessManager;
	QNetworkReply *m_reply;
	QByteArray m_announceResponse;

	// For reannouncing
	int m_reannounceInterval;
	QTimer m_reannounceTimer;

	// The current index in the TorrentInfo::announceUrlsList()
	// This is incremented if connecting to the tracker failed
	int m_currentAnnounceListIndex;

	// Set the current announce url to the next one
	// Returns false when it reaches the end
	bool nextAnnounceUrl();

	// Returns the current announce url
	const QByteArray& currentAnnounceUrl() const;

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
