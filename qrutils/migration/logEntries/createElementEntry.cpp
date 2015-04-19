#include "createElementEntry.h"

using namespace qReal::migration;

CreateElementEntry::CreateElementEntry(const Id &id)
	: mId(id)
{
}

QString CreateElementEntry::toString() const
{
	return "createElement@" + mId.toString();
}

void CreateElementEntry::reverse(qrRepo::CommonRepoApi *repo) const
{
	repo->removeChild(repo->parent(mId), mId);
	repo->removeElement(mId);
}
