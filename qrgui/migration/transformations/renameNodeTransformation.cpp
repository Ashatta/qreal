#include "migration/transformations/renameNodeTransformation.h"

using namespace qReal::migration;

RenameNodeTransformation::RenameNodeTransformation(QString const &oldName, QString const &newName)
	: mOldName(oldName), mNewName(newName)
{
}
