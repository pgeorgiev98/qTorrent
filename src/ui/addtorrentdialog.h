#ifndef ADDTORRENTDIALOG_H
#define ADDTORRENTDIALOG_H

#include <QDialog>
#include <QUrl>

class QLineEdit;
class QPushButton;
class QCheckBox;

class AddTorrentDialog : public QDialog
{
	Q_OBJECT

public:
	AddTorrentDialog(QWidget *parent);

	void setTorrentUrl(QUrl url);

public slots:
	void browseFilePath();
	void browseDownloadLocation();
	void ok();
	void cancel();

private:
	QLineEdit* m_filePath;
	QPushButton* m_browseFilePath;
	QLineEdit* m_downloadLocation;
	QPushButton* m_browseDownloadLocation;
	QCheckBox* m_startImmediately;
	QPushButton* m_ok;
	QPushButton* m_cancel;

	void connectAll();

};

#endif // ADDTORRENTDIALOG_H
