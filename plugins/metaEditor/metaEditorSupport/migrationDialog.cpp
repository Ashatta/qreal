#include "migrationDialog.h"
#include "ui_migrationDialog.h"

using namespace metaEditor;

MigrationDialog::MigrationDialog(const QString &name, int fromVersion, const QString &fromName, qrRepo::RepoApi *fromRepo
		, int toVersion, const QString &toName, qrRepo::RepoApi *toRepo, const QString &languageName
		, QWidget *parent)
	: QDialog(parent)
	, ui(new Ui::MigrationDialog)
	, mFromVersion(fromVersion)
	, mToVersion(toVersion)
	, mFromVersionName(fromName)
	, mToVersionName(toName)
	, mMigrationName(name)
	, mMigrationIndex(-1)
{
	ui->setupUi(this);
	setWindowTitle(mMigrationName);

	mFromEditor = new MigrationEditor(languageName, fromRepo, ui->fromWidget);
	mToEditor = new MigrationEditor(languageName, toRepo, ui->toWidget);

	connect(this, &MigrationDialog::accepted, this, &MigrationDialog::onAccept);
}

MigrationDialog::~MigrationDialog()
{
	delete ui;
}

void MigrationDialog::onAccept()
{
	qReal::migration::Migration result(mMigrationName, mFromVersion, mToVersion
			, mFromVersionName, mToVersionName, mFromEditor->serializedData()
			, mToEditor->serializedData()
			, ui->policyComboBox->currentIndex() == 0
					? qReal::migration::Migration::applyOnce
					: qReal::migration::Migration::fixedPoint);
	emit migrationCreated(result, mMigrationIndex);
}

void MigrationDialog::setMigration(const migration::Migration &migration, int idx)
{
	mFromEditor->load(migration.mFromData);
	mToEditor->load(migration.mToData);
	mMigrationIndex = idx;
	mMigrationName = migration.mName;
	setWindowTitle(mMigrationName);
	ui->policyComboBox->setCurrentIndex(migration.mPolicy == qReal::migration::Migration::applyOnce ? 0 : 1);
}
