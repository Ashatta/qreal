#pragma once

#include <QDialog>

#include <qrrepo/repoApi.h>
#include "migrationEditor.h"

namespace Ui {
class MigrationDialog;
}

namespace metaEditor {

class MigrationDialog : public QDialog
{
	Q_OBJECT

public:
	explicit MigrationDialog(int fromVersion, const QString &fromName, qrRepo::RepoApi *fromRepo
			, int toVersion, const QString &toName, qrRepo::RepoApi *toRepo, QWidget *parent = 0);
	~MigrationDialog();

signals:
	void migrationCreated(int fromVersion, const QString &fromName, const QByteArray &fromData
			, int toVersion, const QString &toName, const QByteArray &toData);

private slots:
	void onAccept();

private:
	Ui::MigrationDialog *ui;

	MigrationEditor *mFromEditor;
	MigrationEditor *mToEditor;

	int mFromVersion;
	int mToVersion;
	QString mFromVersionName;
	QString mToVersionName;
};

}
