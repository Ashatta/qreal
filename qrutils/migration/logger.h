#pragma once

#include "qrutils/migration/loggerInterface.h"

namespace qrRepo {
	class LogicalRepoApi;
}

namespace qReal {
namespace migration {

class Logger : public LoggerInterface
{
public:
	Logger(qrRepo::LogicalRepoApi *repo);
	~Logger();

	void addLogEntry(qReal::Id const &diagram, qReal::migration::LogEntry * const entry);
	void deleteLogEntry(qReal::Id const &diagram);
	int version() const;

	void createNewVersion();

	void clear();
	void reset(QHash<qReal::Id, QList<qReal::migration::LogEntry *> > const &log);
	QHash<qReal::Id, QList<qReal::migration::LogEntry *> > log() const;
	QHash<qReal::Id, QList<qReal::migration::LogEntry *> > logBetween(int startVersion, int endVersion) const;

	void rollBackTo(int version);

private:
	bool needNewVersion() const;
	void removeLastVersions(qReal::Id const &diagram);

	QHash<qReal::Id, QList<qReal::migration::LogEntry *> > mLog;
	int mModelVersion;

	qrRepo::LogicalRepoApi *mRepo; // Doesn't take ownership
};

}
}
