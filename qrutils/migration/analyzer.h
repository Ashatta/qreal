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

	/// finds chain (or cicle) of transformations containing \p trans and saves it in \p chain
	void findChain(
		ReplaceTypeTransformation * const trans
		, QList<ReplaceTypeTransformation *> &chain
		, QHash<QString, QHash<QString, ReplaceTypeTransformation *> > &transforms
		, QHash<QString, QString> const &newType
		, QHash<QString, QString> const &oldType
	) const;

	QHash<qReal::Id, QList<qReal::migration::Transformation *> > mTransformations;
	QHash<qReal::Id, QList<qReal::migration::LogEntry *> > mLog;

	QList<DifferenceModel *> const &mDifferenceModels;
};

}
}
