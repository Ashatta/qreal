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
#include "qrutils/migration/logEntries/logEntry.h"

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

void Serializer::saveToDisk(QList<Object *> const &objects, QHash<QString, QVariant> const &metaInfo
		, QHash<Id, QList<migration::LogEntry *> > const &log, QMap<QString, int> const &metamodelsVersions) const
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
	saveMetamodelsVersions(metamodelsVersions);
	saveMetaInfo(metaInfo);

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

void Serializer::loadFromDisk(QHash<qReal::Id, Object*> &objectsHash, QHash<QString, QVariant> &metaInfo
		, QHash<Id, QList<migration::LogEntry *> > &log, QMap<QString, int> &metamodelsVersions)
{
	clearWorkingDir();
	if (!mWorkingFile.isEmpty()) {
		decompressFile(mWorkingFile);
	}

	QString const currentPath = SettingsManager::value("temp").toString();
	loadFromDisk(currentPath, objectsHash);
	loadMetaInfo(metaInfo);
	loadLog(currentPath, log);
	loadMetamodelsVersions(currentPath, metamodelsVersions);
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

void Serializer::loadLog(QString const &currentPath, QHash<Id, QList<migration::LogEntry *> > &log) const
{
	QDir const dir(currentPath + "/logs");

	foreach (QFileInfo const &fileInfo, dir.entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot)) {
		if (fileInfo.isFile()) {
			QStringList strings = InFile::readAll(fileInfo.filePath()).split('\n', QString::SkipEmptyParts);
			qReal::Id const id = qReal::Id::loadFromString(strings.first());
			strings.removeFirst();
			foreach (QString const &string, strings) {
				log[id] << migration::LogEntry::loadFromString(string);
			}
		}
	}
}

void Serializer::loadMetamodelsVersions(QString const &currentPath, QMap<QString, int> &metamodelsVersions) const
{
	if (QFile::exists(currentPath + "metamodelsVersions.txt")) {
		QStringList const &strings
				= InFile::readAll(currentPath + "metamodelsVersions.txt").split('\n', QString::SkipEmptyParts);
		foreach (QString const &string, strings) {
			QStringList const &nameAndVersion = string.split("=");
			int version = 1;
			if (nameAndVersion.size() >= 1 && !nameAndVersion[1].isEmpty()) {
				bool ok = true;
				version = nameAndVersion[1].toInt(&ok);
				if (!ok) {
					version = 1;
				}
			}

			metamodelsVersions[nameAndVersion[0]] = version;
		}
	}
}

void Serializer::saveMetaInfo(QHash<QString, QVariant> const &metaInfo) const
{
	QDomDocument document;
	QDomElement root = document.createElement("metaInformation");
	document.appendChild(root);
	for (QString const &key : metaInfo.keys()) {
		QDomElement element = document.createElement("info");
		element.setAttribute("key", key);
		element.setAttribute("type", metaInfo[key].typeName());
		element.setAttribute("value", ValuesSerializer::serializeQVariant(metaInfo[key]));
		root.appendChild(element);
	}

	QString const filePath = mWorkingDir + "/metaInfo.xml";
	OutFile out(filePath);
	out() << document.toString(4);
}

void Serializer::loadMetaInfo(QHash<QString, QVariant> &metaInfo) const
{
	metaInfo.clear();

	QString const filePath = mWorkingDir + "/metaInfo.xml";
	QDomDocument const document = xmlUtils::loadDocument(filePath);
	for (QDomElement child = document.documentElement().firstChildElement("info")
			; !child.isNull()
			; child = child.nextSiblingElement("info"))
	{
		metaInfo[child.attribute("key")] = ValuesSerializer::deserializeQVariant(
				child.attribute("type"), child.attribute("value"));
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

void Serializer::saveLog(QHash<Id, QList<migration::LogEntry *> > const &log) const
{
	foreach (qReal::Id const &id, log.keys()) {
		QStringList const partsList = id.toString().split('/');
		Q_ASSERT(partsList.size() >= 1 && partsList.size() <= 5);

		QDir dir;
		dir.rmdir(mWorkingDir);
		dir.mkpath(mWorkingDir + "/logs");

		OutFile out(mWorkingDir + "/logs/" + partsList[partsList.size() - 1]);
		out() << id.toString() << "\n";
		foreach (migration::LogEntry const * const entry, log[id]) {
			QString const string = entry->toString();
			if (!string.isEmpty()) {
				out() << string << "\n";
			}
		}
	}
}

void Serializer::saveMetamodelsVersions(QMap<QString, int> const &metamodelsVersions) const
{
	OutFile out(mWorkingDir + "metamodelsVersions.txt");
	foreach (QString const &name, metamodelsVersions.keys()) {
		out() << name << "=" << metamodelsVersions[name] << "\n";
	}
}

void Serializer::decompressFile(QString const &fileName)
{
	FolderCompressor::decompressFolder(fileName, mWorkingDir);
}
