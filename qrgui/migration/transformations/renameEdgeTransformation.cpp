#include "migration/transformations/renameEdgeTransformation.h"

using namespace qReal::migration;

RenameEdgeTransformation::RenameEdgeTransformation(QString const &oldName, QString const &newName)
{
	mOldElementType = oldName;
	mNewElementType = newName;

}
