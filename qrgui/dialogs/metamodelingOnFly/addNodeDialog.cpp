#include "addNodeDialog.h"
#include "ui_addNodeDialog.h"

#include <QtWidgets/QMessageBox>

#include "mainWindow/mainWindow.h"
#include "restoreElementDialog.h"

using namespace qReal;
using namespace gui;

AddNodeDialog::AddNodeDialog(MainWindow &mainWindow, Id const &diagram
		, EditorManagerInterface const &editorManagerProxy)
		: QDialog(&mainWindow)
		, mUi(new Ui::AddNodeDialog)
		, mMainWindow(mainWindow)
		, mDiagram(diagram)
		, mEditorManagerProxy(editorManagerProxy)
{
	mUi->setupUi(this);
	connect(mUi->okPushButton, SIGNAL(clicked()), this, SLOT(okButtonClicked()));
}

AddNodeDialog::~AddNodeDialog()
{
	delete mUi;
}

void AddNodeDialog::okButtonClicked()
{
	if (mUi->nameEdit->text().isEmpty()) {
		QMessageBox::critical(this, tr("Error"), tr("All required properties should be filled!"));
	} else {
		mNodeName = mUi->nameEdit->text();
		IdList const nodesWithTheSameNameList = mEditorManagerProxy.elementsWithTheSameName(mDiagram
				, mUi->nameEdit->text(), "MetaEntityNode");
		if (!nodesWithTheSameNameList.isEmpty()) {
			mNodeName = mUi->nameEdit->text() + "_" + nodesWithTheSameNameList.count();
			mRestoreElementDialog = new RestoreElementDialog(this, mMainWindow, mEditorManagerProxy, nodesWithTheSameNameList);
			mRestoreElementDialog->setModal(true);
			mRestoreElementDialog->show();
			connect(mRestoreElementDialog, &qReal::RestoreElementDialog::createNewChosen
					, this, &AddNodeDialog::addNode);
			connect(mRestoreElementDialog, &qReal::RestoreElementDialog::restoreChosen
					, this, &AddNodeDialog::done);
		} else {
			addNode();
		}
	}
}

void AddNodeDialog::addNode()
{
	mEditorManagerProxy.addNodeElement(mDiagram, mNodeName, mUi->nameEdit->text(), mUi->checkBox->isChecked());
	mMainWindow.loadPlugins();
	done(QDialog::Accepted);
}
