#ifndef TORRENTSETTINGS_H
#define TORRENTSETTINGS_H

#include <QString>

class TorrentSettings {
public:
	TorrentSettings();

	/* Setters */
	void setDownloadLocation(const QString& downloadLocation);
	void setStartImmediately(bool startImmediately);

	/* Getters */
	const QString& downloadLocation() const;
	bool startImmediately() const;

private:
	QString m_downloadLocation;
	bool m_startImmediately;
};

#endif // TORRENTSETTINGS_H
