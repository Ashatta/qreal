#include "migrationDialog.h"
#include "ui_migrationDialog.h"

using namespace metaEditor;

MigrationDialog::MigrationDialog(int fromVersion, const QString &fromName, qrRepo::RepoApi *fromRepo
		, int toVersion, const QString &toName, qrRepo::RepoApi *toRepo, QWidget *parent)
	: QDialog(parent)
	, ui(new Ui::MigrationDialog)
	, mFromVersion(fromVersion)
	, mToVersion(toVersion)
	, mFromVersionName(fromName)
	, mToVersionName(toName)
{
	ui->setupUi(this);

	mFromEditor = new MigrationEditor(fromName, fromRepo, ui->fromWidget);
	mToEditor = new MigrationEditor(toName, toRepo, ui->toWidget);

	connect(this, &MigrationDialog::accepted, this, &MigrationDialog::onAccept);
}

MigrationDialog::~MigrationDialog()
{
	delete ui;
}

void MigrationDialog::onAccept()
{
	emit migrationCreated(mFromVersion, mFromVersionName, mFromEditor->serializedData()
			, mToVersion, mToVersionName, mToEditor->serializedData());
}
