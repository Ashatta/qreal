#include "migration/transformations/renamePropertyTransformation.h"

using namespace qReal::migration;

RenamePropertyTransformation::RenamePropertyTransformation(QString const &parent, QString const &oldName
		, QString const &newName)
{
	mOldElementType = parent;
	mNewElementType = parent;

	mPropertyMap[oldName] = newName;
}
