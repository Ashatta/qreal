#include "qrgui/migration/logEntries/renameEntry.h"

using namespace qReal::migration;

RenameEntry::RenameEntry(qReal::Id const &id, QString const &oldName, QString const &newName)
	: mId(id), mOldName(oldName), mNewName(newName)
{
}

QString RenameEntry::toString() const
{
	return "rename@" + mId.toString() + "@" + mOldName + ">" + mNewName;
}
