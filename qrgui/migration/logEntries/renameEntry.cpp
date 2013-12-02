#include "qrgui/migration/logEntries/renameEntry.h"

using namespace qReal::migration;

RenameEntry::RenameEntry(qReal::Id const &id, qReal::Id const &parent, QString const &oldName, QString const &newName)
	: mId(id), mParentId(parent), mOldName(oldName), mNewName(newName)
{
}

QString RenameEntry::toString() const
{
	return "rename@" + mId.toString() + "@" + mOldName + ">" + mNewName + "@parent=" + mParentId.toString();
}

void RenameEntry::reverse(qrRepo::details::Repository *repo) const
{
	repo->setProperty(mId, "name", mOldName);
}

qReal::Id RenameEntry::id() const
{
	return mId;
}

qReal::Id RenameEntry::parent() const
{
	return mParentId;
}

QString RenameEntry::oldName() const
{
	return mOldName;
}

QString RenameEntry::newName() const
{
	return mNewName;
}
