#include "migration/migrator.h"

#include "qrgui/pluginManager/interpreterEditorManager.h"

using namespace qReal::migration;

Migrator::Migrator(EditorManagerInterface const &editorManager)
	: mEditorManager(editorManager)
{
}

Migrator::~Migrator()
{
	clear();
}

void Migrator::migrate(models::Models *model, QStringList const &metamodels)
{
	clear();
	mModel = model;

	foreach (QString const &editor, metamodels.toSet()) {
		mNewMetamodels[editor] = mEditorManager.metamodel(editor);

		mOldMetamodels[editor] = mEditorManager.metamodel(editor);
		mOldMetamodels[editor]->rollBackTo(model->logicalRepoApi().metamodelVersion(editor));
	}

	ensureLoadWithOldMetamodels();
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
}

void Migrator::ensureLoadWithOldMetamodels()
{
	InterpreterEditorManager interpreter("");
	foreach (QString const &editor, mOldMetamodels.keys()) {
		interpreter.addPlugin(editor, mOldMetamodels[editor]);
	}

	interpreter.ensureModelCorrectness(mModel->mutableLogicalRepoApi());
}
