#pragma once

#include "qrutils/utilsDeclSpec.h"
#include "qrkernel/ids.h"

namespace qrRepo {
	class LogicalRepoApi;
}

namespace qReal {
namespace migration {
class LogEntry;

class QRUTILS_EXPORT Logger
{
public:
	Logger(qrRepo::LogicalRepoApi *repo);
	~Logger();

	void addDiagram(qReal::Id const &diagram);
	void addLogEntry(qReal::Id const &diagram, qReal::migration::LogEntry * const entry);
	void deleteLogEntry(qReal::Id const &diagram);
	QMap<int, QString> versionNames() const;
	int version() const;

	void createNewVersion(QString const &versionName);

	void clear();
	void reset(QHash<qReal::Id, QList<qReal::migration::LogEntry *> > const &log);
	QHash<qReal::Id, QList<qReal::migration::LogEntry *> > log() const;
	QHash<qReal::Id, QList<qReal::migration::LogEntry *> > logBetween(int startVersion, int endVersion) const;

	void rollBackTo(int version);

private:
	bool needNewVersion() const;
	void removeLastVersions(qReal::Id const &diagram);

	QHash<qReal::Id, QList<qReal::migration::LogEntry *> > mLog;
	QMap<int, QString> mVersionNames;
	int mModelVersion;

	qrRepo::LogicalRepoApi *mRepo; // Doesn't take ownership
};

}
}
