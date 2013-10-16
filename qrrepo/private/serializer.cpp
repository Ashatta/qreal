#include "serializer.h"

#include <QtCore/QDir>
#include <QtCore/QDebug>
#include <QtCore/QPointF>
#include <QtGui/QPolygon>

#include "../../qrkernel/settingsManager.h"
#include "../../qrkernel/exception/exception.h"
#include "../../qrutils/outFile.h"
#include "../../qrutils/inFile.h"
#include "../../qrutils/xmlUtils.h"
#include "../../qrutils/fileSystemUtils.h"

#include "folderCompressor.h"
#include "classes/logicalObject.h"
#include "classes/graphicalObject.h"
#include "logSupport/versionEntry.h"

using namespace qrRepo;
using namespace details;
using namespace utils;
using namespace qReal;

Serializer::Serializer(QString const& saveDirName)
	: mWorkingDir(SettingsManager::value("temp").toString())
	, mWorkingFile(saveDirName)
{
	clearWorkingDir();
}

void Serializer::clearWorkingDir() const
{
	clearDir(mWorkingDir);
}

void Serializer::removeFromDisk(Id const &id) const
{
	QDir dir;
	dir.remove(pathToElement(id));
}

void Serializer::setWorkingFile(QString const &workingFile)
{
	mWorkingFile = workingFile;
}

void Serializer::saveToDisk(QList<Object*> const &objects, QHash<Id, QList<LogEntry *> > const &log) const
{
	Q_ASSERT_X(!mWorkingFile.isEmpty()
		, "Serializer::saveToDisk(...)"
		, "may be Repository of RepoApi (see Models constructor also) has been initialised with empty filename?");

	foreach (Object const * const object, objects) {
		QString const filePath = createDirectory(object->id(), object->isLogicalObject());

		QDomDocument doc;
		QDomElement root = object->serialize(doc);
		doc.appendChild(root);

		OutFile out(filePath);
		doc.save(out(), 2);
	}

	saveLog(log);

	QFileInfo fileInfo(mWorkingFile);
	QString fileName = fileInfo.baseName();

	QDir compressDir(SettingsManager::value("temp").toString());
	QDir dir = fileInfo.absolutePath();

	QFile previousSave(dir.absolutePath() + "/" + fileName +".qrs");
	if (previousSave.exists()) {
		previousSave.remove();
	}

	QString const filePath = fileInfo.absolutePath() + "/" + fileName + ".qrs";
	FolderCompressor::compressFolder(compressDir.absolutePath(), filePath);

	// Hiding autosaved files
	if (fileName.contains("~")) {
		FileSystemUtils::makeHidden(filePath);
	}

	clearDir(mWorkingDir);
}

void Serializer::loadFromDisk(QHash<qReal::Id, Object*> &objectsHash, QHash<Id, QList<LogEntry *> > &log, int &version)
{
	clearWorkingDir();
	if (!mWorkingFile.isEmpty()) {
		decompressFile(mWorkingFile);
	}

	QString const currentPath = SettingsManager::value("temp").toString();
	loadFromDisk(currentPath, objectsHash);
	loadLog(currentPath, log, version);
}

void Serializer::loadFromDisk(QString const &currentPath, QHash<qReal::Id, Object*> &objectsHash)
{
	QDir dir(currentPath + "/tree");
	if (dir.cd("logical")) {
		loadModel(dir, objectsHash);
		dir.cdUp();
		dir.cd("graphical");
		loadModel(dir, objectsHash);
	}
}

void Serializer::loadModel(QDir const &dir, QHash<qReal::Id, Object*> &objectsHash)
{
	foreach (QFileInfo const &fileInfo, dir.entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot)) {
		QString const path = fileInfo.filePath();
		if (fileInfo.isDir()) {
			loadModel(path, objectsHash);
		} else if (fileInfo.isFile()) {
			QDomDocument doc = xmlUtils::loadDocument(path);
			QDomElement const element = doc.documentElement();

			// To ensure backwards compatibility. Replace this by separate tag names when save updating mechanism
			// will be implemented.
			Object * const object = element.hasAttribute("logicalId") && element.attribute("logicalId") != "qrm:/"
					? dynamic_cast<Object *>(new GraphicalObject(element))
					: dynamic_cast<Object *>(new LogicalObject(element))
					;

			objectsHash.insert(object->id(), object);
		}
	}
}

void Serializer::loadLog(QString const &currentPath, QHash<Id, QList<LogEntry *> > &log, int &version) const
{
	QDir const dir(currentPath + "/logs");
	version = 0;

	foreach (QFileInfo const &fileInfo, dir.entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot)) {
		if (fileInfo.isFile()) {
			QStringList strings = InFile::readAll(fileInfo.filePath()).split('\n', QString::SkipEmptyParts);
			qReal::Id const id = qReal::Id::loadFromString(strings.first());
			strings.removeFirst();
			foreach (QString const &string, strings) {
				log[id] << LogEntry::loadFromString(string);
			}

			if (!log[id].empty()) {
				VersionEntry const * const entry = dynamic_cast<VersionEntry *>(log[id].last());
				if (entry) {
					int const lastVersion = entry->version();
					version = qMax(version, lastVersion);
				} else {
					qDebug() << "no version mark at the end of log file";
				}
			}
		}
	}
}

void Serializer::clearDir(QString const &path)
{
	if (path.isEmpty()) {
		return;
	}

	QDir dir(path);
	if (!dir.exists()) {
		return;
	}

	foreach (QFileInfo const &fileInfo, dir.entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot)) {
		if (fileInfo.isDir()) {
			clearDir(fileInfo.filePath());
			dir.rmdir(fileInfo.fileName());
		} else {
			dir.remove(fileInfo.fileName());
		}
	}
}

QString Serializer::pathToElement(Id const &id) const
{
	QString dirName = mWorkingDir;

	QStringList partsList = id.toString().split('/');
	Q_ASSERT(partsList.size() >=1 && partsList.size() <= 5);
	for (int i = 1; i < partsList.size() - 1; ++i) {
		dirName += "/" + partsList[i];
	}

	return dirName + "/" + partsList[partsList.size() - 1];
}

QString Serializer::createDirectory(Id const &id, bool logical) const
{
	QString dirName = mWorkingDir + "/tree";
	dirName += logical ? "/logical" : "/graphical";

	QStringList const partsList = id.toString().split('/');
	Q_ASSERT(partsList.size() >= 1 && partsList.size() <= 5);
	for (int i = 1; i < partsList.size() - 1; ++i) {
		dirName += "/" + partsList[i];
	}

	QDir dir;
	dir.rmdir(mWorkingDir);
	dir.mkpath(dirName);

	return dirName + "/" + partsList[partsList.size() - 1];
}

void Serializer::saveLog(QHash<Id, QList<LogEntry *> > const &log) const
{
	foreach (qReal::Id const &id, log.keys()) {
		QStringList const partsList = id.toString().split('/');
		Q_ASSERT(partsList.size() >= 1 && partsList.size() <= 5);

		QDir dir;
		dir.rmdir(mWorkingDir);
		dir.mkpath(mWorkingDir + "/logs");

		OutFile out(mWorkingDir + "/logs/" + partsList[partsList.size() - 1]);
		out() << id.toString() << "\n";
		foreach (LogEntry const * const entry, log[id]) {
			QString const string = entry->toString();
			if (!string.isEmpty()) {
				out() << string << "\n";
			}
		}
	}
}

void Serializer::decompressFile(QString const &fileName)
{
	FolderCompressor::decompressFolder(fileName, mWorkingDir);
}
