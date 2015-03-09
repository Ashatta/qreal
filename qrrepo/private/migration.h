#pragma once

#include <QtCore/QString>

namespace qrRepo {
namespace details {

/// Plain old data object for storing information about migration
class Migration
{
public:
	Migration(int fromVersion
			, int toVersion
			, const QString &fromVersionName
			, const QString &toVersionName
			, const QByteArray &fromData
			, const QByteArray &toData);

	int mFromVersion;
	int mToVersion;
	QString mFromVersionName;
	QString mToVersionName;
	QByteArray mFromData;
	QByteArray mToData;
};

}
}
