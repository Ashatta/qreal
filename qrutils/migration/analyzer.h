#pragma once

#include "qrutils/utilsDeclSpec.h"

#include "qrutils/migration/differenceModel.h"
#include "qrutils/migration/transformations/replaceTypeTransformation.h"

namespace qReal {
namespace migration {

class LogEntry;

class QRUTILS_EXPORT Analyzer
{
public:
	Analyzer(QHash<qReal::Id, QList<qReal::migration::LogEntry *> > const &log
			, QList<DifferenceModel *> const &differenceModels);
	~Analyzer();

	void analyze();
	QList<qReal::migration::Transformation *> transformations() const;

private:
	void handleRenames();

	QHash<qReal::Id, QList<qReal::migration::Transformation *> > mTransformations;
	QHash<qReal::Id, QList<qReal::migration::LogEntry *> > mLog;

	QList<DifferenceModel *> const &mDifferenceModels;
};

}
}
