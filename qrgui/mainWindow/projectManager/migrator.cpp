#include "mainWindow/projectManager/migrator.h"

#include "qrgui/plugins/pluginManager/interpreterEditorManager.h"
#include "qrutils/migration/analyzer.h"

#include <qrgui/models/logicalModelAssistApi.h>
#include <qrgui/models/graphicalModelAssistApi.h>

#include <QtWidgets/QMessageBox>

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

bool Migrator::migrate(models::ModelsInterface *model, const IdList &ignoredElements, QWidget *window)
{
	clear();

	mModel = model;

	QSet<QString> editorsToCheck;
	const IdList allElements = model->logicalModelAssistApi().children(Id::rootId());
	for (const Id &element : allElements) {
		if (mEditorManager.editorVersion(element) < mModel->logicalRepoApi().metamodelVersion(element.editor())) {
			QMessageBox::information(window, QObject::tr("Can`t open project file")
					, QObject::tr("Plugin for %1 needed for opening the save file is outdated. "
							"Please update your environment.").arg(element.editor()));
			return false;
		}

		if (mEditorManager.editorVersion(element) > mModel->logicalRepoApi().metamodelVersion(element.editor())) {
			editorsToCheck << element.editor();
		}
	}

	if (editorsToCheck.isEmpty()) {
		return true;
	}

	initMetamodelsRepos(editorsToCheck);
	initDifferenceModels();
	ensureLoadWithOldMetamodels(ignoredElements);

	Analyzer analyzer(logBetweenVersions(), mDifferenceModels);
	analyzer.analyze();

	foreach (Transformation *transform, analyzer.transformations()) {
		transform->apply(mModel, ignoredElements);
	}

	foreach (QString const &metamodel, editorsToCheck) {
		mModel->mutableLogicalRepoApi().addUsedMetamodel(metamodel, mNewMetamodels[metamodel]->version() - 1);
	}

	return true;
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

void Migrator::ensureLoadWithOldMetamodels(const IdList &ignoredElements)
{
	InterpreterEditorManager interpreter("");
	foreach (QString const &editor, mOldMetamodels.keys()) {
		interpreter.addPlugin(editor, mOldMetamodels[editor]);
	}

	interpreter.ensureModelCorrectness(mModel->mutableLogicalRepoApi(), ignoredElements);

	// find another way to keep old metamodels alive after interpreter destruction
	foreach (QString const &editor, mOldMetamodels.keys()) {
		mOldMetamodels[editor] = mEditorManager.metamodel(editor);
		mOldMetamodels[editor]->rollBackTo(mModel->logicalRepoApi().metamodelVersion(editor));
	}
}

void Migrator::initMetamodelsRepos(QSet<QString> const &metamodels)
{
	foreach (QString const &editor, metamodels) {
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
