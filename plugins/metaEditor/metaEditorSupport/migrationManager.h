#pragma once

#include <QtWidgets/QDialog>
#include <QtWidgets/QListWidgetItem>

#include <qrrepo/repoControlInterface.h>
#include <qrrepo/logicalRepoApi.h>
#include <qrrepo/repoApi.h>

namespace Ui {
class MigrationManager;
}

namespace metaEditor {

class MigrationManager : public QDialog
{
	Q_OBJECT

public:
	explicit MigrationManager(qrRepo::RepoControlInterface &repoControlApi
			, qrRepo::LogicalRepoApi &logicalRepoApi, QWidget *parent = 0);
	~MigrationManager();

private slots:
	void chooseVersions();
	void createMigration(QString name, const QString &fromName, int from, const QString &toName, int to);
	void editMigration();
	void setMigration(const qReal::migration::Migration &migration, int idx);

	void deleteMigrations();

	void up();
	void down();

	void save();

	void selectionChanged();

private:
	qrRepo::RepoApi *migrationLanguageForVersion(int version);

	bool check(int direction) const;
	void move(int direction);
	void deleteVersions();

	static QListWidgetItem *versionItem(const QString &label, int version);
	static QListWidgetItem *migrationItem(const qReal::migration::Migration &migration);

	Ui::MigrationManager *ui;

	qrRepo::RepoControlInterface &mRepoControlApi;
	qrRepo::LogicalRepoApi &mLogicalRepoApi;

	bool mUnsaved;
};

}
