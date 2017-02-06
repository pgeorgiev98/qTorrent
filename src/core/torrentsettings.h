#ifndef TORRENTSETTINGS_H
#define TORRENTSETTINGS_H

#include <QString>

class TorrentSettings {
public:
	TorrentSettings();

	/* Setters */
	void setDownloadLocation(const QString& downloadLocation);

	/* Getters */
	const QString& downloadLocation() const;

private:
	QString m_downloadLocation;
};

#endif // TORRENTSETTINGS_H
