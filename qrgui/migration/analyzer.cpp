#include "migration/analyzer.h"

using namespace qReal::migration;

Analyzer::Analyzer(QHash<qReal::Id, QList<qReal::migration::LogEntry *> > const &log)
{
	Q_UNUSED(log)
}
