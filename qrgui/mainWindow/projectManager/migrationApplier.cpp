#include "migrationApplier.h"

#include <QtWidgets/QMessageBox>

#include <qrgui/models/logicalModelAssistApi.h>
#include <qrgui/models/graphicalModelAssistApi.h>
#include <qrutils/migration/graphTransformation.h>
#include <qrrepo/repoApi.h>

using namespace qReal::migration;

bool MigrationApplier::runUserMigrations(const EditorManagerInterface &editorManager
		, models::ModelsInterface *model, IdList &createdElements, QWidget *window)
{
	QSet<QString> editorsToCheck;
	const IdList allElements = model->logicalModelAssistApi().children(Id::rootId());
	for (const Id &element : allElements) {
		editorsToCheck << element.editor();
	}

	for (const QString &editor : editorsToCheck) {
		int oldVersion = model->logicalRepoApi().metamodelVersion(editor);
		int currentVersion = editorManager.editorVersion(Id(editor));

		if (oldVersion == currentVersion) {
			continue;
		}

		if (currentVersion < oldVersion) {
			QMessageBox::information(window, QObject::tr("Can`t open project file")
					, QObject::tr("Plugin for %1 needed for opening the save file is outdated. "
							"Please update your environment.").arg(editor));
			return false;
		}

		qrRepo::RepoApi *metamodelRepo = editorManager.metamodel(editor);
		QList<Migration> migrations = metamodelRepo->migrations();
		for (const Migration &migration : migrations) {
			if (migration.mFromVersion < oldVersion || migration.mToVersion > currentVersion) {
				continue;
			}

			initTemporaryMigrationFiles(migration);

			QScopedPointer<qrRepo::RepoApi> fromTemplate(new qrRepo::RepoApi("temp1", false, false));
			QScopedPointer<qrRepo::RepoApi> toTemplate(new qrRepo::RepoApi("temp2", false, false));

			GraphTransformation transformation(model->logicalModelAssistApi(), model->graphicalModelAssistApi()
					, *fromTemplate.data(), *toTemplate.data());
			transformation.apply();
			createdElements << transformation.createdElements();

			QFile::remove("temp1");
			QFile::remove("temp2");
		}

		delete metamodelRepo;
	}

	return true;
}

void MigrationApplier::initTemporaryMigrationFiles(const Migration &migration)
{
	QFile file1("temp1");
	file1.open(QIODevice::WriteOnly);
	file1.write(migration.mFromData);
	QFile file2("temp2");
	file2.open(QIODevice::WriteOnly);
	file2.write(migration.mToData);
}
