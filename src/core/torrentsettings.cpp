#include "torrentsettings.h"

TorrentSettings::TorrentSettings()
{
}


void TorrentSettings::setDownloadLocation(const QString& downloadLocation) {
	m_downloadLocation = downloadLocation;
}


const QString& TorrentSettings::downloadLocation() const {
	return m_downloadLocation;
}
