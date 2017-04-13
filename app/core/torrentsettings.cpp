/* qTorrent - An open-source, cross-platform BitTorrent client
 * Copyright (C) 2017 Petko Georgiev
 *
 * torrentsettings.cpp
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

#include "torrentsettings.h"

TorrentSettings::TorrentSettings()
{
}


void TorrentSettings::setDownloadLocation(const QString &downloadLocation)
{
	m_downloadLocation = downloadLocation;
}

void TorrentSettings::setStartImmediately(bool startImmediately)
{
	m_startImmediately = startImmediately;
}

void TorrentSettings::setSkipHashCheck(bool skipHashCheck)
{
	m_skipHashCheck = skipHashCheck;
}


const QString &TorrentSettings::downloadLocation() const
{
	return m_downloadLocation;
}

bool TorrentSettings::startImmediately() const
{
	return m_startImmediately;
}

bool TorrentSettings::skipHashCheck() const
{
	return m_skipHashCheck;
}
