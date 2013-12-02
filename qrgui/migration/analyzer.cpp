#include "migration/analyzer.h"

#include "migration/logEntries/renameEntry.h"
#include "migration/transformations/renameNodeTransformation.h"
#include "migration/transformations/renameEdgeTransformation.h"
#include "migration/transformations/renamePropertyTransformation.h"

using namespace qReal::migration;

Analyzer::Analyzer(QHash<qReal::Id, QList<qReal::migration::LogEntry *> > const &log)
	: mLog(log)
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
	foreach (qReal::Id const &id, mLog.keys()) {
		QHash<qReal::Id, QList<qReal::migration::RenameEntry *> > renames;
		foreach (LogEntry * const entry, mLog[id]) {
			RenameEntry * const renameEntry = dynamic_cast<RenameEntry *>(entry);
			if (renameEntry) {
				if (renameEntry->oldName() != renameEntry->newName()) {
					renames[renameEntry->id()] << renameEntry;
				}

				mLog[id].removeAll(entry);
			}
		}

		foreach (qReal::Id const &elemId, renames.keys()) {
			if (elemId.element() == "MetaEntityNode") {
				mTransformations[id] << new RenameNodeTransformation(renames[elemId].first()->oldName()
						, renames[elemId].last()->newName());
			} else if (elemId.element() == "MetaEntityEdge") {
				mTransformations[id] << new RenameEdgeTransformation(renames[elemId].first()->oldName()
						, renames[elemId].last()->newName());
			} else if (elemId.element() == "MetaEntity_Attribute") {
				mTransformations[id] << new RenamePropertyTransformation(renames[elemId].first()->parent()
						, renames[elemId].first()->oldName(), renames[elemId].last()->newName());
			}
		}
	}
}
