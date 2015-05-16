#pragma once

#include <QDialog>
#include <QMap>

namespace Ui {
class VersionChooserDialog;
}

namespace metaEditor {

class VersionChooserDialog : public QDialog
{
	Q_OBJECT

public:
	explicit VersionChooserDialog(QMap<int, QString> const &versions, QWidget *parent = 0);
	~VersionChooserDialog();

signals:
	void versionChosen(const QString &name, const QString &fromName, int from, const QString &toName, int to);

private slots:
	void onAccept();

private:
	QMap<int, QString> mVersions;

	Ui::VersionChooserDialog *ui;
};

}
