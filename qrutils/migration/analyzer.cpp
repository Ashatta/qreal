#include "migration/analyzer.h"

#include "migration/logEntries/logEntry.h"

using namespace qReal::migration;

Analyzer::Analyzer(QHash<qReal::Id, QList<qReal::migration::LogEntry *> > const &log
		, QList<DifferenceModel *> const &differenceModels)
	: mLog(log), mDifferenceModels(differenceModels)
{
}

Analyzer::~Analyzer()
{
	foreach (Id const &id, mTransformations.keys()) {
		foreach (Transformation *transform, mTransformations[id]) {
			delete transform;
		}
	}
}

void Analyzer::analyze()
{
	handleRenames();
}

void Analyzer::handleRenames()
{
	foreach (DifferenceModel const * const diff, mDifferenceModels) {
		mTransformations[Id()].append(diff->replacedTypes());
	}
}

QList<qReal::migration::Transformation *> Analyzer::transformations() const
{
	QList<Transformation *> result;
	foreach (QList<Transformation *> const &list, mTransformations.values()) {
		result.append(list);
	}

	return result;
}
