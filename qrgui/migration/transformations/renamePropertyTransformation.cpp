#include "migration/transformations/renamePropertyTransformation.h"

using namespace qReal::migration;

RenamePropertyTransformation::RenamePropertyTransformation(qReal::Id const &parent, QString const &oldName
		, QString const &newName)
	: mParent(parent), mOldName(oldName), mNewName(newName)
{
}
