#include "migrationManager.h"
#include "ui_migrationManager.h"

#include "migrationDialog.h"
#include "versionChooserDialog.h"

#include "qrutils/migration/migration.h"

#include <QtWidgets/QMessageBox>

using namespace metaEditor;

MigrationManager::MigrationManager(qrRepo::RepoControlInterface &repoControlApi
			, qrRepo::LogicalRepoApi &logicalRepoApi, QWidget *parent)
	: QDialog(parent)
	, ui(new Ui::MigrationManager)
	, mRepoControlApi(repoControlApi)
	, mLogicalRepoApi(logicalRepoApi)
	, mUnsaved(false)
{
	ui->setupUi(this);

	connect(ui->addButton, &QPushButton::clicked, this, &MigrationManager::chooseVersions);
	connect(ui->deleteButton, &QPushButton::clicked, this, &MigrationManager::deleteMigrations);
	connect(ui->editButton, &QPushButton::clicked, this, &MigrationManager::editMigration);
	connect(ui->upButton, &QPushButton::clicked, this, &MigrationManager::up);
	connect(ui->downButton, &QPushButton::clicked, this, &MigrationManager::down);

	connect(ui->buttonBox->button(QDialogButtonBox::Ok), &QPushButton::clicked, this, &MigrationManager::save);
	connect(ui->buttonBox->button(QDialogButtonBox::Save), &QPushButton::clicked, this, &MigrationManager::save);

	connect(ui->migrationsList, &QListWidget::itemSelectionChanged, this, &MigrationManager::selectionChanged);

	int lastVersion = -1;
	for (const qReal::migration::Migration &migration : mRepoControlApi.migrations()) {
		if (migration.mFromVersion > lastVersion) {
			lastVersion = migration.mFromVersion;
			ui->migrationsList->addItem(versionItem(migration.mFromVersionName, lastVersion));
		}

		ui->migrationsList->addItem(migrationItem(migration));
	}

	ui->migrationsList->setSelectionMode(QAbstractItemView::ExtendedSelection);
	selectionChanged();
}

MigrationManager::~MigrationManager()
{
	delete ui;
}

void MigrationManager::chooseVersions()
{
	VersionChooserDialog versionChooser(mRepoControlApi.versionNames(), this);
	connect(&versionChooser, SIGNAL(versionChosen(QString,QString,int,QString,int))
			, this, SLOT(createMigration(QString,QString,int,QString,int)));
	versionChooser.exec();

}

void MigrationManager::createMigration(QString name, const QString &fromName, int from
		, const QString &toName, int to)
{
	if (name.isEmpty()) {
		name = "Migration Rule";
	}

	// guarantee correctness?
	const QString languageName = mLogicalRepoApi.name(mLogicalRepoApi.elementsByType("MetamodelDiagram")[0]);
	MigrationDialog dialog(name, from, fromName, migrationLanguageForVersion(from)
			, to, toName, migrationLanguageForVersion(to), languageName);
	connect(&dialog, &MigrationDialog::migrationCreated, this, &MigrationManager::setMigration);
	dialog.exec();

}

void MigrationManager::editMigration()
{
	QListWidgetItem *selected = ui->migrationsList->selectedItems()[0];
	qReal::migration::Migration migration = selected->data(Qt::UserRole).value<qReal::migration::Migration>();

	const QString languageName = mLogicalRepoApi.name(mLogicalRepoApi.elementsByType("MetamodelDiagram")[0]);
	const int from = migration.mFromVersion;
	const int to = migration.mToVersion;
	MigrationDialog dialog(migration.mName, from, migration.mFromVersionName, migrationLanguageForVersion(from)
			, to, migration.mToVersionName, migrationLanguageForVersion(to), languageName);
	dialog.setMigration(migration, ui->migrationsList->row(selected));
	connect(&dialog, &MigrationDialog::migrationCreated, this, &MigrationManager::setMigration);
	dialog.exec();
}

void MigrationManager::setMigration(const qReal::migration::Migration &migration, int idx)
{
	mUnsaved = true;

	if (idx != -1) {
		ui->migrationsList->item(idx)->setData(Qt::UserRole, migration.toVariant());
		return;
	}

	bool foundCurrentVersion = false;
	int lastVersion = -1;
	if (idx == -1) {
		idx = ui->migrationsList->count();
		for (int i = 0; i < ui->migrationsList->count(); ++i) {
			QListWidgetItem *item = ui->migrationsList->item(i);
			if (item->data(Qt::UserRole).type() != QVariant::Int) {
				continue;
			}

			int currentVersion = item->data(Qt::UserRole).toInt();
			if (currentVersion <= migration.mFromVersion) {
				lastVersion = currentVersion;
				if (currentVersion == migration.mFromVersion) {
					foundCurrentVersion = true;
				}

				continue;
			}

			idx = i;
			if (lastVersion < migration.mFromVersion) {
				ui->migrationsList->insertItem(idx, versionItem(migration.mFromVersionName, migration.mFromVersion));
				idx++;
			}

			break;
		}
	}

	if (idx == ui->migrationsList->count() && !foundCurrentVersion) {
		ui->migrationsList->insertItem(idx, versionItem(migration.mFromVersionName, migration.mFromVersion));
		idx++;
	}

	ui->migrationsList->insertItem(idx, migrationItem(migration));
}

void MigrationManager::deleteMigrations()
{
	mUnsaved = true;

	for (QListWidgetItem *item : ui->migrationsList->selectedItems()) {
		ui->migrationsList->removeItemWidget(item);
		delete item;
	}

	deleteVersions();
	selectionChanged();
}

void MigrationManager::deleteVersions()
{
	for (int i = 0; i < ui->migrationsList->count() - 1; ++i) {
		QListWidgetItem *current = ui->migrationsList->item(i);
		QListWidgetItem *next = ui->migrationsList->item(i + 1);
		if (current->data(Qt::UserRole).type() == QVariant::Int && next->data(Qt::UserRole).type() == QVariant::Int) {
			ui->migrationsList->removeItemWidget(current);
			delete current;
			--i;
		}
	}

	if (ui->migrationsList->count() > 0) {
		QListWidgetItem *last = ui->migrationsList->item(ui->migrationsList->count() - 1);
		if (last->data(Qt::UserRole).type() == QVariant::Int) {
			ui->migrationsList->removeItemWidget(last);
			delete last;
		}
	}
}

void MigrationManager::up()
{
	move(-1);
}

void MigrationManager::down()
{
	move(1);
}

void MigrationManager::move(int direction)
{
	mUnsaved = true;

	int idx = ui->migrationsList->row(ui->migrationsList->selectedItems()[0]);
	QListWidgetItem *item = ui->migrationsList->takeItem(idx);
	ui->migrationsList->insertItem(idx + direction, item);
	ui->migrationsList->setCurrentRow(idx + direction);
}

void MigrationManager::save()
{
	if (mUnsaved) {
		QList<qReal::migration::Migration> result;
		for (int i = 0; i < ui->migrationsList->count(); ++i) {
			QVariant data = ui->migrationsList->item(i)->data(Qt::UserRole);
			if (data.type() != QVariant::Int) {
				result << data.value<qReal::migration::Migration>();
			}
		}

		mRepoControlApi.setMigrations(result);
	}
}

void MigrationManager::selectionChanged()
{
	const int selected = ui->migrationsList->selectedItems().size();
	ui->deleteButton->setEnabled(selected > 0);
	ui->editButton->setEnabled(selected == 1);
	ui->upButton->setEnabled(selected == 1 && check(-1));
	ui->downButton->setEnabled(selected == 1 && check(1));
}

bool MigrationManager::check(int direction) const
{
	QListWidgetItem *item = ui->migrationsList->selectedItems()[0];
	QListWidgetItem *neighbour = ui->migrationsList->item(ui->migrationsList->row(item) + direction);
	return neighbour && (neighbour->data(Qt::UserRole).type() != QVariant::Int);
}

qrRepo::RepoApi *MigrationManager::migrationLanguageForVersion(int version)
{
	const QString tempFile = "currentModel.qrs";
	mRepoControlApi.saveTo(tempFile);
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

QListWidgetItem *MigrationManager::versionItem(const QString &label, int version)
{
	QListWidgetItem *item = new QListWidgetItem(label);
	item->setBackgroundColor(Qt::darkGray);
	item->setData(Qt::UserRole, version);
	item->setFlags(Qt::NoItemFlags);
	return item;
}

QListWidgetItem *MigrationManager::migrationItem(const migration::Migration &migration)
{
	QListWidgetItem *item = new QListWidgetItem(migration.mName);
	item->setData(Qt::UserRole, migration.toVariant());
	item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
	return item;
}
