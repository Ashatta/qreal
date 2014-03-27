#pragma once

#include "qrkernel/ids.h"
#include "qrgui/pluginManager/editorManagerInterface.h"
#include "qrgui/models/models.h"

namespace qReal {
namespace migration {

class DifferenceModel;

class Migrator
{
public:
	Migrator(qReal::EditorManagerInterface const &editorManager);
	~Migrator();

	void migrate(qReal::models::Models *model, QStringList const &metamodels);

private:
	void clear();

	void initMetamodelsRepos(QStringList const &metamodels);
	void ensureLoadWithOldMetamodels();
	QHash<qReal::Id, QList<qReal::migration::LogEntry *> > logBetweenVersions() const;
	void initDifferenceModels();

	qReal::EditorManagerInterface const &mEditorManager;
	qReal::models::Models *mModel; // Doesn't take ownership

	QMap<QString, qrRepo::RepoApi *> mOldMetamodels;
	QMap<QString, qrRepo::RepoApi *> mNewMetamodels;

	QList<DifferenceModel *> mDifferenceModels;

	QMap<QString, QHash<qReal::Id, QList<LogEntry *> > > mLogs;
};

}
}
