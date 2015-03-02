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
	explicit MigrationDialog(const QString &fromName, qrRepo::RepoApi *fromRepo
			, const QString &toName, qrRepo::RepoApi *toRepo, QWidget *parent = 0);
	~MigrationDialog();

private:
	Ui::MigrationDialog *ui;

	MigrationEditor *mFromEditor;
	MigrationEditor *mToEditor;
};

}
