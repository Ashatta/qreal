#pragma once

#include <QtCore/QString>

#include <qrutils/utilsDeclSpec.h>

namespace qReal {
namespace migration {

/// Plain old data object for storing information about migration
class QRUTILS_EXPORT Migration
{
public:
	Migration(int fromVersion
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
	{}

	int mFromVersion;
	int mToVersion;
	QString mFromVersionName;
	QString mToVersionName;
	QByteArray mFromData;
	QByteArray mToData;
};

inline bool operator<(const Migration &first, const Migration &second)
{
	return first.mFromVersion < second.mFromVersion;
}

}
}
