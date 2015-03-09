#include "migration.h"

using namespace qrRepo::details;

Migration::Migration(int fromVersion
		, int toVersion
		, const QString &fromVersionName
		, const QString &toVersionName
		, const QByteArray &fromData
		, const QByteArray &toData)
	: mFromVersion(fromVersion)
	, mToVersion(toVersion)
	, mFromVersionName(fromVersionName)
	, mToVersionName(toVersionName)
	, mFromData(fromData)
	, mToData(toData)
{

}
