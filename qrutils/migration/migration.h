#pragma once

#include <QtCore/QMetaType>
#include <QtCore/QVariant>
#include <QtCore/QString>

#include <qrutils/utilsDeclSpec.h>

namespace qReal {
namespace migration {

/// Plain old data object for storing information about migration
class QRUTILS_EXPORT Migration
{
public:
	Migration() {}

	Migration(const QString &name
			, int fromVersion
			, int toVersion
			, const QString &fromVersionName
			, const QString &toVersionName
			, const QByteArray &fromData
			, const QByteArray &toData)
		: mName(name)
		, mFromVersion(fromVersion)
		, mToVersion(toVersion)
		, mFromVersionName(fromVersionName)
		, mToVersionName(toVersionName)
		, mFromData(fromData)
		, mToData(toData)
	{}

	QVariant toVariant() const
	{
		QVariant result;
		result.setValue(*this);
		return result;
	}

	QString mName;
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

Q_DECLARE_METATYPE(qReal::migration::Migration)
