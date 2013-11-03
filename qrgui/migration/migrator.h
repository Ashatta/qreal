#pragma once

#include "qrkernel/ids.h"
#include "qrgui/pluginManager/editorManagerInterface.h"
#include "qrgui/models/models.h"

namespace migration {

class Migrator
{
public:
	Migrator(qReal::EditorManagerInterface const &editorManager);

	void migrate(qReal::models::Models *model, QStringList const &metamodels);

private:
	void clear();

	qReal::EditorManagerInterface const &mEditorManager;
	QMap<QString, qrRepo::RepoApi *> mMetamodels;
	qReal::models::Models *mModel; // Doesn't take ownership
};

}
