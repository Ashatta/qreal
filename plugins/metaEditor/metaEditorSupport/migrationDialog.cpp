#include "migrationDialog.h"
#include "ui_migrationDialog.h"

using namespace metaEditor;

MigrationDialog::MigrationDialog(const QString &fromName, qrRepo::RepoApi *fromRepo
		, const QString &toName, qrRepo::RepoApi *toRepo, QWidget *parent)
	: QDialog(parent)
	, ui(new Ui::MigrationDialog)
{
	ui->setupUi(this);

	mFromEditor = new MigrationEditor(fromName, fromRepo, ui->fromWidget);
	mToEditor = new MigrationEditor(toName, toRepo, ui->toWidget);
}

MigrationDialog::~MigrationDialog()
{
	delete ui;
}
