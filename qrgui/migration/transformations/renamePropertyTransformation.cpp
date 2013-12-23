#include "migration/transformations/renamePropertyTransformation.h"

using namespace qReal::migration;

RenamePropertyTransformation::RenamePropertyTransformation(qReal::Id const &parent, QString const &oldName
		, QString const &newName)
	: mParent(parent), mOldName(oldName), mNewName(newName)
{
	Id oldElem("Migration", "MigrationDiagram", "Element", QUuid::createUuid().toString());
	mBefore->addChild(Id::rootId(), oldElem);
	mBefore->setName(oldElem, parent.element());
	mBefore->setProperty(oldElem, "links", "");

	Id newElem("Migration", "MigrationDiagram", "Element", QUuid::createUuid().toString());
	mAfter->addChild(Id::rootId(), newElem);
	mAfter->setName(newElem, parent.element());
	mAfter->setProperty(newElem, "links", "");

	mElements[oldElem] = newElem;
	mProperties[qMakePair(parent, oldName)] = qMakePair(parent, newName);
}
