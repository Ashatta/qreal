#include "changePropertyEntry.h"

using namespace qReal::migration;

ChangePropertyEntry::ChangePropertyEntry(const Id &id, const QString &property
		, const QVariant &oldValue, const QVariant &newValue)
	: mId(id), mProperty(property), mOldValue(oldValue), mNewValue(newValue)
{
}

QString ChangePropertyEntry::toString() const
{
	return QString("changeProperty@%1@%2@%3@%4").arg(mId.toString(), mProperty
			, mOldValue.toString(), mNewValue.toString());
}

void ChangePropertyEntry::reverse(qrRepo::CommonRepoApi *repo) const
{
	repo->setProperty(mId, mProperty, mOldValue);
}
