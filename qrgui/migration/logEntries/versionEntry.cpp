#include "qrgui/migration/logEntries/versionEntry.h"

using namespace qReal::migration;

VersionEntry::VersionEntry(int version)
	: mVersion(version)
{
}

QString VersionEntry::toString() const
{
	return "version@" + QString::number(mVersion);
}

int VersionEntry::version() const
{
	return mVersion;
}
