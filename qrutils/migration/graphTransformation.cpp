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
	mIgnoreProperties = { "from", "incomingConnections", "incomingUsages", "links", "name", "outgoingConnections"
			, "outgoingUsages", "to", "fromPort", "toPort", "__migrationId__" };
}

void GraphTransformation::apply()
{
	analyzeTemplates();

	while (findMatch()) {
		QHash<Id, Id> match = mMatches[0];

		for (const Id &from : mFromElements) {
			if (!mFromToMap.contains(from)) {
				deleteElement(match[from]);
			}
		}

		for (const Id &to : mToElements) {
			if (!mToFromMap.contains(to)) {
				createElement(to, match);
			}
		}

		for (const Id &from : mFromToMap.keys()) {
			deleteElement(match[from]);
			createElement(mFromToMap[from], match);
			mPropertiesMatches[mToModelMap[mFromToMap[from]]] = mPropertiesMatches[match[from]];
			mPropertiesMatches.remove(match[from]);
		}

		for (const Id &to : mToElements) {
			setProperties(to);
		}
	}
}

void GraphTransformation::analyzeTemplates()
{
	mFromElements.clear();
	mToElements.clear();
	mFromToMap.clear();
	mToFromMap.clear();

	elementsFromTemplate(mFromElements, mFromTemplate, migrationDiagram(mFromTemplate));
	elementsFromTemplate(mToElements, mToTemplate, migrationDiagram(mToTemplate));

	QHash<QString, Id> idMap;
	for (const Id &from : mFromElements) {
		const QString id = mFromTemplate.stringProperty(mFromTemplate.logicalId(from), "__migrationId__");
		if (!id.isEmpty()) {
			idMap[id] = from;
		}
	}

	for (const Id &to : mToElements) {
		const QString id = mToTemplate.stringProperty(mToTemplate.logicalId(to), "__migrationId__");
		if (!id.isEmpty()) {
			const Id from = idMap[id];
			if (!from.isNull()) {
				mFromToMap[from] = to;
				mToFromMap[to] = from;
			}
		}
	}
}

void GraphTransformation::elementsFromTemplate(IdList &elements, qrRepo::GraphicalRepoApi &migrationTemplate
		, const Id &root)
{
	for (const Id &child : migrationTemplate.children(root)) {
		elements << child;
		elementsFromTemplate(elements, migrationTemplate, child);
	}
}

void GraphTransformation::deleteElement(const Id &id)
{

}

void GraphTransformation::createElement(const Id &templateElement, const QHash<Id, Id> &match)
{

}

void GraphTransformation::setProperties(const Id &toElement)
{

}

bool GraphTransformation::findMatch()
{
	mMatches.clear();
	mPropertiesMatches.clear();
	mToModelMap.clear();
	mOldGraphicalProperties.clear();
	mOldLogicalProperties.clear();

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

bool GraphTransformation::compareElements(const Id &first, const Id &second)
{
	if (BaseGraphTransformationUnit::compareElements(first, second)) {
		mOldGraphicalProperties[first] = mGraphicalModelApi.mutableGraphicalRepoApi().properties(first);
		mOldLogicalProperties[first] = mGraphicalModelApi.mutableGraphicalRepoApi()
				.properties(mGraphicalModelApi.logicalId(first));
		return true;
	}

	return false;
}

bool GraphTransformation::compareLinks(const Id &first, const Id &second)
{
	if (BaseGraphTransformationUnit::compareLinks(first, second)) {
		mOldGraphicalProperties[first] = mGraphicalModelApi.mutableGraphicalRepoApi().properties(first);
		mOldLogicalProperties[first] = mGraphicalModelApi.mutableGraphicalRepoApi()
				.properties(mGraphicalModelApi.logicalId(first));
		return true;
	}

	return false;
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
			if (propertyRegExp.indexIn(property(first, key).toString()) == -1) {
				return false;
			}

			// can be true iff search finds only one match
			mPropertiesMatches[first][key]
					<< mLogicalModelApi.logicalRepoApi().stringProperty(mGraphicalModelApi.logicalId(first), key)
					<< propertyRegExp.capturedTexts();
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
