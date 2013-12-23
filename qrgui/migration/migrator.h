#pragma once

#include "qrkernel/ids.h"
#include "qrgui/pluginManager/editorManagerInterface.h"
#include "qrgui/mainwindow/mainWindowInterpretersInterface.h"
#include "qrgui/models/models.h"

namespace qReal {
namespace migration {

class Migrator
{
public:
	Migrator(qReal::EditorManagerInterface const &editorManager
			, gui::MainWindowInterpretersInterface &interpretersInterface);
	~Migrator();

	void migrate(qReal::models::Models *model, QStringList const &metamodels);

private:
	void clear();

	void initMetamodelsRepos(QStringList const &metamodels);
	void ensureLoadWithOldMetamodels();
	QHash<qReal::Id, QList<qReal::migration::LogEntry *> > logBetweenVersions() const;

	qReal::EditorManagerInterface const &mEditorManager;
	qReal::gui::MainWindowInterpretersInterface &mInterpretersInterface;

	qReal::models::Models *mModel; // Doesn't take ownership

	QMap<QString, qrRepo::RepoApi *> mOldMetamodels;
	QMap<QString, qrRepo::RepoApi *> mNewMetamodels;

	QMap<QString, QHash<qReal::Id, QList<LogEntry *> > > mLogs;
};

}
}
