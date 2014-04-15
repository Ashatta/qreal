#pragma once

#include "qrkernel/ids.h"

namespace qReal {
namespace migration {

class LogEntry;

class LoggerInterface
{
public:
	virtual ~LoggerInterface() {}

	virtual void addLogEntry(qReal::Id const &diagram, qReal::migration::LogEntry * const entry) = 0;
	virtual void deleteLogEntry(qReal::Id const &diagram) = 0;
	virtual int version() const = 0;

	virtual void createNewVersion() = 0;

	virtual void clear() = 0;
	virtual void reset(QHash<qReal::Id, QList<qReal::migration::LogEntry *> > const &log) = 0;
	virtual QHash<qReal::Id, QList<qReal::migration::LogEntry *> > log() const = 0;
	virtual QHash<qReal::Id, QList<qReal::migration::LogEntry *> > logBetween(int startVersion, int endVersion) const = 0;

	virtual void rollBackTo(int version) = 0;
};

}
}
