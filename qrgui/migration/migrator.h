#pragma once

#include "qrkernel/ids.h"
#include "qrgui/pluginManager/editorManagerInterface.h"
#include "qrgui/models/models.h"

namespace qReal {
namespace migration {

class Migrator
{
public:
	Migrator(qReal::EditorManagerInterface const &editorManager);

	void migrate(qReal::models::Models *model, QStringList const &metamodels);

private:
	void clear();

	qReal::EditorManagerInterface const &mEditorManager;
	qReal::models::Models *mModel; // Doesn't take ownership

	QMap<QString, qrRepo::RepoApi *> mOldMetamodels;
	QMap<QString, qrRepo::RepoApi *> mNewMetamodels;
};

}
}
