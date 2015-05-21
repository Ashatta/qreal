#include "graphTransformation.h"

#include <qrgui/plugins/pluginManager/editorManagerInterface.h>

#include <QtCore/QRegularExpression>

using namespace qReal::migration;

GraphTransformation::GraphTransformation(LogicalModelAssistInterface &logicalRepoApi
		, GraphicalModelAssistInterface &graphicalRepoApi
		, qrRepo::GraphicalRepoApi &fromTemplate
		, qrRepo::GraphicalRepoApi &toTemplate
		, const IdList &allowedTypes
		, qReal::migration::Migration::Policy policy)
	: BaseGraphTransformationUnit(logicalRepoApi, graphicalRepoApi, nullptr)
	, mFromTemplate(fromTemplate)
	, mToTemplate(toTemplate)
	, mAllowedTypes(allowedTypes)
	, mRepeat(policy == qReal::migration::Migration::fixedPoint)
{
	mIgnoreProperties = { "from", "links", "name", "to", "fromPort", "toPort", "__migrationId__"
			, "outgoingExplosion", "incomingExplosions", "position", "configuration"
			, "childrenOrder", "expanded", "folded", "linkShape", "isView", "__typeName__" };
}

void GraphTransformation::apply()
{
	mCreatedElements.clear();
	mAnyPropertyToReal.clear();
	analyzeTemplates();

	findMatch();
	do {
		resolveOverlaps();
		for (const QHash<Id, Id> &match : mMatches) {
			setMatch(match);
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
	} while (mRepeat && findMatch());
}

void GraphTransformation::analyzeTemplates()
{
	mIdToFrom.clear();
	mIdToTo.clear();

	analyzeTemplate(mIdToFrom, mFromTemplate, mFromRect);
	analyzeTemplate(mIdToTo, mToTemplate, mToRect);
}

void GraphTransformation::analyzeTemplate(QHash<QString, Id> &elements, qrRepo::GraphicalRepoApi &migrationTemplate
		, QRect &bounds)
{
	const Id diagram = migrationDiagram(migrationTemplate);
	elementsFromTemplate(elements, migrationTemplate, migrationDiagram(migrationTemplate));

	for (const Id &child : migrationTemplate.children(diagram)) {
		QRect configuration = migrationTemplate.configuration(child).value<QPolygon>().boundingRect();
		configuration.translate(migrationTemplate.position(child).toPoint());
		if (bounds.isNull()) {
			bounds = configuration;
			continue;
		}

		bounds.setBottom(qMax(bounds.bottom(), configuration.bottom()));
		bounds.setTop(qMin(bounds.top(), configuration.top()));
		bounds.setLeft(qMin(bounds.left(), configuration.left()));
		bounds.setRight(qMax(bounds.right(), configuration.right()));
	}
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
	if (second.element() == "AnyNode") {
		QString typeTemplate = mFromTemplate.property(mFromTemplate.logicalId(second), "TypeName").toString();
		QRegExp typeRegExp ("^" + typeTemplate + "$");
		return !isEdgeInModel(first) && (typeTemplate.isEmpty() || typeRegExp.indexIn(first.element()) != -1)
				&& compareWildCardProperties(first, second);
	} else if (second.element() == "AnyEdge") {
		QString typeTemplate = mFromTemplate.property(mFromTemplate.logicalId(second), "TypeName").toString();
		QRegExp typeRegExp ("^" + typeTemplate + "$");
		return isEdgeInModel(first) && (typeTemplate.isEmpty() || typeRegExp.indexIn(first.element()) != -1);
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

bool GraphTransformation::compareWildCardProperties(const Id &model, const Id &pattern)
{
	QHash<Id, QStringList> propertyMap;
	QHash<Id, QString> assignment;

	for (const Id &property : mFromTemplate.children(pattern)) {
		propertyMap[property] = QStringList();
		const Id logical = mFromTemplate.logicalId(property);

		const QMap<QString, QVariant> properties = mGraphicalModelApi.properties(mGraphicalModelApi.logicalId(model));
		for (const QString &modelProperty : properties.keys()) {
			if (!mFromTemplate.stringProperty(logical, "PropertyName").isEmpty()) {
				QRegExp namePattern("^" + mFromTemplate.stringProperty(logical, "PropertyName") + "$");
				if (namePattern.indexIn(modelProperty) == -1) {
					continue;
				}
			}

			if (!mFromTemplate.stringProperty(logical, "Value").isEmpty()) {
				QRegExp valuePattern("^" + mFromTemplate.stringProperty(logical, "Value") + "$");
				if (valuePattern.indexIn(properties[modelProperty].toString()) == -1) {
					continue;
				}
			}

			propertyMap[property] << modelProperty;
		}
	}

	// todo: fucking assignment problem
	for (const Id &property : propertyMap.keys()) {
		if (propertyMap[property].isEmpty()) {
			return false;
		}

		QString assign = propertyMap[property][0];
		assignment[property] = assign;
		for (const Id &prop : propertyMap.keys()) {
			propertyMap[prop].removeAll(assign);
		}
	}

	mAnyPropertyToReal[model] = assignment;

	return true;
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

void GraphTransformation::resolveOverlaps()
{
	QSet<Id> occupiedElements;
	for (const QHash<Id, Id> &match : mMatches) {
		bool notOverlapping = true;
		for (const Id &element : match.values()) {
			if (occupiedElements.contains(element)) {
				notOverlapping = false;
				mMatches.removeAll(match);
				break;
			}
		}

		if (notOverlapping) {
			occupiedElements.unite(match.values().toSet());
		}
	}
}

void GraphTransformation::setMatch(const QHash<Id, Id> &match)
{
	mCurrentMatch = match;

	mMatchRect = QRect();
	for (const Id &element : match.values()) {
		QRect configuration = mGraphicalModelApi.configuration(element).boundingRect();
		configuration.translate(mGraphicalModelApi.position(element).toPoint());
		if (mMatchRect.isNull()) {
			mMatchRect = configuration;
			continue;
		}

		mMatchRect.setBottom(qMax(mMatchRect.bottom(), configuration.bottom()));
		mMatchRect.setTop(qMin(mMatchRect.top(), configuration.top()));
		mMatchRect.setLeft(qMin(mMatchRect.left(), configuration.left()));
		mMatchRect.setRight(qMax(mMatchRect.right(), configuration.right()));

	}
}

void GraphTransformation::saveProperties()
{
	mPropertiesMatches.clear();

	for (const Id &key : mCurrentMatch.keys()) {
		QString migrationId = mFromTemplate.property(mFromTemplate.logicalId(key), "__migrationId__").toString();
		if (!migrationId.isEmpty()) {
			if (key.element() == "AnyNode" || key.element() == "AnyEdge") {
				addPropertyValue(migrationId
						, "__typeName__"
						, mFromTemplate.property(mFromTemplate.logicalId(key), "TypeName").toString()
						, mCurrentMatch[key].element());

				for (const Id &anyProperty : mFromTemplate.children(key)) {
					Id logicalId = mFromTemplate.logicalId(anyProperty);
					QString propertyId = mFromTemplate.stringProperty(logicalId, "__migrationId__");
					QMap<QString, QString> properties;
					properties["PropertyName"] = mAnyPropertyToReal[mCurrentMatch[key]][anyProperty];
					properties["Value"] = mLogicalModelApi.propertyByRoleName(
							mGraphicalModelApi.logicalId(mCurrentMatch[key])
							, mAnyPropertyToReal[mCurrentMatch[key]][anyProperty]).toString();

					addPropertyValue(migrationId, properties["PropertyName"]
							, mFromTemplate.property(logicalId, "Value").toString(), properties["Value"]);
					if (!propertyId.isEmpty()) {
						for (const QString &propertyName : properties.keys()) {
							addPropertyValue(propertyId, propertyName
									, mFromTemplate.property(logicalId, propertyName).toString()
									, properties[propertyName]);
						}
					}
				}
			} else {
				QMap<QString, QVariant> properties(mFromTemplate.properties(mFromTemplate.logicalId(key)));

				for (const QString &propertyName : properties.keys()) {
					if (mIgnoreProperties.contains(propertyName)) {
						continue;
					}

					addPropertyValue(migrationId, propertyName, properties[propertyName].toString()
							, property(mCurrentMatch[key], propertyName).toString());
				}

				mPropertiesMatches[migrationId]["__typeName__"] << key.element();
			}
		}
	}
}

void GraphTransformation::addPropertyValue(const QString &migrationId, const QString &propertyName
		, const QString &regExp, const QString &value)
{
	QRegExp propertyRegExp("^" + regExp + "$");
	propertyRegExp.indexIn(value);
	mPropertiesMatches[migrationId][propertyName] << propertyRegExp.capturedTexts();
	mPropertiesMatches[migrationId][propertyName][0] = value;
}

void GraphTransformation::createNodes(const Id &root, const Id &createdRoot)
{
	for (const Id &child : mToTemplate.children(root)) {
		if (mToTemplate.from(child) == Id::rootId() && mToTemplate.to(child) == Id::rootId()) {
			if (child.element() == "AnyNode") {
				createWildCard(child, createdRoot);
				setWildCardProperties(child);
			} else {
				const Id created = createElement(createdRoot, Id(createdRoot.editor(), child.diagram(), child.element()));
				createNodes(child, created);
				setLogicalProperties(created, child); // todo: names
				mToModel[child] = created;
			}
		}
	}
}

void GraphTransformation::createLinks(const Id &root, const Id &diagram)
{
	for (const Id &child : mToTemplate.children(root)) {
		if (mToTemplate.from(child) != Id::rootId() || mToTemplate.to(child) != Id::rootId()) {
			if (child.element() == "AnyEdge") {
				createWildCard(child, diagram);
			} else {
				const Id created = createElement(diagram, Id(diagram.editor(), child.diagram(), child.element()));
				setLogicalProperties(created, child);
				mToModel[child] = created;
			}
		} else {
			createLinks(child, diagram);
		}
	}
}


void GraphTransformation::createWildCard(const Id &pattern, const Id &createdRoot)
{
	Id logicalId = mToTemplate.logicalId(pattern);
	QString migrationId = mToTemplate.stringProperty(logicalId, "__migrationId__");
	Id created;
	if (migrationId.isEmpty()) {
		QString typeName = getNewValue("", "", mToTemplate.stringProperty(logicalId, "TypeName"));
		for (const Id &type : mAllowedTypes) {
			if (type.element() == typeName) {
				created = createElement(createdRoot, type);
				break;
			}
		}

		if (created == Id()) {
			return;
		}
	} else {
		created = createElement(createdRoot, mCurrentMatch[mIdToFrom[migrationId]].type());
	}

	mToModel[pattern] = created;
}

qReal::Id GraphTransformation::createElement(const Id &parent, const Id &type)
{
	int oldVersion = mLogicalModelApi.logicalRepoApi().metamodelVersion(parent.editor());
	Id created = mGraphicalModelApi.createElement(parent, type);
	mLogicalModelApi.mutableLogicalRepoApi().addUsedMetamodel(created.editor(), oldVersion);
	mCreatedElements << created << mGraphicalModelApi.logicalId(created);
	return created;
}

void GraphTransformation::setWildCardProperties(const Id &pattern)
{
	Id created = mToModel[pattern];
	if (created == Id()) {
		return;
	}

	QSet<QString> setProperties;
	QString migrationId = mToTemplate.stringProperty(mToTemplate.logicalId(pattern), "__migrationId__");
	for (const Id &anyProperty : mToTemplate.children(pattern)) {
		Id propertyLogical = mToTemplate.logicalId(anyProperty);
		QString propertyId = mToTemplate.stringProperty(propertyLogical, "__migrationId__");
		QString nameTemplate = mToTemplate.stringProperty(propertyLogical, "PropertyName");
		QString valueTemplate = mToTemplate.stringProperty(propertyLogical, "Value");
		QString name = propertyId.isEmpty()
				? getNewValue(migrationId, "name", nameTemplate)
				: getNewValue(propertyId, "PropertyName", nameTemplate);
		QString value = migrationId.isEmpty()
				? getNewValue(propertyId, "Value", valueTemplate)
				: getNewValue(migrationId, name, valueTemplate);
		mLogicalModelApi.setPropertyByRoleName(mGraphicalModelApi.logicalId(created), value, name);
		setProperties << name;
	}

	const Id oldElement = mGraphicalModelApi.logicalId(mCurrentMatch[mIdToFrom[migrationId]]);
	const Id logicalCreated = mGraphicalModelApi.logicalId(created);
	QMap<QString, QVariant> properties = mGraphicalModelApi.properties(logicalCreated);
	for (const QString &key : properties.keys()) {
		if (mIgnoreProperties.contains(key) || setProperties.contains(key)) {
			continue;
		}

		if (mLogicalModelApi.logicalRepoApi().hasProperty(oldElement, key)) {
			const QVariant oldValue = mLogicalModelApi.propertyByRoleName(oldElement, key);
			mLogicalModelApi.setPropertyByRoleName(logicalCreated, oldValue, key);
		}
	}

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

		QString valueTemplate = mToTemplate.stringProperty(logicalRule, propertyName);
		mLogicalModelApi.setPropertyByRoleName(logicalId, getNewValue(migrationId, propertyName, valueTemplate)
				, propertyName);
	}
}

QString GraphTransformation::getNewValue(const QString &migrationId, const QString &propertyName
		, QString valueTemplate)
{
	if (valueTemplate.isEmpty()) {
		const Id pattern = mIdToFrom[migrationId];
		if (pattern.element() == "AnyProperty") {
			const Id oldElement = mCurrentMatch[mFromTemplate.parent(pattern)];
			const QString name = mAnyPropertyToReal[oldElement][pattern];
			if (propertyName == "PropertyName") {
				return name;
			} else if (propertyName == "Value") {
				return mLogicalModelApi.propertyByRoleName(mGraphicalModelApi.logicalId(oldElement), name).toString();
			}
		} else {
			const Id oldElement = mGraphicalModelApi.logicalId(mCurrentMatch[pattern]);
			return mLogicalModelApi.propertyByRoleName(oldElement, propertyName).toString();
		}
	}

	QRegularExpression variableRegex("!<(!@.*?)?(!#.*?)?(!%.*?)?!>");
	while (true) {
		QRegularExpressionMatch match = variableRegex.match(valueTemplate);
		if (!match.hasMatch()) {
			break;
		}

		QString id = match.capturedTexts()[1];
		if (id.startsWith("!@")) {
			id.remove(0, 2);
		}

		if (id.isEmpty()) {
			id = migrationId;
		}

		QString property = match.capturedTexts()[2];
		if (property.startsWith("!#")) {
			property.remove(0, 2);
		}

		if (property.isEmpty()) {
			property = propertyName;
		}

		QString captureString = match.capturedTexts()[3];
		int captureNumber = captureString.startsWith("!%") ? captureString.remove(0, 2).toInt() : 0;
		valueTemplate.replace(match.capturedStart(), match.capturedLength(), mPropertiesMatches[id][property].at(captureNumber));
	}

	return valueTemplate;
}

void GraphTransformation::setNodesGraphicalProperties(const Id &root)
{
	for (const Id &child : mToTemplate.children(root)) {
		if (mToTemplate.from(child) != Id::rootId() || mToTemplate.to(child) != Id::rootId()) {
			continue;
		}

		const QString migrationId = mToTemplate.property(mToTemplate.logicalId(child), "__migrationId__").toString();
		const Id oldElement = mCurrentMatch.value(mIdToFrom[migrationId], Id());
		const Id created = mToModel[child];
		mGraphicalModelApi.mutableGraphicalRepoApi().setConfiguration(created, mToTemplate.configuration(child));
		if (mToTemplate.parent(root) == Id::rootId()) {
			if (!migrationId.isEmpty() && !oldElement.isNull()) {
				mGraphicalModelApi.setPosition(created, mGraphicalModelApi.position(oldElement).toPoint());
			} else {
				QPoint templateOffset = mToTemplate.position(child).toPoint() - mToRect.center();
				const qreal widthRatio = mMatchRect.width() / mToRect.width();
				const qreal heightRatio = mMatchRect.height() / mToRect.height();
				templateOffset.setX(templateOffset.x() * widthRatio);
				templateOffset.setY(templateOffset.y() * heightRatio);
				mGraphicalModelApi.setPosition(created, mMatchRect.center() + templateOffset);
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
					mLogicalModelApi.setFrom(mGraphicalModelApi.logicalId(link), mGraphicalModelApi.logicalId(created));
					QPolygon configuration = mGraphicalModelApi.configuration(link);
					configuration.setPoint(0, configuration.at(0) + offset);
					mGraphicalModelApi.mutableGraphicalRepoApi().setConfiguration(link, configuration);
				}

				if (oldElement == mGraphicalModelApi.to(link)) {
					mGraphicalModelApi.mutableGraphicalRepoApi().setTo(link, created);
					mLogicalModelApi.setTo(mGraphicalModelApi.logicalId(link), mGraphicalModelApi.logicalId(created));
					QPolygon configuration = mGraphicalModelApi.configuration(link);
					configuration.setPoint(configuration.length() - 1, configuration.last() + offset);
					mGraphicalModelApi.mutableGraphicalRepoApi().setConfiguration(link, configuration);
				}
			}
		}

		if (child.element() != "AnyNode") {
			setNodesGraphicalProperties(child);
		}
	}
}

void GraphTransformation::setLinksGraphicalProperties(const Id &root)
{
	for (const Id &child : mToTemplate.children(root)) {
		if (mToTemplate.from(child) == Id::rootId() && mToTemplate.to(child) == Id::rootId()) {
			if (child.element() != "AnyNode") {
				setLinksGraphicalProperties(child);
			}

			continue;
		}

		const Id created = mToModel[child];
		const Id from = mToModel[mToTemplate.from(child)];
		const Id to = mToModel[mToTemplate.to(child)];

		mGraphicalModelApi.setFrom(created, from);
		mGraphicalModelApi.setTo(created, to);

		mLogicalModelApi.setFrom(mGraphicalModelApi.logicalId(created), mGraphicalModelApi.logicalId(from));
		mLogicalModelApi.setTo(mGraphicalModelApi.logicalId(created), mGraphicalModelApi.logicalId(to));

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
