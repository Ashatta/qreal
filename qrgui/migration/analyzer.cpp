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
		QHash<qReal::Id, QList<qReal::migration::RenameEntry *> > elemRenames;
		QHash<qReal::Id, QList<qReal::migration::RenameEntry *> > propRenames;
		initRenames(id, elemRenames, propRenames);

		addElementRenames(id, elemRenames);
		addPropertyRenames(id, elemRenames, propRenames);
	}
}

void Analyzer::initRenames(qReal::Id const &diagram
		, QHash<Id, QList<qReal::migration::RenameEntry *> > &elemRenames
		, QHash<Id, QList<qReal::migration::RenameEntry *> > &propRenames)
{
	foreach (LogEntry * const entry, mLog[diagram]) {
		RenameEntry * const renameEntry = dynamic_cast<RenameEntry *>(entry);
		if (renameEntry) {
			if (renameEntry->oldName() != renameEntry->newName()) {
				Id const elemId = renameEntry->id();
				if (elemId.element() == "MetaEntity_Attribute") {
					propRenames[elemId] << renameEntry;
				} else {
					elemRenames[elemId] << renameEntry;
				}
			}

			mLog[diagram].removeAll(entry);
		}
	}
}

void Analyzer::addElementRenames(qReal::Id const &diagram
		, QHash<qReal::Id, QList<qReal::migration::RenameEntry *> > const &elemRenames)
{
	foreach (qReal::Id const &elemId, elemRenames.keys()) {
		if (elemId.element() == "MetaEntityNode") {
			mTransformations[diagram] << new RenameNodeTransformation(elemRenames[elemId].first()->oldName()
					, elemRenames[elemId].last()->newName());
		} else if (elemId.element() == "MetaEntityEdge") {
			mTransformations[diagram] << new RenameEdgeTransformation(elemRenames[elemId].first()->oldName()
					, elemRenames[elemId].last()->newName());
		}
	}
}

void Analyzer::addPropertyRenames(qReal::Id const &diagram
		, QHash<Id, QList<qReal::migration::RenameEntry *> > const &elemRenames
		, QHash<Id, QList<qReal::migration::RenameEntry *> > const &propRenames)
{
	foreach (qReal::Id const &propId, propRenames.keys()) {
		qReal::Id const parent = propRenames[propId].first()->parent();
		QString const parentName = elemRenames.contains(parent) ? elemRenames[parent].last()->newName()
				: propRenames[propId].first()->parentName();
		mTransformations[diagram] << new RenamePropertyTransformation(parentName, propRenames[propId].first()->oldName()
				, propRenames[propId].last()->newName());
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
