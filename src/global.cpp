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

#include "global.h"

QString formatSize(qint64 size) {
	int i;
	qint64 nw = size, mul = 1;
	for(i = 0; nw >= 1024 && i < 7; i++, mul *= 1024, nw /= 1024);
	double nbytes = (double)size/mul;


	QString str = QString::number(nbytes, 'f', 2);
	str += ' ';

	switch(i) {
	case 0: return str + "B";
	case 1: return str + "KiB";
	case 2: return str + "MiB";
	case 3: return str + "GiB";
	case 4: return str + "TiB";
	case 5: return str + "PiB";
	case 6: return str + "EiB";
	default: return str + "ZiB";
	}
}

QByteArray percentEncode(const QByteArray &data) {
	QByteArray encoded;
	for(char b : data) {
		encoded += '%';
		encoded += QByteArray::number(b, 16).right(2).rightJustified(2, '0');
	}
	return encoded;
}
