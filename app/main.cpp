/* qTorrent - An open-source, cross-platform BitTorrent client
 * Copyright (C) 2017 Petko Georgiev
 *
 * main.cpp
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

#include "qtorrent.h"
#include "core/remote.h"
#include <QApplication>
#include <QDebug>

int main(int argc, char *argv[])
{
	QApplication app(argc, argv);
	app.setApplicationName("qTorrent");

	Remote remote;
	if (!remote.start()) {
		qDebug() << "Already running";
		return 0;
	}

	QTorrent qTorrent;

	if (!qTorrent.startServer()) {
		qDebug() << "Failed to start server";
		return 1;
	}

	qTorrent.resumeTorrents();

	qTorrent.startLSDClient();

	qTorrent.showMainWindow();
	app.exec();
	qTorrent.shutDown();
	return 0;
}
