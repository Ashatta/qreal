#include "mainWindow/projectManager/migrator.h"

#include "qrgui/plugins/pluginManager/interpreterEditorManager.h"
#include "qrutils/migration/analyzer.h"
#include <qrutils/migration/graphTransformation.h>

#include <qrgui/models/logicalModelAssistApi.h>
#include <qrgui/models/graphicalModelAssistApi.h>

#include <qrutils/outFile.h>

using namespace qReal::migration;

Migrator::Migrator(EditorManagerInterface const &editorManager)
	: mEditorManager(editorManager)
{
}

Migrator::~Migrator()
{
	clear();
}

bool Migrator::migrate(models::ModelsInterface *model)
{
	clear();

	mModel = model;

	QStringList canMigrate;;
	mEditorManager.canMigrateMetamodels(canMigrate, mMigrationFailed
			, mModel->logicalRepoApi(), mModel->graphicalRepoApi());
	mMigrationFailed.removeDuplicates();

	if (!mMigrationFailed.empty()) {
		return false;
	}

	initMetamodelsRepos(canMigrate);
	runUserMigrations();

	if (canMigrate.isEmpty()) {
		return true;
	}

	initDifferenceModels();
	ensureLoadWithOldMetamodels();

	Analyzer analyzer(logBetweenVersions(), mDifferenceModels);
	analyzer.analyze();

	foreach (Transformation *transform, analyzer.transformations()) {
		transform->apply(mModel);
	}

	foreach (QString const &metamodel, canMigrate) {
		mModel->mutableLogicalRepoApi().addUsedMetamodel(metamodel, mNewMetamodels[metamodel]->version() - 1);
	}

	return true;
}

QStringList Migrator::migrationFailed() const
{
	return mMigrationFailed;
}

void Migrator::clear()
{
	foreach (qrRepo::RepoApi *metamodel, mNewMetamodels) {
		delete metamodel;
	}

	mNewMetamodels.clear();

	foreach (qrRepo::RepoApi *metamodel, mNewMetamodels) {
		delete metamodel;
	}

	mOldMetamodels.clear();

	foreach (DifferenceModel *diffModel, mDifferenceModels) {
		delete diffModel;
	}

	mDifferenceModels.clear();
	mMigrationFailed.clear();
}

void Migrator::ensureLoadWithOldMetamodels()
{
	InterpreterEditorManager interpreter("");
	foreach (QString const &editor, mOldMetamodels.keys()) {
		interpreter.addPlugin(editor, mOldMetamodels[editor]);
	}

	interpreter.ensureModelCorrectness(mModel->mutableLogicalRepoApi());

	// find another way to keep old metamodels alive after interpreter destruction
	foreach (QString const &editor, mOldMetamodels.keys()) {
		mOldMetamodels[editor] = mEditorManager.metamodel(editor);
		mOldMetamodels[editor]->rollBackTo(mModel->logicalRepoApi().metamodelVersion(editor));
	}
}

void Migrator::initMetamodelsRepos(QStringList const &metamodels)
{
	foreach (QString const &editor, metamodels.toSet()) {
		mNewMetamodels[editor] = mEditorManager.metamodel(editor);

		mOldMetamodels[editor] = mEditorManager.metamodel(editor);
		mOldMetamodels[editor]->rollBackTo(mModel->logicalRepoApi().metamodelVersion(editor));
	}
}

void Migrator::runUserMigrations()
{
	for (const Id &editor : mEditorManager.editors()) {
		qrRepo::RepoApi *metamodelRepo = mNewMetamodels.value(editor.editor()
				, mEditorManager.metamodel(editor.editor()));

		if (!metamodelRepo) {
			continue;
		}

		QList<QPair<QByteArray, QByteArray> > migrations = metamodelRepo->migrations();
		for (const QPair<QByteArray, QByteArray> &migration : migrations) {
			initTemporaryMigrationFiles(migration);

			QScopedPointer<qrRepo::RepoApi> fromTemplate(new qrRepo::RepoApi("temp1", false, false));
			QScopedPointer<qrRepo::RepoApi> toTemplate(new qrRepo::RepoApi("temp2", false, false));

			GraphTransformation transformation(mModel->logicalModelAssistApi(), mModel->graphicalModelAssistApi()
					, *fromTemplate.data(), *toTemplate.data());
			transformation.apply();

			QFile::remove("temp1");
			QFile::remove("temp2");
		}
	}
}

QHash<qReal::Id, QList<qReal::migration::LogEntry *> > Migrator::logBetweenVersions() const
{
	QHash<qReal::Id, QList<qReal::migration::LogEntry *> > result;

	foreach (QString const &editor, mNewMetamodels.keys()) {
		result.unite(mNewMetamodels[editor]->logBetween(mModel->logicalRepoApi().metamodelVersion(editor)
				, mNewMetamodels[editor]->version()));
	}

	return result;
}

void Migrator::initDifferenceModels()
{
	foreach (QString const &editor, mOldMetamodels.keys()) {
		mDifferenceModels.append(new DifferenceModel(mOldMetamodels[editor], mNewMetamodels[editor]));
	}
}

void Migrator::initTemporaryMigrationFiles(const QPair<QByteArray, QByteArray> &migration)
{
	QFile file1("temp1");
	file1.open(QIODevice::WriteOnly);
	file1.write(migration.first);
	QFile file2("temp2");
	file2.open(QIODevice::WriteOnly);
	file2.write(migration.second);
}
