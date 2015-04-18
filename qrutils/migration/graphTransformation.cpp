#include "graphTransformation.h"

#include <qrgui/plugins/pluginManager/editorManagerInterface.h>

using namespace qReal::migration;

GraphTransformation::GraphTransformation(LogicalModelAssistInterface &logicalRepoApi
		, GraphicalModelAssistInterface &graphicalRepoApi
		, qrRepo::GraphicalRepoApi &fromTemplate
		, qrRepo::GraphicalRepoApi &toTemplate)
	: BaseGraphTransformationUnit(logicalRepoApi, graphicalRepoApi, nullptr)
	, mFromTemplate(fromTemplate)
	, mToTemplate(toTemplate)
{
	mIgnoreProperties = { "from", "links", "name", "to", "fromPort", "toPort", "__migrationId__"
			, "outgoingExplosion", "incomingExplosions", "position", "configuration"
			, "name", "childrenOrder", "expanded", "folded", "linkShape", "isView" };
}

void GraphTransformation::apply()
{
	mCreatedElements.clear();
	analyzeTemplates();

	findMatch();
//	while (findMatch()) {
		for (const QHash<Id, Id> &match : mMatches) {
			mCurrentMatch = match;
			saveProperties();
			Id diagram = migrationDiagram(mToTemplate);
			Id diagramInModel = mGraphicalModelApi.graphicalRepoApi().parent(match.values()[0]);
			mToModel.clear();
			createNodes(diagram, diagramInModel);
			createLinks(diagram, diagramInModel);
			setNodesGraphicalProperties(diagram);
			setLinksGraphicalProperties(diagram);
			deleteElements();
		}
//	}
}

void GraphTransformation::analyzeTemplates()
{
	mIdToFrom.clear();
	mIdToTo.clear();

	elementsFromTemplate(mIdToFrom, mFromTemplate, migrationDiagram(mFromTemplate));
	elementsFromTemplate(mIdToTo, mToTemplate, migrationDiagram(mToTemplate));
}

void GraphTransformation::elementsFromTemplate(QHash<QString, Id> &elements
		, qrRepo::GraphicalRepoApi &migrationTemplate
		, const Id &root)
{
	for (const Id &child : migrationTemplate.children(root)) {
		QString migrationId = migrationTemplate.property(migrationTemplate.logicalId(child), "__migrationId__")
				.toString();
		if (!migrationId.isEmpty()) {
			elements[migrationId] = child;
		}


		elementsFromTemplate(elements, migrationTemplate, child);
	}
}

bool GraphTransformation::findMatch()
{
	mMatches.clear();

	return checkRuleMatching();
}

bool GraphTransformation::checkRuleMatching()
{
	IdList elementsForMatching;
	for (const Id &diagram : mGraphicalModelApi.children(Id::rootId())) {
		if (mGraphicalModelApi.isGraphicalId(diagram)) {
			elementsForMatching << mGraphicalModelApi.children(diagram);
		}
	}

	return BaseGraphTransformationUnit::checkRuleMatching(elementsForMatching);
}

qReal::Id GraphTransformation::startElement() const
{
	Id diagram = migrationDiagram(mFromTemplate);

	foreach (Id const &element, mFromTemplate.children(diagram)) {
		if (!isEdgeInRule(element)) {
			return element;
		}
	}

	return Id::rootId();
}

bool GraphTransformation::compareElementTypesAndProperties(const Id &first, const Id &second)
{
	if (second.diagram() == "MigrationDiagram" && second.element() == "AnyNode") {
		return isEdgeInModel(first);
	} else if (second.diagram() == "MigrationDiagram" && second.element() == "AnyEdge") {
		return !isEdgeInModel(first);
	} else if (first.element() == second.element() && first.diagram() == second.diagram()) {
		QMap<QString, QVariant> secondProperties = mFromTemplate.properties(mFromTemplate.logicalId(second));
		foreach (const QString &key, secondProperties.keys()) {
			const QString value = secondProperties.value(key).toString();
			if (value.isEmpty() || mIgnoreProperties.contains(key)) {
				continue;
			}

			if (!hasProperty(first, key)) {
				return false;
			}

			QRegExp propertyRegExp("^" + value + "$");
			QString prop = property(first, key).toString();
			if (propertyRegExp.indexIn(prop) == -1) {
				return false;
			}
		}

		return true;
	}

	return false;
}

qReal::Id GraphTransformation::toInRule(const Id &id) const
{
	return mFromTemplate.to(id);
}

qReal::Id GraphTransformation::fromInRule(const Id &id) const
{
	return mFromTemplate.from(id);
}

qReal::IdList GraphTransformation::linksInRule(const Id &id) const
{
	return mFromTemplate.links(id);
}

qReal::Id GraphTransformation::migrationDiagram(qrRepo::GraphicalRepoApi &migrationTemplate) const
{
	Id result;
	for (const Id &diagram : migrationTemplate.children(Id::rootId())) {
		if (migrationTemplate.isGraphicalElement(diagram)) {
			result = diagram;
			break;
		}
	}

	return result;
}

void GraphTransformation::saveProperties()
{
	mPropertiesMatches.clear();

	for (const Id &key : mCurrentMatch.keys()) {
		QString migrationId = mFromTemplate.property(mFromTemplate.logicalId(key), "__migrationId__").toString();
		if (!migrationId.isEmpty()) {
			QMap<QString, QVariant> properties(mFromTemplate.properties(mFromTemplate.logicalId(key)));

			for (const QString &propertyName : properties.keys()) {
				if (mIgnoreProperties.contains(propertyName)) {
					continue;
				}

				QRegExp propertyRegExp("^" + properties[propertyName].toString() + "$");
				propertyRegExp.indexIn(property(mCurrentMatch[key], propertyName).toString());
				mPropertiesMatches[migrationId][propertyName] << propertyRegExp.capturedTexts();
				mPropertiesMatches[migrationId][propertyName][0] = property(mCurrentMatch[key], propertyName).toString();
			}
		}
	}
}

void GraphTransformation::createNodes(const Id &root, const Id &createdRoot)
{
	for (const Id &child : mToTemplate.children(root)) {
		if (mToTemplate.from(child) == Id::rootId() && mToTemplate.to(child) == Id::rootId()) {
			// todo: creation of "any nodes"
			const Id created = createElement(createdRoot, Id(createdRoot.editor(), child.diagram(), child.element()));
			createNodes(child, created);
			setLogicalProperties(created, child); // todo: names
			mToModel[child] = created;
		}
	}
}

void GraphTransformation::createLinks(const Id &root, const Id &diagram)
{
	for (const Id &child : mToTemplate.children(root)) {
		if (mToTemplate.from(child) != Id::rootId() || mToTemplate.to(child) != Id::rootId()) {
			const Id created = createElement(diagram, Id(diagram.editor(), child.diagram(), child.element()));
			setLogicalProperties(created, child);
			mToModel[child] = created;
		} else {
			createLinks(child, diagram);
		}
	}
}

qReal::Id GraphTransformation::createElement(const Id &parent, const Id &type)
{
	int oldVersion = mLogicalModelApi.logicalRepoApi().metamodelVersion(parent.editor());
	Id created = mGraphicalModelApi.createElement(parent, type);
	mLogicalModelApi.mutableLogicalRepoApi().addUsedMetamodel(created.editor(), oldVersion);
	mCreatedElements << created << mGraphicalModelApi.logicalId(created);
	return created;
}

void GraphTransformation::setLogicalProperties(const Id &created, const Id &rule)
{
	const Id logicalRule = mToTemplate.logicalId(rule);
	const QString migrationId = mToTemplate.stringProperty(logicalRule, "__migrationId__");
	const Id logicalId = mGraphicalModelApi.logicalId(created);

	for (const QString &propertyName : mToTemplate.properties(logicalRule).keys()) {
		if (mIgnoreProperties.contains(propertyName)) {
			continue;
		}

		QString value = mToTemplate.stringProperty(logicalRule, propertyName);
		if (!value.isEmpty()) {
			QRegExp variableRegex("!<(!@.*)?(!#.*)?(!%.*)?!>");  // non-greedy quantifiers
			int index = -1;
			while ((index = variableRegex.indexIn(value)) != -1) { // todo: update offset?
				QString id = variableRegex.capturedTexts()[1];
				if (id.startsWith("!@")) {
					id.remove(0, 2);
				}

				if (id.isEmpty()) {
					id = migrationId;
				}

				QString property = variableRegex.capturedTexts()[2];
				if (property.startsWith("!#")) {
					property.remove(0, 2);
				}

				if (property.isEmpty()) {
					property = propertyName;
				}

				QString captureString = variableRegex.capturedTexts()[3];
				int captureNumber = captureString.startsWith("!%") ? captureString.remove(0, 2).toInt() : 0;

				if (mPropertiesMatches.contains(id) && mPropertiesMatches[id].contains(property)
						&& mPropertiesMatches[id][property].length() > captureNumber) {
					mLogicalModelApi.setPropertyByRoleName(logicalId
							, value.replace(variableRegex, mPropertiesMatches[id][property].at(captureNumber))
							, propertyName);
				} else {  // todo: report broken migration pattern
					mLogicalModelApi.setPropertyByRoleName(logicalId, value.remove(variableRegex), propertyName);
				}
			}
		} else {
			if (mIdToFrom.contains(migrationId)) {
				const Id oldElement = mGraphicalModelApi.logicalId(mCurrentMatch[mIdToFrom[migrationId]]);
				if (mLogicalModelApi.logicalRepoApi().hasProperty(oldElement, propertyName)) {
					mLogicalModelApi.setPropertyByRoleName(logicalId
							, mLogicalModelApi.propertyByRoleName(oldElement, propertyName), propertyName);
				}
			}
		}
	}
}

void GraphTransformation::setNodesGraphicalProperties(const Id &root)
{
	for (const Id &child : mToTemplate.children(root)) {
		if (mToTemplate.from(child) != Id::rootId() || mToTemplate.to(child) != Id::rootId()) {
			continue;
		}

		const QString migrationId = mToTemplate.property(mToTemplate.logicalId(child), "__migrationId__").toString();
		const Id oldElement = mCurrentMatch[mIdToFrom[migrationId]];
		const Id created = mToModel[child];
		mGraphicalModelApi.mutableGraphicalRepoApi().setConfiguration(created, mToTemplate.configuration(child));
		if (mToTemplate.parent(root) == Id::rootId()) {
			if (!migrationId.isEmpty() && !oldElement.isNull()) {
				mGraphicalModelApi.setPosition(created, mGraphicalModelApi.position(oldElement).toPoint());
			} else {
				mGraphicalModelApi.setPosition(created, mToTemplate.position(child).toPoint()); // todo: find better place for new elements
			}
		} else {
			mGraphicalModelApi.setPosition(created, mToTemplate.position(child).toPoint());
		}

		if (!oldElement.isNull()) {
			QPoint offset = mGraphicalModelApi.position(created).toPoint()
					- mGraphicalModelApi.position(oldElement).toPoint();
			for (const Id &link : mGraphicalModelApi.graphicalRepoApi().links(oldElement)) {
				if (oldElement == mGraphicalModelApi.from(link)) {
					mGraphicalModelApi.mutableGraphicalRepoApi().setFrom(link, created);
					QPolygon configuration = mGraphicalModelApi.configuration(link);
					configuration.setPoint(0, configuration.at(0) + offset);
					mGraphicalModelApi.mutableGraphicalRepoApi().setConfiguration(link, configuration);
				}

				if (oldElement == mGraphicalModelApi.to(link)) {
					mGraphicalModelApi.mutableGraphicalRepoApi().setTo(link, created);
					QPolygon configuration = mGraphicalModelApi.configuration(link);
					configuration.setPoint(configuration.length() - 1, configuration.last() + offset);
					mGraphicalModelApi.mutableGraphicalRepoApi().setConfiguration(link, configuration);
				}
			}
		}

		setNodesGraphicalProperties(child);
	}
}

void GraphTransformation::setLinksGraphicalProperties(const Id &root)
{
	for (const Id &child : mToTemplate.children(root)) {
		if (mToTemplate.from(child) == Id::rootId() && mToTemplate.to(child) == Id::rootId()) {
			setLinksGraphicalProperties(child);
			continue;
		}

		const Id created = mToModel[child];
		const Id from = mToModel[mToTemplate.from(child)];
		const Id to = mToModel[mToTemplate.to(child)];

		mGraphicalModelApi.setFrom(created, from);
		mGraphicalModelApi.setTo(created, to);

		mGraphicalModelApi.setConfiguration(created
				, QPolygon() << mGraphicalModelApi.position(from).toPoint() << mGraphicalModelApi.position(to).toPoint());
	}
}

void GraphTransformation::deleteElements()
{
	for (const Id &element : mCurrentMatch.values()) {
		const QString migrationId = mFromTemplate.property(mFromTemplate.logicalId(mCurrentMatch.key(element))
				, "__migrationId__").toString();
		const Id newParent = migrationId.isEmpty()
				? mGraphicalModelApi.graphicalRepoApi().parent(element)
				: mToModel[mIdToTo[migrationId]];

		for (const Id &child : mGraphicalModelApi.children(element)) {
			mGraphicalModelApi.changeParent(child, newParent
					, mGraphicalModelApi.position(element) + mGraphicalModelApi.position(child));
		}

		const Id logicalId = mGraphicalModelApi.logicalId(element);
		mGraphicalModelApi.removeElement(element);
		if (mGraphicalModelApi.graphicalIdsByLogicalId(logicalId).isEmpty()) {
			mLogicalModelApi.removeElement(logicalId);
		}
	}
}

qReal::IdList GraphTransformation::createdElements() const
{
	return mCreatedElements;
}
