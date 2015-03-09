#pragma once

#include <QWidget>

#include <qrrepo/repoApi.h>
#include <qrgui/editor/editorView.h>
#include <qrgui/models/models.h>
#include <qrgui/models/propertyEditorModel.h>
#include <qrgui/editor/sceneCustomizer.h>
#include <qrgui/plugins/pluginManager/interpreterEditorManager.h>
#include <qrgui/controller/controller.h>

namespace Ui {
class MigrationEditor;
}

namespace metaEditor {

class MigrationEditor : public QWidget
{
	Q_OBJECT

public:
	explicit MigrationEditor(const QString &languageName, qrRepo::RepoApi *repo, QWidget *parent = 0);
	~MigrationEditor();

	QByteArray serializedData();

private slots:
	void sceneSelectionChanged();

private:
	Ui::MigrationEditor *ui;

	QString mLanguageName;
	qReal::EditorView *mEditorView; // Takes ownership
	qrRepo::RepoApi *mRepo; // Takes ownership

	InterpreterEditorManager mInterpreter;
	models::Models mModels;
	Controller mController;
	PropertyEditorModel mPropertyModel;
	SceneCustomizer mSceneCustomizer;
};

}
