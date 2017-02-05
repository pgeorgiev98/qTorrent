#include "qtorrent.h"
#include <QApplication>
#include <QDebug>

int main(int argc, char *argv[]) {
	QApplication app(argc, argv);
	app.setApplicationName("qTorrent");

	QTorrent qTorrent;

	if(!qTorrent.startServer()) {
		qDebug() << "Failed to start server";
		return 1;
	}

	qTorrent.resumeTorrents();

	qTorrent.showMainWindow();
	app.exec();
	qTorrent.shutDown();
	return 0;
}
