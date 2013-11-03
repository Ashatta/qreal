#include "migration/migrator.h"

using namespace migration;
using namespace qReal;

Migrator::Migrator(EditorManagerInterface const &editorManager)
	: mEditorManager(editorManager)
{
}

void Migrator::migrate(models::Models *model, QStringList const &metamodels)
{
	clear();
	mModel = model;

	foreach (QString const &editor, metamodels.toSet()) {
		mMetamodels[editor] = mEditorManager.metamodel(editor);
	}
}

void Migrator::clear()
{
	foreach (qrRepo::RepoApi *metamodel, mMetamodels) {
		delete metamodel;
	}

	mMetamodels.clear();
}
