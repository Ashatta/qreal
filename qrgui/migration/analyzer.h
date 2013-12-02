#pragma once

#include "migration/logEntries/logEntry.h"
#include "migration/transformations/transformation.h"

namespace qReal {
namespace migration {

class Analyzer
{
public:
	Analyzer(QHash<qReal::Id, QList<qReal::migration::LogEntry *> > const &log);
	~Analyzer();

	void analyze();

private:
	void handleRenames();

	QHash<qReal::Id, QList<qReal::migration::Transformation *> > mTransformations;
	QHash<qReal::Id, QList<qReal::migration::LogEntry *> > mLog;
};

}
}
