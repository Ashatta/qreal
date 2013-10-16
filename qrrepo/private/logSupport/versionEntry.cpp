#include "private/logSupport/versionEntry.h"

using namespace qReal;

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
