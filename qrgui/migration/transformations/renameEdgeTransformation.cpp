#include "migration/transformations/renameEdgeTransformation.h"

using namespace qReal::migration;

RenameEdgeTransformation::RenameEdgeTransformation(QString const &oldName, QString const &newName)
	: mOldName(oldName), mNewName(newName)
{
	Id oldElem("Migration", "MigrationDiagram", "Edge", QUuid::createUuid().toString());
	mBefore->addChild(Id::rootId(), oldElem);
	mBefore->setName(oldElem, oldName);
	mBefore->setProperty(oldElem, "links", "");

	Id newElem("Migration", "MigrationDiagram", "Edge", QUuid::createUuid().toString());
	mAfter->addChild(Id::rootId(), newElem);
	mAfter->setName(newElem, newName);
	mAfter->setProperty(newElem, "links", "");

	mElements[oldElem] = newElem;
}
