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

#include "paletteTreeWidgets.h"

#include "palette/paletteTree.h"
#include "palette/draggableElement.h"

using namespace qReal;
using namespace gui;

PaletteTreeWidgets::PaletteTreeWidgets(PaletteTree &parent, models::Models &models
		, EditorManagerInterface &editorManagerProxy, const QList<QAction *> &additionalActions)
	: QSplitter(Qt::Vertical)
	, mEditorManager(&editorManagerProxy)
	, mParentPalette(&parent)
	, mModels(models)
	, mEditorTree(new PaletteTreeWidget(parent, editorManagerProxy, false, additionalActions))
	, mUserTree(new PaletteTreeWidget(parent, editorManagerProxy, true, additionalActions))
	, mAdditionalActions(additionalActions)
{
	initWidgets();
}

PaletteTreeWidgets::PaletteTreeWidgets(PaletteTree &parent, models::Models &models
		, EditorManagerInterface &editorManagerProxy
		, const Id &editor, const Id &diagram, const QList<QAction *> &additionalActions)
	: QSplitter(Qt::Vertical)
	, mParentPalette(&parent)
	, mModels(models)
	, mEditor(editor)
	, mDiagram(diagram)
	, mEditorTree(new PaletteTreeWidget(parent, editorManagerProxy, false, additionalActions))
	, mUserTree(new PaletteTreeWidget(parent, editorManagerProxy, true, additionalActions))
	, mAdditionalActions(additionalActions)
{
	mEditorManager = &editorManagerProxy;
	initWidgets();
	initEditorTree();
	initUserTree();
}

void PaletteTreeWidgets::initWidgets()
{
	initWidget(mEditorTree);
	initWidget(mUserTree);
}

void PaletteTreeWidgets::initWidget(PaletteTreeWidget * const tree)
{
	connect(tree, &PaletteTreeWidget::requestForPropertiesChange
			, this, &PaletteTreeWidgets::requestForPropertiesChange);
	connect(tree, &PaletteTreeWidget::requestForAppearanceChange
			, this, &PaletteTreeWidgets::requestForAppearanceChange);
	connect(tree, &PaletteTreeWidget::requestForElementDeletion
			, this, &PaletteTreeWidgets::requestForElementDeletion);
	connect(tree, &PaletteTreeWidget::requestForElementCreation
			, this, &PaletteTreeWidgets::requestForElementCreation);

	tree->setHeaderHidden(true);
	tree->setSelectionMode(QAbstractItemView::NoSelection);
	addWidget(tree);
}

void PaletteTreeWidgets::initEditorTree()
{
	IdList elements = mEditorManager->elements(mDiagram) + mEditorManager->groups(mDiagram);
	const bool sort = mEditorManager->shallPaletteBeSorted(mEditor, mDiagram);
	if (sort) {
		PaletteTreeWidget::sortByFriendlyName(elements);
	}

	if (!mEditorManager->paletteGroups(mEditor, mDiagram).empty()) {
		QList<QPair<QString, QList<PaletteElement>>> groups;
		QMap<QString, QString> descriptions;
		for (const QString &group : mEditorManager->paletteGroups(mEditor, mDiagram)) {
			const QStringList paletteGroup = mEditorManager->paletteGroupList(mEditor, mDiagram, group);
			QList<PaletteElement> groupElements;
			for (const QString &name : paletteGroup) {
				for (const Id &element : elements) {
					if (element.element() == name) {
						groupElements << PaletteElement(*mEditorManager, element);
						break;
					}
				}
			}

			groups << qMakePair(group, groupElements);
			descriptions[group] = mEditorManager->paletteGroupDescription(mEditor, mDiagram, group);
		}

		mEditorTree->addGroups(groups, descriptions, false, mEditorManager->friendlyName(mDiagram), sort);
	} else {
		for (const Id &element : elements) {
			addTopItemType(PaletteElement(*mEditorManager, element), mEditorTree);
		}
	}
}

void PaletteTreeWidgets::initUserTree()
{
	refreshUserPalette();
	connect(&mModels.exploser(), &models::Exploser::explosionsSetCouldChange
			, this, &PaletteTreeWidgets::refreshUserPalette);
}

void PaletteTreeWidgets::addTopItemType(const PaletteElement &data, QTreeWidget *tree)
{
	QTreeWidgetItem *item = new QTreeWidgetItem;
	DraggableElement *element = createDraggableElement(data, mParentPalette->iconsView());

	mPaletteElements.insert(data.id(), element);

	tree->addTopLevelItem(item);
	tree->setItemWidget(item, 0, element);
}

void PaletteTreeWidgets::resizeIcons()
{
	if (mParentPalette->iconsView() && mParentPalette->itemsCountInARow() > 1) {
		const int iconSize = 48;
		const int widgetSize = this->size().width() - (iconSize << 1);
		const int itemsCount = maxItemsCountInARow();
		const int newSize = (widgetSize < itemsCount * iconSize)
				? (widgetSize / itemsCount) : iconSize;
		for (int i = 0; i < mEditorTree->topLevelItemCount(); i++) {
			for (int j = 0; j < mEditorTree->topLevelItem(i)->childCount(); j++) {
				QWidget *field = mEditorTree->itemWidget(mEditorTree->topLevelItem(i)->child(j), 0);
				if (!field) {
					break;
				}

				foreach (QObject *child, field->children()) {
					DraggableElement *element = dynamic_cast<DraggableElement*>(child);
					if (element) {
						element->setIconSize(newSize);
					}
				}
			}
		}
	}
}

int PaletteTreeWidgets::maxItemsCountInARow() const
{
	int max = 0;
	for (int i = 0; i < mEditorTree->topLevelItemCount(); i++) {
		for (int j = 0; j < mEditorTree->topLevelItem(i)->childCount(); j++) {
			QWidget *field = mEditorTree->itemWidget(mEditorTree->topLevelItem(i)->child(j), 0);
			if (!field) {
				break;
			}

			int itemsCount = field->children().count();
			if (itemsCount > max) {
				max = itemsCount;
			}
		}
	}
	return max;
}

void PaletteTreeWidgets::expand()
{
	mEditorTree->expand();
	mEditorTree->expand();
}

void PaletteTreeWidgets::collapse()
{
	mEditorTree->collapse();
	mUserTree->collapse();
}

void PaletteTreeWidgets::saveConfiguration(const QString &title) const
{
	saveConfiguration(mEditorTree, title);
	saveConfiguration(mUserTree, title);
}

void PaletteTreeWidgets::saveConfiguration(const PaletteTreeWidget *tree, const QString &title) const
{
	for (int j = 0; j < tree->topLevelItemCount(); j++) {
		const QTreeWidgetItem *topItem = tree->topLevelItem(j);
		if (topItem) {
			SettingsManager::setValue(title, topItem->isExpanded());
		}
	}
}

void PaletteTreeWidgets::setElementVisible(const Id &metatype, bool visible)
{
	if (mPaletteElements.contains(metatype)) {
		mPaletteElements[metatype]->setVisible(visible);
	} else {
		mEditorTree->setElementVisible(metatype, visible);
	}
}

void PaletteTreeWidgets::setVisibleForAllElements(bool visible)
{
	foreach (QWidget * const element, mPaletteElements.values()) {
		element->setVisible(visible);
	}

	mEditorTree->setVisibleForAllElements(visible);
}

void PaletteTreeWidgets::setElementEnabled(const Id &metatype, bool enabled)
{
	if (mPaletteElements.contains(metatype)) {
		mPaletteElements[metatype]->setEnabled(enabled);
	} else {
		mEditorTree->setElementEnabled(metatype, enabled);
	}
}

void PaletteTreeWidgets::setEnabledForAllElements(bool enabled)
{
	foreach (QWidget * const element, mPaletteElements.values()) {
		element->setEnabled(enabled);
	}

	mEditorTree->setEnabledForAllElements(enabled);
}

void PaletteTreeWidgets::customizeExplosionTitles(const QString &userGroupTitle, const QString &userGroupDescription)
{
	mUserGroupTitle = userGroupTitle;
	mUserGroupDescription = userGroupDescription;
}

void PaletteTreeWidgets::refreshUserPalette()
{
	QList<QPair<QString, QList<gui::PaletteElement>>> groups;
	QMap<QString, QString> descriptions = { { mUserGroupTitle, mUserGroupDescription } };
	QList<gui::PaletteElement> groupElements;

	const QMultiMap<Id, Id> types = mModels.exploser().explosions(mDiagram);
	for (const Id &source : types.uniqueKeys()) {
		for (const Id &target : types.values(source)) {
			groupElements << gui::PaletteElement(source
					, mModels.logicalRepoApi().name(target)
					, QString(), mEditorManager->icon(source)
					, mEditorManager->iconSize(source)
					, target);
		}
	}

	if (!groupElements.isEmpty()) {
		groups << qMakePair(mUserGroupTitle, groupElements);
	}

	mUserTree->addGroups(groups, descriptions, true, mEditorManager->friendlyName(mDiagram), true);
}

DraggableElement *PaletteTreeWidgets::createDraggableElement(const PaletteElement &paletteElement, bool iconsOnly)
{
	DraggableElement *element = new DraggableElement(paletteElement, iconsOnly, *mEditorManager, mAdditionalActions);

	connect(element, &DraggableElement::requestForPropertiesChange
			, this, &PaletteTreeWidgets::requestForPropertiesChange);
	connect(element, &DraggableElement::requestForAppearanceChange
			, this, &PaletteTreeWidgets::requestForAppearanceChange);
	connect(element, &DraggableElement::requestForElementDeletion
			, this, &PaletteTreeWidgets::requestForElementDeletion);

	return element;
}

void PaletteTreeWidgets::setAdditionalActions(const QList<QAction *> &additionalActions)
{
	mAdditionalActions = additionalActions;

	mEditorTree->setAdditionalActions(additionalActions);
	mUserTree->setAdditionalActions(additionalActions);

	for (DraggableElement * element : mPaletteElements.values()) {
		element->setAdditionalActions(additionalActions);
	}
}
