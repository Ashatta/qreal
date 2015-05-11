/* Copyright 2007-2015 QReal Research Group
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License. */

#include "metaEditorSupportPlugin.h"

#include <QtCore/QProcess>
#include <QtWidgets/QApplication>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QProgressBar>
#include <QtWidgets/QDesktopWidget>

#include <qrkernel/settingsManager.h>
#include <qrmc/metaCompiler.h>
#include <qrgui/plugins/pluginManager/interpreterEditorManager.h>
#include <qrrepo/repoApi.h>

#include "editorGenerator.h"
#include "xmlParser.h"
#include "versionChooserDialog.h"
#include "migrationDialog.h"

using namespace qReal;
using namespace metaEditor;

MetaEditorSupportPlugin::MetaEditorSupportPlugin()
		: mGenerateEditorForQrxcAction(nullptr)
		, mGenerateEditorWithQrmcAction(nullptr)
		, mParseEditorXmlAction(nullptr)
		, mCommitNewVersionAction(nullptr)
		, mCreateMigrationAction(nullptr)
		, mRepoControlApi(nullptr)
		, mCompilerSettingsPage(new PreferencesCompilerPage())
{
}

MetaEditorSupportPlugin::~MetaEditorSupportPlugin()
{
}

void MetaEditorSupportPlugin::init(PluginConfigurator const &configurator)
{
	mMainWindowInterface = &configurator.mainWindowInterpretersInterface();
	mLogicalRepoApi = &configurator.logicalModelApi().mutableLogicalRepoApi();
	mGraphicalRepoApi = &configurator.graphicalModelApi().mutableGraphicalRepoApi();
	mRepoControlApi = &configurator.repoControlInterface();
}

QList<ActionInfo> MetaEditorSupportPlugin::actions()
{
	mGenerateEditorForQrxcAction.setText(tr("Generate editor"));
	ActionInfo generateEditorForQrxcActionInfo(&mGenerateEditorForQrxcAction, "generators", "tools");
	connect(&mGenerateEditorForQrxcAction, SIGNAL(triggered()), this, SLOT(generateEditorForQrxc()));

	mGenerateEditorWithQrmcAction.setText(tr("Generate editor (qrmc)"));
	ActionInfo generateEditorWithQrmcActionInfo(&mGenerateEditorWithQrmcAction, "generators", "tools");
	connect(&mGenerateEditorWithQrmcAction, SIGNAL(triggered()), this, SLOT(generateEditorWithQrmc()));

	mParseEditorXmlAction.setText(tr("Parse editor xml")); // button for parsing xml, doesn't work
	ActionInfo parseEditorXmlActionInfo(&mParseEditorXmlAction, "generators", "tools");
	connect(&mParseEditorXmlAction, SIGNAL(triggered()), this, SLOT(parseEditorXml()));

	mCommitNewVersionAction.setText(tr("Commit new version"));
	ActionInfo commitNewVersionActionInfo(&mCommitNewVersionAction, "generators", "tools");
	connect(&mCommitNewVersionAction, SIGNAL(triggered()), this, SLOT(commitNewVersion()));

	mCreateMigrationAction.setText(tr("Create migration"));
	ActionInfo createMigrationActionInfo(&mCreateMigrationAction, "generators", "tools");
	connect(&mCreateMigrationAction, SIGNAL(triggered()), this, SLOT(openMigrationDialog()));

	return QList<ActionInfo>() << generateEditorForQrxcActionInfo
			<< generateEditorWithQrmcActionInfo
			<< parseEditorXmlActionInfo
			<< commitNewVersionActionInfo
			<< createMigrationActionInfo;
}

QPair<QString, gui::PreferencesPage *> MetaEditorSupportPlugin::preferencesPage()
{
	return qMakePair(QObject::tr("Compiler"), static_cast<gui::PreferencesPage *>(mCompilerSettingsPage));
}

void MetaEditorSupportPlugin::generateEditorForQrxc()
{
	EditorGenerator editorGenerator(*mLogicalRepoApi, *mMainWindowInterface->errorReporter());

	QDir dir(".");

	QHash<Id, QPair<QString, QString> > metamodelList = editorGenerator.getMetamodelList();
	foreach (Id const &key, metamodelList.keys()) {
		QString const nameOfTheDirectory = metamodelList[key].first;
		QString const pathToQRealRoot = metamodelList[key].second;
		dir.mkpath(nameOfTheDirectory);
		QPair<QString, QString> const metamodelNames = editorGenerator.generateEditor(key, nameOfTheDirectory
			, pathToQRealRoot, mRepoControlApi);

		if (!mMainWindowInterface->errorReporter()->wereErrors()) {
			if (QMessageBox::question(mMainWindowInterface->windowWidget()
					, tr("loading.."), QString(tr("Do you want to load generated editor %1?")).arg(metamodelNames.first),
					QMessageBox::Yes, QMessageBox::No) == QMessageBox::No)
			{
				return;
			}
			loadNewEditor(nameOfTheDirectory, metamodelNames
					, SettingsManager::value("pathToQmake").toString()
					, SettingsManager::value("pathToMake").toString()
					, SettingsManager::value("pluginExtension").toString()
					, SettingsManager::value("prefix").toString()
					, mLogicalRepoApi->stringProperty(key, "buildConfiguration"));
		}
	}
	if (metamodelList.isEmpty()) {
		mMainWindowInterface->errorReporter()->addError(tr("There is nothing to generate"));
	}
}

void MetaEditorSupportPlugin::generateEditorWithQrmc()
{
	qrmc::MetaCompiler metaCompiler(qApp->applicationDirPath() + "/../qrmc", mLogicalRepoApi);

	IdList const metamodels = mLogicalRepoApi->children(Id::rootId());

	QProgressBar *progress = new QProgressBar(mMainWindowInterface->windowWidget());
	progress->show();
	int const progressBarWidth = 240;
	int const progressBarHeight = 20;

	QApplication::processEvents();
	QRect const screenRect = qApp->desktop()->availableGeometry();
	progress->move(screenRect.width() / 2 - progressBarWidth / 2, screenRect.height() / 2 - progressBarHeight / 2);
	progress->setFixedWidth(progressBarWidth);
	progress->setFixedHeight(progressBarHeight);
	progress->setRange(0, 100);

	int forEditor = 60 / metamodels.size();

	foreach (Id const &key, metamodels) {
		QString const objectType = key.element();
		if (objectType == "MetamodelDiagram" && mLogicalRepoApi->isLogicalElement(key)) {
			QString nameOfTheDirectory = mLogicalRepoApi->stringProperty(key, "name of the directory");
			QString nameOfMetamodel = mLogicalRepoApi->stringProperty(key, "name");
			QString nameOfPlugin = nameOfTheDirectory.split("/").last();

			if (QMessageBox::question(mMainWindowInterface->windowWidget()
					, tr("loading..")
					, QString(tr("Do you want to compile and load editor %1?")).arg(nameOfPlugin)
					, QMessageBox::Yes, QMessageBox::No)
					== QMessageBox::No)
			{
				continue;
			}

			progress->setValue(5);

			if (!metaCompiler.compile(nameOfMetamodel)) { // generating source code for all metamodels
				QMessageBox::warning(mMainWindowInterface->windowWidget()
						, tr("error")
						, tr("Cannot generate source code for editor ") + nameOfPlugin);
				continue;
			}
			progress->setValue(20);

			QProcess builder;
			builder.setWorkingDirectory("../qrmc/plugins");
			builder.start(SettingsManager::value("pathToQmake").toString());
			qDebug()  << "qmake";
			if ((builder.waitForFinished()) && (builder.exitCode() == 0)) {
				progress->setValue(40);

				builder.start(SettingsManager::value("pathToMake").toString());

				bool finished = builder.waitForFinished(100000);
				qDebug()  << "make";
				if (finished && (builder.exitCode() == 0)) {
					qDebug()  << "make ok";

					progress->setValue(progress->value() + forEditor / 2);

					QString normalizedName = nameOfPlugin.at(0).toUpper() + nameOfPlugin.mid(1);
					if (!nameOfPlugin.isEmpty()) {
						if (!mMainWindowInterface->unloadPlugin(normalizedName)) {
							QMessageBox::warning(mMainWindowInterface->windowWidget()
									, tr("error")
									, tr("cannot unload plugin ") + normalizedName);
							progress->close();
							delete progress;
							continue;
						}
					}

					QString const generatedPluginFileName = SettingsManager::value("prefix").toString()
							+ nameOfPlugin
							+ "."
							+ SettingsManager::value("pluginExtension").toString()
							;

					if (mMainWindowInterface->loadPlugin(generatedPluginFileName, normalizedName)) {
						progress->setValue(progress->value() + forEditor / 2);
					}
				}
				progress->setValue(100);
			}
		}
	}
	if (progress->value() != 100) {
		QMessageBox::warning(mMainWindowInterface->windowWidget(), tr("error"), tr("cannot load new editor"));
	}
	progress->setValue(100);
	progress->close();
	delete progress;
}

void MetaEditorSupportPlugin::parseEditorXml()
{
	if (!mMainWindowInterface->pluginLoaded("MetaEditor")) {
		QMessageBox::warning(mMainWindowInterface->windowWidget(), tr("error"), tr("required plugin (MetaEditor) is not loaded"));
		return;
	}
	QDir dir(".");
	QString directoryName = ".";
	while (dir.cdUp()) {
		QFileInfoList const infoList = dir.entryInfoList(QDir::Dirs);
		foreach (QFileInfo const &directory, infoList){
			if (directory.baseName() == "qrxml") {
				directoryName = directory.absolutePath() + "/qrxml";
			}
		}
	}
	QString const fileName = QFileDialog::getOpenFileName(mMainWindowInterface->windowWidget()
			, tr("Select xml file to parse")
			, directoryName
			, "XML files (*.xml)");

	if (fileName.isEmpty())
		return;

	XmlParser parser(*mLogicalRepoApi, *mGraphicalRepoApi);

	parser.parseFile(fileName);

	parser.loadIncludeList(fileName);

	mMainWindowInterface->reinitModels();
}

void MetaEditorSupportPlugin::commitNewVersion()
{
	if (!checkModel()) {
		QMessageBox::warning(mMainWindowInterface->windowWidget(), tr("error")
				, tr("Can't commit: current model is not valid"));
		return;
	}

	Id firstDiagram;
	for (Id const &id : mLogicalRepoApi->children(Id::rootId())) {
		if (mLogicalRepoApi->isLogicalElement(id)) {
			firstDiagram = id;
			break;
		}
	}

	mRepoControlApi->createNewVersion(mLogicalRepoApi->stringProperty(firstDiagram, "version"));

	mMainWindowInterface->errorReporter()->addInformation("Version created");
}

bool MetaEditorSupportPlugin::checkModel()
{
	return true;
}

void MetaEditorSupportPlugin::openMigrationDialog()
{
	VersionChooserDialog versionChooser(mRepoControlApi->versionNames(), mMainWindowInterface->windowWidget());
	connect(&versionChooser, SIGNAL(versionChosen(QString,int,QString,int))
			, this, SLOT(createMigrationsForVersions(QString,int,QString,int)));
	versionChooser.exec();
}

void MetaEditorSupportPlugin::createMigrationsForVersions(const QString &fromName, int from
		, const QString  &toName, int to)
{
	const QString name = mLogicalRepoApi->name(mLogicalRepoApi->elementsByType("MetamodelDiagram")[0]);  // guarantee correctness?
	const QString fromLanguage = name + "_" + fromName;
	const QString toLanguage = name + "_" + toName;

	MigrationDialog dialog(from, name, migrationLanguageForVersion(from)
			, to, name, migrationLanguageForVersion(to));
	connect(&dialog, &MigrationDialog::migrationCreated, this, &MetaEditorSupportPlugin::addNewMigration);
	dialog.exec();
}

qrRepo::RepoApi *MetaEditorSupportPlugin::migrationLanguageForVersion(int version)
{
	const QString tempFile = "currentModel.qrs";
	mRepoControlApi->saveTo(tempFile);
	qrRepo::RepoApi *repo = new qrRepo::RepoApi(tempFile);
	repo->rollBackTo(version);
	QFile::remove(tempFile);

	const Id migrationEditor("MetaEditor", "MetaEditor", "MetamodelDiagram", QUuid::createUuid().toString());
	repo->addChild(Id::rootId(), migrationEditor);
	repo->setProperty(migrationEditor, "name", "migrationEditor");
	repo->setProperty(migrationEditor, "displayedName", "Migration Editor");

	for (const Id &editor : repo->logicalElements(migrationEditor.type())) {
		if (editor == migrationEditor) {
			continue;
		}

		for (const Id &diagram : repo->children(editor)) {
			repo->setParent(diagram, migrationEditor);
			if (diagram.element() != "MetaEditorDiagramNode") {
				continue;
			}

			const QString diagramNodeName = repo->stringProperty(diagram, "nodeName");
			Id diagramNodeId;
			for (const Id &node : repo->children(diagram)) {
				if (repo->name(node) == diagramNodeName) {
					diagramNodeId = node;
					break;
				}
			}

			if (diagramNodeId.isNull()) {
				diagramNodeId = Id("MetaEditor", "MetaEditor", "MetaEntityNode", QUuid::createUuid().toString());
				repo->addChild(diagram, diagramNodeId);
				repo->setProperty(diagramNodeId, "name", diagramNodeName);
				repo->setProperty(diagramNodeId, "displayedName", diagramNodeName);

				const Id properties("MetaEditor", "MetaEditor", "MetaEntityPropertiesAsContainer"
						, QUuid::createUuid().toString());
				repo->addChild(diagramNodeId, properties);
				repo->setName(properties, "diagramNodeProperties");
				repo->setProperty(properties, "banChildrenMove", false);
			}

			if (!repo->hasProperty(diagramNodeId, "shape") || repo->stringProperty(diagramNodeId, "shape").isEmpty()) {
				const QString shape =
					"<graphics>\n"
					"    <picture sizex=\"300\" sizey=\"300\">\n"
					"        <rectangle fill=\"#ffffff\" stroke-style=\"solid\" stroke=\"#000000\" y1=\"0\" "
					"x1=\"0\" y2=\"300\" stroke-width=\"1\" x2=\"300\" fill-style=\"solid\"/>\n"
					"    </picture>\n"
					"</graphics>\n";

				repo->setProperty(diagramNodeId, "shape", shape);
			}

			const Id anyNodeId("MetaEditor", "MetaEditor", "MetaEntityNode", QUuid::createUuid().toString());
			repo->addChild(diagram, anyNodeId);
			repo->setName(anyNodeId, "AnyNode");
			repo->setProperty(anyNodeId, "displayedName", "Any Node");
			repo->setProperty(anyNodeId, "isResizeable", true);
			repo->setProperty(anyNodeId, "shape"
					, "<graphics>\n"
					"	<picture sizex=\"150\" sizey=\"100\">\n"
					"		<line fill=\"#000000\" stroke-style=\"solid\" stroke=\"#2f4f4f\""
					"y1=\"0\" x1=\"0\" y2=\"100\" stroke-width=\"2\" x2=\"0\" fill-style=\"solid\"/>\n"
					"		<line fill=\"#000000\" stroke-style=\"solid\" stroke=\"#2f4f4f\""
					"y1=\"0\" x1=\"150\" y2=\"100\" stroke-width=\"2\" x2=\"150\" fill-style=\"solid\"/>\n"
					"		<line fill=\"#000000\" stroke-style=\"solid\" stroke=\"#2f4f4f\""
					"y1=\"0\" x1=\"0\" y2=\"0\" stroke-width=\"2\" x2=\"150\" fill-style=\"solid\"/>\n"
					"		<line fill=\"#000000\" stroke-style=\"solid\" stroke=\"#2f4f4f\""
					"y1=\"100\" x1=\"0\" y2=\"100\" stroke-width=\"2\" x2=\"150\" fill-style=\"solid\"/>\n"
					"		<line fill=\"#000000\" stroke-style=\"solid\" stroke=\"#2f4f4f\""
					"y1=\"25a\" x1=\"0a\" y2=\"25a\" stroke-width=\"2\" x2=\"150\" fill-style=\"solid\"/>\n"
					"	</picture>\n"
					"	<labels>\n"
					"		<label x=\"5\" y=\"3\" textBinded=\"TypeName\" center=\"true\"/>\n"
					"	</labels>\n"
					"    <ports>\n"
					"        <pointPort x=\"0\" y=\"50\"/>\n"
					"        <pointPort x=\"150\" y=\"50\"/>\n"
					"        <pointPort x=\"75\" y=\"0\"/>\n"
					"        <pointPort x=\"75\" y=\"100\"/>\n"
					"    </ports>\n"
					"</graphics>\n"
			);

			const Id typePropertyId("MetaEditor", "MetaEditor", "MetaEntity_Attribute", QUuid::createUuid().toString());
			repo->addChild(anyNodeId, typePropertyId);
			repo->setName(typePropertyId, "TypeName");
			repo->setProperty(typePropertyId, "attributeType", "string");
			repo->setProperty(typePropertyId, "displayedName", "Type Name");
			repo->setProperty(typePropertyId, "defaultValue", "");

			const Id anyPropertyId("MetaEditor", "MetaEditor", "MetaEntityNode", QUuid::createUuid().toString());
			repo->addChild(diagram, anyPropertyId);
			repo->setName(anyPropertyId, "AnyProperty");
			repo->setProperty(anyPropertyId, "displayedName", "Any Property");
			repo->setProperty(anyPropertyId, "isResizeable", true);
			repo->setProperty(anyPropertyId, "shape"
					, "<graphics>\n"
					"	<picture sizex=\"140\" sizey=\"30\">\n"
					"	</picture>\n"
					"	<labels>\n"
					"		<label x=\"10\" y=\"0\" textBinded=\"Name\"/>\n"
					  "		<label x=\"70\" y=\"0\" textBinded=\"Value\"/>\n"
					"	</labels>\n"
					"	<ports/>\n"
					"</graphics>\n"
			);

			const Id anyPropertyName("MetaEditor", "MetaEditor", "MetaEntity_Attribute", QUuid::createUuid().toString());
			repo->addChild(anyPropertyId, anyPropertyName);
			repo->setName(anyPropertyName, "PropertyName");
			repo->setProperty(anyPropertyName, "attributeType", "string");
			repo->setProperty(anyPropertyName, "displayedName", "Property Name");
			repo->setProperty(anyPropertyName, "defaultValue", "");

			const Id anyPropertyValue("MetaEditor", "MetaEditor", "MetaEntity_Attribute", QUuid::createUuid().toString());
			repo->addChild(anyPropertyId, anyPropertyValue);
			repo->setName(anyPropertyValue, "Value");
			repo->setProperty(anyPropertyValue, "attributeType", "string");
			repo->setProperty(anyPropertyValue, "displayedName", "Value");
			repo->setProperty(anyPropertyValue, "defaultValue", "");

			const Id containerProperties("MetaEditor", "MetaEditor", "MetaEntityPropertiesAsContainer"
					, QUuid::createUuid().toString());
			repo->addChild(anyNodeId, containerProperties);
			repo->setName(containerProperties, "ContainerProperties");
			repo->setProperty(containerProperties, "sortContainer", true);
			repo->setProperty(containerProperties, "minimizeToChildren", true);
			repo->setProperty(containerProperties, "banChildrenMove", true);
			repo->setProperty(containerProperties, "forestallingSize", "10,30,10,10");
			repo->setProperty(containerProperties, "childrenForestallingSize", "5");

			const Id container("MetaEditor", "MetaEditor", "Container", QUuid::createUuid().toString());
			repo->addChild(diagram, container);
			repo->setName(container, "Container");
			repo->setTo(container, anyPropertyId);
			repo->setFrom(container, anyNodeId);

			const Id anyEdgeId("MetaEditor", "MetaEditor", "MetaEntityEdge", QUuid::createUuid().toString());
			repo->addChild(diagram, anyEdgeId);
			repo->setName(anyEdgeId, "AnyEdge");
			repo->setProperty(anyEdgeId, "displayedName", "Any Edge");
			repo->setProperty(anyEdgeId, "lineType", "solidLine");
			repo->setProperty(anyEdgeId, "labelText", "");

			const Id edgeTypeId("MetaEditor", "MetaEditor", "MetaEntity_Attribute", QUuid::createUuid().toString());
			repo->addChild(anyEdgeId, edgeTypeId);
			repo->setName(edgeTypeId, "TypeName");
			repo->setProperty(edgeTypeId, "attributeType", "string");
			repo->setProperty(edgeTypeId, "displayedName", "Type Name");
			repo->setProperty(edgeTypeId, "defaultValue", "");

			for (const Id &node : repo->children(diagram)) {
				if (node.type() == Id("MetaEditor", "MetaEditor", "MetaEntityNode")
						|| node.type() == Id("MetaEditor", "MetaEditor", "MetaEntityEdge")) {
					const Id idProperty("MetaEditor", "MetaEditor", "MetaEntity_Attribute"
							, QUuid::createUuid().toString());
					repo->addChild(node, idProperty);
					repo->setName(idProperty, "__migrationId__");
					repo->setProperty(idProperty, "displayedName", "__migrationId__");
					repo->setProperty(idProperty, "attributeType", "int");
					repo->setProperty(idProperty, "defaultValue", "");

					for (const Id &property : repo->children(node)) {
						repo->setProperty(property, "attributeType", "string");
					}
				}

				if (node.type() == Id("MetaEditor", "MetaEditor", "MetaEntityNode") && node != diagramNodeId) {
					bool contains = false;
					for (const Id &link : repo->outgoingLinks(node)) {
						if (link.type() == Id("MetaEditor", "MetaEditor", "Container")
								&& repo->otherEntityFromLink(link, node) == diagramNodeId) {
							contains = true;
							break;
						}
					}

					if (!contains) {
						const Id container("MetaEditor", "MetaEditor", "Container", QUuid::createUuid().toString());
						repo->addChild(diagram, container);
						repo->setName(container, "Container");
						repo->setFrom(container, diagramNodeId);
						repo->setTo(container, node);
					}
				}
			}
		}

		repo->removeElement(editor);
	}

	return repo;
}

void MetaEditorSupportPlugin::addNewMigration(int fromVersion, const QString &fromName, const QByteArray &fromData
		, int toVersion, const QString &toName, const QByteArray &toData)
{
	mRepoControlApi->addMigration(fromVersion, toVersion, fromName, toName, fromData, toData);
}

void MetaEditorSupportPlugin::loadNewEditor(QString const &directoryName
		, QPair<QString, QString> const &metamodelNames
		, QString const &commandFirst
		, QString const &commandSecond
		, QString const &extension
		, QString const &prefix
		, QString const &buildConfiguration
		)
{
	int const progressBarWidth = 240;
	int const progressBarHeight = 20;

	QString const metamodelName = metamodelNames.first;
	QString const normalizerMetamodelName = metamodelNames.second;

	if ((commandFirst == "") || (commandSecond == "") || (extension == "")) {
		QMessageBox::warning(mMainWindowInterface->windowWidget(), tr("error"), tr("please, fill compiler settings"));
		return;
	}

	QString const normalizeDirName = metamodelName.at(0).toUpper() + metamodelName.mid(1);

	QProgressBar * const progress = new QProgressBar(mMainWindowInterface->windowWidget());
	progress->show();

	QApplication::processEvents();

	QRect const screenRect = qApp->desktop()->availableGeometry();
	progress->move(screenRect.width() / 2 - progressBarWidth / 2, screenRect.height() / 2 - progressBarHeight / 2);
	progress->setFixedWidth(progressBarWidth);
	progress->setFixedHeight(progressBarHeight);
	progress->setRange(0, 100);
	progress->setValue(5);

	if (!mMainWindowInterface->unloadPlugin(normalizeDirName)) {
		progress->close();
		delete progress;
		return;
	}

	progress->setValue(20);

	QProcess builder;
	builder.setWorkingDirectory(directoryName);
	builder.start(commandFirst, {"CONFIG+=" + buildConfiguration});

	if ((builder.waitForFinished()) && (builder.exitCode() == 0)) {
		progress->setValue(60);
		builder.start(commandSecond);
		if (builder.waitForFinished() && (builder.exitCode() == 0)) {
			progress->setValue(80);

			if (mMainWindowInterface->loadPlugin(prefix + metamodelName + "." + extension, normalizeDirName)) {
				progress->setValue(100);
			}
		}
	}

	if (progress->value() == 20) {
		QMessageBox::warning(mMainWindowInterface->windowWidget(), tr("error"), tr("cannot qmake new editor"));
	} else if (progress->value() == 60) {
		QMessageBox::warning(mMainWindowInterface->windowWidget(), tr("error"), tr("cannot make new editor"));
	}

	progress->setValue(100);
	progress->close();
	delete progress;
}
