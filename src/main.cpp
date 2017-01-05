#include "qtorrent.h"
#include <QTextStream>
#include <QApplication>
#include <QDebug>

int main(int argc, char *argv[]) {
	QApplication app(argc, argv);
	QTorrent qTorrent;
	if(argc <= 1) {
		qDebug() << "Please enter a filename as a command line argument";
		return 0;
	}

	if(!qTorrent.startServer()) {
		qDebug() << "Failed to start server";
	}

	if(!qTorrent.addTorrent(argv[1])) {
		qDebug() << "Failed to add torrent";
	} else {
		qDebug() << "Torrent added";
	}
	return app.exec();
}
