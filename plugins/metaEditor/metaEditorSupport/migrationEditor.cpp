#include "migrationEditor.h"
#include "ui_migrationEditor.h"

using namespace metaEditor;

MigrationEditor::MigrationEditor(const QString &languageName
		, qrRepo::RepoApi *repo
		, QWidget *parent)
	: QWidget(parent)
	, ui(new Ui::MigrationEditor)
	, mLanguageName(languageName)
	, mRepo(repo)
	, mInterpreter("")
	, mModels("", mInterpreter)
	, mController(mModels.repoControlApi())
{
	ui->setupUi(this);

	mInterpreter.addPlugin(mLanguageName, mRepo);
	mEditorView = new EditorView(mModels, mController, mSceneCustomizer, qReal::Id::rootId(), ui->editorWidget);
}

MigrationEditor::~MigrationEditor()
{
	delete ui;
}
