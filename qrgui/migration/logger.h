#pragma once

#include "qrkernel/ids.h"

namespace qrRepo {
namespace details {
	class Repository;
}
}

namespace qReal {
namespace migration {

class LogEntry;

class Logger
{
public:
	Logger(qrRepo::details::Repository *repo);
	~Logger();

	void addLogEntry(qReal::Id const &diagram, qReal::migration::LogEntry * const entry);
	void deleteLogEntry(qReal::Id const &diagram);
	int version() const;

	void createNewVersion();

	void clear();
	void reset(QHash<qReal::Id, QList<qReal::migration::LogEntry *> > const &log);
	QHash<qReal::Id, QList<qReal::migration::LogEntry *> > log() const;

	void rollBackTo(int version);

private:
	bool needNewVersion() const;
	void removeLastVersions(qReal::Id const &diagram);

	QHash<qReal::Id, QList<qReal::migration::LogEntry *> > mLog;
	int mModelVersion;

	qrRepo::details::Repository *mRepository; // Doesn't take ownership
};

}
}
