#include "migration/logEntries/renameEntry.h"

#include "qrrepo/graphicalRepoApi.h"

using namespace qReal::migration;

RenameEntry::RenameEntry(qReal::Id const &id, Id const &parent, QString const &parentName, QString const &oldName
		, QString const &newName)
	: mId(id), mParent(parent), mParentName(parentName), mOldName(oldName), mNewName(newName)
{
}

QString RenameEntry::toString() const
{
	return "rename@" + mId.toString() + "@" + mOldName + ">" + mNewName + "@parent=" + mParent.toString()
			+ "@parentName=" + mParentName;
}

void RenameEntry::reverse(qrRepo::CommonRepoApi *repo) const
{
	qrRepo::GraphicalRepoApi *graphicalRepo = dynamic_cast<qrRepo::GraphicalRepoApi *>(repo);
	if (!graphicalRepo) {
		return;
	}

	graphicalRepo->setProperty(mId, "name", mOldName);
	foreach (Id const &elem, graphicalRepo->graphicalElements()) {
		if (graphicalRepo->logicalId(elem) == mId) {
			graphicalRepo->setProperty(elem, "name", mOldName);
		}
	}
}

qReal::Id RenameEntry::id() const
{
	return mId;
}

qReal::Id RenameEntry::parent() const
{
	return mParent;
}

QString RenameEntry::parentName() const
{
	return mParentName;
}

QString RenameEntry::oldName() const
{
	return mOldName;
}

QString RenameEntry::newName() const
{
	return mNewName;
}
