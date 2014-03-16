#pragma once

#include "migration/logEntries/logEntry.h"
#include "migration/transformations/transformation.h"

namespace qReal {
namespace migration {

class RenameEntry;

class Analyzer
{
public:
	Analyzer(QHash<qReal::Id, QList<qReal::migration::LogEntry *> > const &log);
	~Analyzer();

	void analyze();
	QList<qReal::migration::Transformation *> transformations() const;

private:
	void handleRenames();
	void initRenames(qReal::Id const &diagram, QHash<Id, QList<qReal::migration::RenameEntry *> > &elemRenames
			, QHash<Id, QList<qReal::migration::RenameEntry *> > &propRenames);
	void addElementRenames(qReal::Id const &diagram
			, QHash<Id, QList<qReal::migration::RenameEntry *> > const &elemRenames);
	void addPropertyRenames(qReal::Id const &diagram
			, QHash<Id, QList<qReal::migration::RenameEntry *> > const &elemRenames
			, QHash<Id, QList<qReal::migration::RenameEntry *> > const &propRenames);

	QHash<qReal::Id, QList<qReal::migration::Transformation *> > mTransformations;
	QHash<qReal::Id, QList<qReal::migration::LogEntry *> > mLog;
};

}
}
