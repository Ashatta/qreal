#include "migration/logEntries/versionEntry.h"

using namespace qReal::migration;

VersionEntry::VersionEntry(int version, QString const &name)
	: mVersion(version)
	, mName(name)
{
}

QString VersionEntry::toString() const
{
	return "version@" + QString::number(mVersion) + "@" + mName;
}

int VersionEntry::version() const
{
	return mVersion;
}

QString VersionEntry::name() const
{
	return mName;
}
