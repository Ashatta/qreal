#pragma once

#include <QDialog>

#include <qrrepo/repoApi.h>
#include "migrationEditor.h"
#include <qrutils/migration/migration.h>

namespace Ui {
class MigrationDialog;
}

namespace metaEditor {

class MigrationDialog : public QDialog
{
	Q_OBJECT

public:
	explicit MigrationDialog(const QString &name, int fromVersion, const QString &fromName, qrRepo::RepoApi *fromRepo
			, int toVersion, const QString &toName, qrRepo::RepoApi *toRepo
			, const QString &languageName, QWidget *parent = 0);
	~MigrationDialog();

	void setMigration(const qReal::migration::Migration &migration, int idx);

signals:
	void migrationCreated(const qReal::migration::Migration &migration, int idx);

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

	QString mMigrationName;
	int mMigrationIndex;
};

}
