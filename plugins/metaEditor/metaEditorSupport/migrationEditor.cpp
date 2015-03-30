#include "migrationEditor.h"
#include "ui_migrationEditor.h"

#include <qrutils/inFile.h>
#include <qrutils/outFile.h>

using namespace metaEditor;

MigrationEditor::MigrationEditor(const QString &languageName
		, qrRepo::RepoApi *repo
		, QWidget *parent)
	: QWidget(parent)
	, ui(new Ui::MigrationEditor)
	, mLanguageName(languageName)
	, mRepo(repo)
	, mInterpreter("")
	, mModels("", mInterpreter, false)
	, mController(mModels.repoControlApi())
	, mPropertyModel(mInterpreter)
{
	ui->setupUi(this);

	mInterpreter.addPlugin(mLanguageName, mRepo);

	Id diagram = mInterpreter.diagrams(Id("migrationEditor")).at(0); // check
	diagram = Id(diagram.editor(), diagram.diagram(), mInterpreter.diagramNodeName(diagram.editor(), diagram.diagram()));
	Id created = mModels.graphicalModelAssistApi().createElement(Id::rootId(), diagram);
	if (created.isNull()) {
		return;
	}

	mModels.repoControlApi().addLogEntries(created, QList<qReal::migration::LogEntry *>());

	QModelIndex index = mModels.graphicalModelAssistApi().indexById(created);
	while (index.parent() != QModelIndex()) {
		index = index.parent();
	}

	Id const diagramId = mModels.graphicalModelAssistApi().idByIndex(index);
	mEditorView = new EditorView(mModels, mController, mSceneCustomizer, diagramId, ui->editorWidget);
	mController.diagramOpened(diagramId);
	mController.setActiveDiagram(diagramId);

	mEditorView->mutableMvIface().configure(mModels.graphicalModelAssistApi()
			, mModels.logicalModelAssistApi(), mModels.exploser());

	mEditorView->mutableMvIface().setModel(mModels.graphicalModel());
	if (mEditorView->sceneRect() == QRectF(0, 0, 0, 0)) {
		mEditorView->setSceneRect(0, 0, 1, 1);
	}

	mEditorView->mutableMvIface().setLogicalModel(mModels.logicalModel());
	mEditorView->mutableMvIface().setRootIndex(index);

	connect(mEditorView->scene(), SIGNAL(selectionChanged()), SLOT(sceneSelectionChanged()));
	connect(mModels.graphicalModel(), SIGNAL(rowsAboutToBeMoved(QModelIndex, int, int, QModelIndex, int))
			, &mEditorView->mvIface(), SLOT(rowsAboutToBeMoved(QModelIndex, int, int, QModelIndex, int)));
	connect(mModels.graphicalModel(), SIGNAL(rowsMoved(QModelIndex, int, int, QModelIndex, int))
			, &mEditorView->mvIface(), SLOT(rowsMoved(QModelIndex, int, int, QModelIndex, int)));

	mEditorView->mutableScene().initNodes();
	mEditorView->centerOn(mEditorView->scene()->sceneRect().topLeft());

	if (SettingsManager::value("PaletteTabSwitching").toBool()) {
		int i = 0;
		foreach (QString const &name, ui->paletteTree->editorsNames()) {
			Id const id = mModels.graphicalModelAssistApi().idByIndex(index);
			Id const diagramId = Id(id.editor(), id.diagram());
			QString const diagramName = mInterpreter.friendlyName(diagramId);
			if (diagramName == name) {
				ui->paletteTree->setComboBoxIndex(i);
				break;
			}
			i++;
		}
	}

	ui->propertyEditor->init(mModels.logicalModelAssistApi(), mController);
	ui->propertyEditor->setModel(&mPropertyModel);
	mPropertyModel.setSourceModels(mModels.logicalModel(), mModels.graphicalModel());

	ui->paletteTree->initModels(&mModels);
	ui->paletteTree->loadPalette(SettingsManager::value("PaletteRepresentation").toBool()
			, SettingsManager::value("PaletteIconsInARowCount").toInt()
			, &mInterpreter);
	SettingsManager::setValue("EditorsLoadedCount", mInterpreter.editors().count());
	ui->paletteTree->refreshUserPalettes();

	ui->paletteDock->setWidget(ui->paletteTree);
	ui->propertyEditorDock->setWidget(ui->propertyEditor);
}

MigrationEditor::~MigrationEditor()
{
	delete ui;
}

void MigrationEditor::sceneSelectionChanged()
{
	QList<Element*> selected;
	QList<QGraphicsItem*> items = mEditorView->scene()->items();

	foreach (QGraphicsItem* item, items) {
		Element* element = dynamic_cast<Element*>(item);
		if (element) {
			if (element->isSelected()) {
				selected.append(element);
				element->setSelectionState(true);
			} else {
				element->setSelectionState(false);
				element->select(false);
			}
		}
	}

	if (selected.isEmpty()) {
		mPropertyModel.clearModelIndexes();
	} else if (selected.length() > 1) {
		foreach(Element* notSingleSelected, selected) {
			notSingleSelected->select(false);
		}
	} else {
		Element* const singleSelected = selected.at(0);
		singleSelected->select(true);
		Id const id = singleSelected->id();

		if (mModels.graphicalModelAssistApi().isGraphicalId(id)) {
			Id const logicalId = mModels.graphicalModelAssistApi().logicalId(id);
			QModelIndex const logicalIndex = mModels.logicalModelAssistApi().indexById(logicalId);
			QModelIndex const graphicalIndex = mModels.graphicalModelAssistApi().indexById(id);
			mPropertyModel.setModelIndexes(logicalIndex, graphicalIndex);
		} else if (mModels.logicalModelAssistApi().isLogicalId(id)) {
			QModelIndex const logicalIndex = mModels.logicalModelAssistApi().indexById(id);
			mPropertyModel.setModelIndexes(logicalIndex, QModelIndex());
		} else {
			mPropertyModel.clearModelIndexes();
		}
	}
}

QByteArray MigrationEditor::serializedData()
{
	// todo: serialization into neat text format
	const QString tempFile = "tempMigrationFile.qrs";
	mModels.repoControlApi().saveTo(tempFile);
	QFile inFile(tempFile);
	inFile.open(QIODevice::ReadOnly);
	QByteArray result = inFile.readAll();
	inFile.remove();
	return result;
}
