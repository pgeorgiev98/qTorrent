/* qTorrent - An open-source, cross-platform BitTorrent client
 * Copyright (C) 2017 Petko Georgiev
 *
 * ratecontroller.h
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

#ifndef RATECONTROLLER_H
#define RATECONTROLLER_H

#include <QObject>
#include <QTime>
#include <QSet>

class RcTcpSocket;

class RateController : public QObject
{
	Q_OBJECT

public:
	RateController(QObject *parent = nullptr);

	static RateController* instance();

	void addSocket(RcTcpSocket *socket);
	void removeSocket(RcTcpSocket *socket);

	inline int uploadLimit() const { return m_upLimit; }
	inline int downloadLimit() const { return m_downLimit; }
	inline void setUploadLimit(int bytesPerSecond) { m_upLimit = bytesPerSecond; }
	void setDownloadLimit(int bytesPerSecond);

public slots:
	void transfer();
	void scheduleTransfer();

private:
	QTime m_stopWatch;
	QSet<RcTcpSocket *> m_sockets;
	int m_upLimit;
	int m_downLimit;
	bool m_transferScheduled;
};


#endif // RATECONTROLLER_H
