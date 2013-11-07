#pragma once

#include <QtXml/QDomDocument>
#include <QtCore/QVariant>
#include <QtCore/QFile>
#include <QtCore/QDir>

#include "../../qrkernel/roles.h"
#include "classes/object.h"
#include "valuesSerializer.h"

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
	Serializer(QString const &saveDirName);

	void clearWorkingDir() const;
	void setWorkingFile(QString const &workingFile);

	void removeFromDisk(qReal::Id const &id) const;
	void saveToDisk(QList<Object *> const &objects, QHash<qReal::Id, QList<qReal::migration::LogEntry *> > const &log
			, QMap<QString, int> const &metamodelsVersions) const;
	void loadFromDisk(QHash<qReal::Id, Object *> &objectsHash
			, QHash<qReal::Id, QList<qReal::migration::LogEntry *> > &log, QMap<QString, int> &metamodelsVersions);

	void decompressFile(QString const &fileName);

private:
	static void clearDir(QString const &path);

	void loadFromDisk(QString const &currentPath, QHash<qReal::Id, Object *> &objectsHash);
	void loadModel(QDir const &dir, QHash<qReal::Id, Object *> &objectsHash);

	void saveLog(QHash<qReal::Id, QList<qReal::migration::LogEntry *> > const &log) const;
	void loadLog(QString const &currentPath, QHash<qReal::Id, QList<qReal::migration::LogEntry *> > &log) const;

	void saveMetamodelsVersions(QMap<QString, int> const &metamodelsVersions) const;
	void loadMetamodelsVersions(QString const &currentPath, QMap<QString, int> &metamodelsVersions) const;

	QString pathToElement(qReal::Id const &id) const;
	QString createDirectory(qReal::Id const &id, bool logical) const;

	QString mWorkingDir;
	QString mWorkingFile;
};

}
}
