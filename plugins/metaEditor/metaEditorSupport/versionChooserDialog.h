#pragma once

#include <QDialog>
#include <QMap>

namespace Ui {
class VersionChooserDialog;
}

class VersionChooserDialog : public QDialog
{
	Q_OBJECT

public:
	explicit VersionChooserDialog(QMap<int, QString> const &versions, QWidget *parent = 0);
	~VersionChooserDialog();

signals:
	void versionChosen(int from, int to);

private slots:
	void onAccept();

private:
	QMap<int, QString> mVersions;

	Ui::VersionChooserDialog *ui;
};
