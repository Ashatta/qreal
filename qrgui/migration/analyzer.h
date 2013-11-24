#pragma once

#include "migration/logEntries/logEntry.h"

namespace qReal {
namespace migration {

class Analyzer
{
public:
	Analyzer(QHash<qReal::Id, QList<qReal::migration::LogEntry *> > const &log);
};

}
}
