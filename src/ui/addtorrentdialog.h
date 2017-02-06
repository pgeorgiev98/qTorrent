#ifndef ADDTORRENTDIALOG_H
#define ADDTORRENTDIALOG_H

#include <QDialog>
#include <QUrl>

class QTorrent;
class QLineEdit;
class QPushButton;

class AddTorrentDialog : public QDialog
{
	Q_OBJECT

public:
	AddTorrentDialog(QWidget *parent, QTorrent* qTorrent);

	void setTorrentUrl(QUrl url);

public slots:
	void browseFilePath();
	void browseDownloadLocation();
	void ok();
	void cancel();

private:
	QTorrent* m_qTorrent;
	QLineEdit* m_filePath;
	QPushButton* m_browseFilePath;
	QLineEdit* m_downloadLocation;
	QPushButton* m_browseDownloadLocation;
	QPushButton* m_ok;
	QPushButton* m_cancel;

	void connectAll();

};

#endif // ADDTORRENTDIALOG_H
