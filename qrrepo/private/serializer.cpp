#include "serializer.h"

#include <QtCore/QDir>
#include <QtCore/QDebug>
#include <QtCore/QPointF>
#include <QtCore/QCoreApplication>
#include <QtGui/QPolygon>

#include <qrkernel/settingsManager.h>
#include <qrkernel/exception/exception.h>
#include <qrutils/outFile.h>
#include <qrutils/inFile.h>
#include <qrutils/xmlUtils.h>
#include <qrutils/fileSystemUtils.h>

#include "folderCompressor.h"
#include "classes/logicalObject.h"
#include "classes/graphicalObject.h"
#include "qrutils/migration/logEntries/logEntry.h"

using namespace qrRepo;
using namespace details;
using namespace utils;
using namespace qReal;

const QString unsavedDir = "unsaved";

Serializer::Serializer(const QString& saveDirName, bool compressSaves)
	: mWorkingDir(QCoreApplication::applicationDirPath() + "/" + unsavedDir)
	, mWorkingFile(saveDirName)
	, mCompressSaves(compressSaves)
{
	clearWorkingDir();
	/// @todo: throw away this legacy piece of sh.t
	SettingsManager::setValue("temp", mWorkingDir);
	QDir dir(QCoreApplication::applicationDirPath());
	if (!dir.cd(mWorkingDir)) {
		QDir().mkdir(mWorkingDir);
	}
}

void Serializer::clearWorkingDir() const
{
	clearDir(mWorkingDir);
}

void Serializer::removeFromDisk(const Id &id) const
{
	QDir dir;
	dir.remove(pathToElement(id));
}

void Serializer::setWorkingFile(const QString &workingFile)
{
	mWorkingFile = workingFile;
}

void Serializer::saveToDisk(QList<Object *> const &objects, QHash<QString, QVariant> const &metaInfo
		, QHash<Id, QList<migration::LogEntry *> > const &log, QMap<QString, int> const &metamodelsVersions
		, const QList<qReal::migration::Migration> &migrations) const
{
	Q_ASSERT_X(!mWorkingFile.isEmpty()
		, "Serializer::saveToDisk(...)"
		, "may be Repository of RepoApi (see Models constructor also) has been initialised with empty filename?");

	clearDir(mWorkingDir);

	foreach (const Object * const object, objects) {
		const QString filePath = createDirectory(object->id(), object->isLogicalObject());

		QDomDocument doc;
		QDomElement root = object->serialize(doc);
		doc.appendChild(root);

		OutFile out(filePath);
		doc.save(out(), 2);
	}

	saveLog(log);
	saveMetamodelsVersions(metamodelsVersions);
	saveMetaInfo(metaInfo);
	saveMigrations(migrations);

	QFileInfo fileInfo(mWorkingFile);
	QString fileName = fileInfo.baseName();

	QDir compressDir(SettingsManager::value("temp").toString());
	QDir dir = fileInfo.absolutePath();

	QFile previousSave(dir.absolutePath() + "/" + fileName +".qrs");
	if (previousSave.exists()) {
		previousSave.remove();
	}

	const QString filePath = fileInfo.absolutePath() + "/" + fileName + ".qrs";
	FolderCompressor::compressFolder(compressDir.absolutePath(), filePath, mCompressSaves);

	// Hiding autosaved files
	if (fileName.contains("~")) {
		FileSystemUtils::makeHidden(filePath);
	}

	clearDir(mWorkingDir);
}

void Serializer::loadFromDisk(QHash<qReal::Id, Object*> &objectsHash, QHash<QString, QVariant> &metaInfo
		, QHash<Id, QList<migration::LogEntry *> > &log, QMap<QString, int> &metamodelsVersions
		, QList<qReal::migration::Migration> &migrations)
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
	loadMigrations(migrations);
}

void Serializer::loadFromDisk(const QString &currentPath, QHash<qReal::Id, Object*> &objectsHash)
{
	QDir dir(currentPath + "/tree");
	if (dir.cd("logical")) {
		loadModel(dir, objectsHash);
		dir.cdUp();
		dir.cd("graphical");
		loadModel(dir, objectsHash);
	}
}

void Serializer::loadModel(const QDir &dir, QHash<qReal::Id, Object*> &objectsHash)
{
	foreach (const QFileInfo &fileInfo, dir.entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot)) {
		const QString path = fileInfo.filePath();
		if (fileInfo.isDir()) {
			loadModel(path, objectsHash);
		} else if (fileInfo.isFile()) {
			QDomDocument doc = xmlUtils::loadDocument(path);
			const QDomElement element = doc.documentElement();

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
	for (const QString &key : metaInfo.keys()) {
		QDomElement element = document.createElement("info");
		element.setAttribute("key", key);
		element.setAttribute("type", metaInfo[key].typeName());
		element.setAttribute("value", ValuesSerializer::serializeQVariant(metaInfo[key]));
		root.appendChild(element);
	}

	const QString filePath = mWorkingDir + "/metaInfo.xml";
	OutFile out(filePath);
	out() << document.toString(4);
}

void Serializer::loadMetaInfo(QHash<QString, QVariant> &metaInfo) const
{
	metaInfo.clear();

	const QString filePath = mWorkingDir + "/metaInfo.xml";
	if (!QFile::exists(filePath)) {
		return;
	}

	const QDomDocument document = xmlUtils::loadDocument(filePath);
	for (QDomElement child = document.documentElement().firstChildElement("info")
			; !child.isNull()
			; child = child.nextSiblingElement("info"))
	{
		metaInfo[child.attribute("key")] = ValuesSerializer::deserializeQVariant(
				child.attribute("type"), child.attribute("value"));
	}
}

void Serializer::saveMigrations(const QList<qReal::migration::Migration> &migrations) const
{
	if (migrations.isEmpty()) {
		return;
	}

	QDir dir;
	dir.mkpath(mWorkingDir + "/migrations");

	int i = 0;
	for (const qReal::migration::Migration &migration : migrations) {
		const QString dirPath = mWorkingDir + "/migrations/" + QString::number(i++);
		dir.mkpath(dirPath);

		OutFile migrationInfo(dirPath + "/migrationInfo.xml");
		QDomDocument document;
		QDomElement root = document.createElement("migrationInfo");
		document.appendChild(root);
		root.setAttribute("fromVersion", migration.mFromVersion);
		root.setAttribute("toVersion", migration.mToVersion);
		root.setAttribute("fromVersionName", migration.mFromVersionName);
		root.setAttribute("toVersionName", migration.mToVersionName);
		document.save(migrationInfo(), QDomNode::EncodingFromTextStream);

		QFile outFrom(dirPath + "/from.qrs");
		outFrom.open(QIODevice::WriteOnly);
		outFrom.write(migration.mFromData);

		QFile outTo(dirPath + "/to.qrs");
		outTo.open(QIODevice::WriteOnly);
		outTo.write(migration.mToData);
	}
}

void Serializer::loadMigrations(QList<qReal::migration::Migration> &migrations) const
{
	migrations.clear();

	const QString migrationDir = mWorkingDir + "/migrations";
	if (!QFile::exists(migrationDir)) {
		return;
	}

	for (int i = 0; ; ++i) {
		const QString dirPath = migrationDir + "/" + QString::number(i);
		if (!QFile::exists(dirPath)) {
			return;
		}

		QFile inFrom(dirPath + "/from.qrs");
		inFrom.open(QIODevice::ReadOnly);
		QFile inTo(dirPath + "/to.qrs");
		inTo.open(QIODevice::ReadOnly);

		QDomDocument document = xmlUtils::loadDocument(dirPath + "/migrationInfo.xml");
		QDomElement root = document.firstChildElement("migrationInfo");
		migrations << qReal::migration::Migration(root.attribute("fromVersion").toInt()
				, root.attribute("toVersion").toInt()
				, root.attribute("fromVersionName"), root.attribute("toVersionName")
				, inFrom.readAll(), inTo.readAll());
	}
}

void Serializer::clearDir(const QString &path)
{
	if (path.isEmpty()) {
		return;
	}

	QDir dir(path);
	if (!dir.exists()) {
		return;
	}

	foreach (const QFileInfo &fileInfo, dir.entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot)) {
		if (fileInfo.isDir()) {
			clearDir(fileInfo.filePath());
			dir.rmdir(fileInfo.fileName());
		} else {
			dir.remove(fileInfo.fileName());
		}
	}
}

QString Serializer::pathToElement(const Id &id) const
{
	QString dirName = mWorkingDir;

	QStringList partsList = id.toString().split('/');
	Q_ASSERT(partsList.size() >=1 && partsList.size() <= 5);
	for (int i = 1; i < partsList.size() - 1; ++i) {
		dirName += "/" + partsList[i];
	}

	return dirName + "/" + partsList[partsList.size() - 1];
}

QString Serializer::createDirectory(const Id &id, bool logical) const
{
	QString dirName = mWorkingDir + "/tree";
	dirName += logical ? "/logical" : "/graphical";

	const QStringList partsList = id.toString().split('/');
	Q_ASSERT(partsList.size() >= 1 && partsList.size() <= 5);
	for (int i = 1; i < partsList.size() - 1; ++i) {
		dirName += "/" + partsList[i];
	}

	QDir dir;
	dir.rmdir(mWorkingDir);
	dir.mkpath(dirName);

	return dirName + "/" + partsList[partsList.size() - 1];
}

void Serializer::saveLog(const QHash<Id, QList<migration::LogEntry *> > &log) const
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

void Serializer::saveMetamodelsVersions(const QMap<QString, int> &metamodelsVersions) const
{
	OutFile out(mWorkingDir + "metamodelsVersions.txt");
	foreach (QString const &name, metamodelsVersions.keys()) {
		out() << name << "=" << metamodelsVersions[name] << "\n";
	}
}

void Serializer::decompressFile(const QString &fileName)
{
	FolderCompressor::decompressFolder(fileName, mWorkingDir, mCompressSaves);
}
