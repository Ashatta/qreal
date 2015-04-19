#include "removeElementEntry.h"

#include <qrrepo/graphicalRepoApi.h>
#include <qrrepo/logicalRepoApi.h>

using namespace qReal::migration;

RemoveElementEntry::RemoveElementEntry(const Id &id, const QMap<QString, QVariant> &properties
		, const Id &parent, const Id &logical)
	: mId(id), mProperties(properties), mParent(parent), mLogicalId(logical)
{
}

QString RemoveElementEntry::toString() const
{
	QStringList properties;
	for (const QString &key : mProperties.keys()) {
		properties << QString("%1>%2").arg(key, mProperties[key].toString());
	}

	QString result = QString("removeElement@%1@%2@%3").arg(mId.toString(), properties.join('|'), mParent.toString());
	if (!mLogicalId.isNull()) {
		result += "@" + mLogicalId.toString();
	}

	return result;
}

void RemoveElementEntry::reverse(qrRepo::CommonRepoApi *repo) const
{
	if (mLogicalId.isNull()) {
		qrRepo::LogicalRepoApi *logicalRepo = dynamic_cast<qrRepo::LogicalRepoApi *>(repo);
		if (!logicalRepo) {
			qDebug() << "common repo is not logical repo? that's weird";
			return;
		}

		logicalRepo->addChild(mParent, mId);
		for (const QString &key : mProperties.keys()) {
			logicalRepo->setProperty(mId, key, mProperties[key]);
		}
	} else {
		qrRepo::GraphicalRepoApi *graphicalRepo = dynamic_cast<qrRepo::GraphicalRepoApi *>(repo);
		if (!graphicalRepo) {
			qDebug() << "common repo is not graphical repo? that's weird";
			return;
		}

		graphicalRepo->addChild(mParent, mId, mLogicalId);
		graphicalRepo->setProperties(mId, mProperties);
	}
}
