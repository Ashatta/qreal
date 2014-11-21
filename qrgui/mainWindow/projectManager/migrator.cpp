#include "mainWindow/projectManager/migrator.h"

#include "qrgui/plugins/pluginManager/interpreterEditorManager.h"
#include "qrutils/migration/analyzer.h"

using namespace qReal::migration;

Migrator::Migrator(EditorManagerInterface const &editorManager)
	: mEditorManager(editorManager)
{
}

Migrator::~Migrator()
{
	clear();
}

void Migrator::migrate(models::ModelsInterface *model, QStringList const &metamodels)
{
	clear();

	mModel = model;
	initMetamodelsRepos(metamodels);
	initDifferenceModels();

	ensureLoadWithOldMetamodels();

	Analyzer analyzer(logBetweenVersions(), mDifferenceModels);

	analyzer.analyze();

	foreach (Transformation *transform, analyzer.transformations()) {
		transform->apply(mModel);
	}

	foreach (QString const &metamodel, metamodels) {
		mModel->mutableLogicalRepoApi().addUsedMetamodel(metamodel, mNewMetamodels[metamodel]->version() - 1);
	}
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
