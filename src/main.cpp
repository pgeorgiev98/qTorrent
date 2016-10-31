#include "qtorrent.h"
#include <QTextStream>
#include <QApplication>

int main(int argc, char *argv[]) {
	QApplication app(argc, argv);
	QTextStream err(stderr);
	QTextStream out(stdout);
	if(argc <= 1) {
		err << "Please enter a filename as a command line argument" << endl;
		return 0;
	}
	QTorrent qTorrent;
	if(!qTorrent.addTorrent(argv[1])) {
		err << "Failed to add torrent" << endl;
	} else {
		err << "Torrent added" << endl;
	}
	return app.exec();
}
