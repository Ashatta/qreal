#pragma once

#include <QtXml/QDomDocument>
#include <QtCore/QVariant>
#include <QtCore/QFile>
#include <QtCore/QDir>

#include <qrkernel/roles.h>

#include "classes/object.h"
#include "valuesSerializer.h"
#include "migration.h"

namespace qReal {
namespace migration {
	class LogEntry;
}
}

namespace qrRepo {
namespace details {

/// Class that is responsible for saving repository contents to disk as .qrs file.
class Serializer
{
public:
	Serializer(const QString &saveDirName, bool compressSaves);

	void clearWorkingDir() const;
	void setWorkingFile(const QString &workingFile);

	void removeFromDisk(const qReal::Id &id) const;
	void saveToDisk(const QList<Object *> &objects, const QHash<QString, QVariant> &metaInfo
			, const QHash<qReal::Id, QList<qReal::migration::LogEntry *> > &log
			, const QMap<QString, int> &metamodelsVersions, const QList<Migration> &migrations) const;
	void loadFromDisk(QHash<qReal::Id, Object *> &objectsHash, QHash<QString, QVariant> &metaInfo
			, QHash<qReal::Id, QList<qReal::migration::LogEntry *> > &log, QMap<QString, int> &metamodelsVersions
			, QList<Migration> &migrations);

	void decompressFile(const QString &fileName);

private:
	static void clearDir(const QString &path);

	void loadFromDisk(const QString &currentPath, QHash<qReal::Id, Object *> &objectsHash);
	void loadModel(const QDir &dir, QHash<qReal::Id, Object *> &objectsHash);

	void saveLog(QHash<qReal::Id, QList<qReal::migration::LogEntry *> > const &log) const;
	void loadLog(QString const &currentPath, QHash<qReal::Id, QList<qReal::migration::LogEntry *> > &log) const;

	void saveMetamodelsVersions(QMap<QString, int> const &metamodelsVersions) const;
	void loadMetamodelsVersions(QString const &currentPath, QMap<QString, int> &metamodelsVersions) const;

	void saveMetaInfo(QHash<QString, QVariant> const &metaInfo) const;
	void loadMetaInfo(QHash<QString, QVariant> &metaInfo) const;

	void saveMigrations(const QList<Migration> &migrations) const;
	void loadMigrations(QList<Migration> &migrations) const;

	QString pathToElement(const qReal::Id &id) const;
	QString createDirectory(const qReal::Id &id, bool logical) const;

	QString mWorkingDir;
	QString mWorkingFile;
	bool mCompressSaves;
};

}
}
