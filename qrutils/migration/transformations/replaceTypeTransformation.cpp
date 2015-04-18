#include "migration/transformations/replaceTypeTransformation.h"

#include "qrrepo/logicalRepoApi.h"
#include "qrrepo/graphicalRepoApi.h"
#include "qrgui/models/graphicalModelAssistApi.h"
#include "qrgui/models/logicalModelAssistApi.h"

using namespace qReal::migration;

ReplaceTypeTransformation::ReplaceTypeTransformation(QString const &oldTypeName, QString const &newTypeName
		, QMap<QString, QString> const &propertyMap)
	: mOldElementType(oldTypeName), mNewElementType(newTypeName), mPropertyMap(propertyMap)
{
}

void ReplaceTypeTransformation::apply(models::ModelsInterface *model, const IdList &ignoredElements)
{
	mModel = model;

	IdList matching = findMatching(ignoredElements);
	foreach (Id const &id, matching) {
		Id const newLogical = createLogical(id);
		foreach (Id const &graphicalId, model->graphicalModelAssistApi().graphicalIdsByLogicalId(id)) {
			createGraphical(newLogical, graphicalId);
		}

		removeOldLogical(id);
	}
}

qReal::IdList ReplaceTypeTransformation::findMatching(const IdList &ignoredElements, qReal::Id const &id) const
{
	IdList result;
	foreach (Id const &child, mModel->logicalModelAssistApi().children(id)) {
		if (!mModel->logicalModelAssistApi().isLogicalId(child)) {
			continue;
		}

		if (!ignoredElements.contains(child) && ((mOldElementType == "@ANY_ELEMENT@")
				|| ((mOldElementType == child.element()) && checkPropertiesMatch(child)))) {
			result << child;
		}

		result.append(findMatching(ignoredElements, child));
	}

	return result;
}

bool ReplaceTypeTransformation::checkPropertiesMatch(qReal::Id const &id) const
{
	foreach (QString const &property, mSearchProperties.keys()) {
		if (property == "@ANY_PROPERTY@") {
			QMapIterator<QString, QVariant> it = mModel->logicalRepoApi().propertiesIterator(id);
			while (it.hasNext()) {
				it.next();
				if (it.value() == mSearchProperties[property]) {
					return true;
				}
			}

			return false;
		}

		if (!mModel->logicalRepoApi().hasProperty(id, property)
				|| (mModel->logicalRepoApi().property(id, property) != mSearchProperties[property])) {
			return false;
		}
	}

	return true;
}

qReal::Id ReplaceTypeTransformation::createLogical(qReal::Id const &oldLogical) const
{
	Id const type = Id(oldLogical.editor(), oldLogical.diagram(), mNewElementType);
	Id const newId = mModel->logicalModelAssistApi().createElement(mModel->logicalRepoApi().parent(oldLogical), type);

	initProperties(oldLogical, newId);
	copyLinks(mModel->mutableLogicalRepoApi(), oldLogical, newId);

	foreach (Id const &child, mModel->logicalRepoApi().children(oldLogical)) {
		mModel->logicalModelAssistApi().changeParent(child, newId);
	}

	return newId;
}

void ReplaceTypeTransformation::createGraphical(qReal::Id const &logical, qReal::Id const &oldGraphical) const
{
	GraphicalModelAssistInterface &graphicalApi = mModel->graphicalModelAssistApi();

	Id const newId = graphicalApi.createElement(mModel->graphicalRepoApi().parent(oldGraphical), logical, true
			, graphicalApi.name(oldGraphical), graphicalApi.position(oldGraphical));

	graphicalApi.setConfiguration(newId, graphicalApi.configuration(oldGraphical));
	graphicalApi.setToPort(newId, graphicalApi.toPort(oldGraphical));
	graphicalApi.setFromPort(newId, graphicalApi.fromPort(oldGraphical));

	copyLinkProperties(oldGraphical, newId);
	copyLinks(mModel->graphicalModelAssistApi().mutableGraphicalRepoApi(), oldGraphical, newId);

	foreach (Id const &child, mModel->graphicalRepoApi().children(oldGraphical)) {
		mModel->graphicalModelAssistApi().changeParent(child, newId, graphicalApi.position(child));
	}
}

void ReplaceTypeTransformation::removeOldLogical(qReal::Id const &id) const
{
	mModel->logicalModelAssistApi().removeElement(id);
}

void ReplaceTypeTransformation::initProperties(qReal::Id const &oldElement, qReal::Id const &newElement) const
{
	qrRepo::LogicalRepoApi &repoApi = mModel->mutableLogicalRepoApi();

	QMapIterator<QString, QVariant> properties = repoApi.propertiesIterator(oldElement);
	while (properties.hasNext()) {
		properties.next();
		if (mPropertyMap.contains(properties.key())) {
			QString const newProperty = mPropertyMap.value(properties.key());
			setProperty(newElement, newProperty, properties.value());
		} else if (!mPropertyMap.values().contains(properties.key())) {
			setProperty(newElement, properties.key(), properties.value());
		}
	}
}

void ReplaceTypeTransformation::setProperty(qReal::Id const &id, QString const &property, QVariant const &value) const
{
	if (property == "to") {
		mModel->mutableLogicalRepoApi().setTo(id, value.value<Id>());
	} else if (property == "from") {
		mModel->mutableLogicalRepoApi().setFrom(id, value.value<Id>());
	} else {
		mModel->mutableLogicalRepoApi().setProperty(id, property, value);
	}
}

void ReplaceTypeTransformation::copyLinkProperties(qReal::Id const &oldId, qReal::Id const &newId) const
{
	GraphicalModelAssistInterface &graphicalApi = mModel->graphicalModelAssistApi();

	if (mModel->logicalRepoApi().hasProperty(oldId, "to")) {
		Id const to = graphicalApi.to(oldId);
		graphicalApi.setTo(oldId, Id::rootId());
		graphicalApi.setTo(newId, to);
	}

	if (mModel->logicalRepoApi().hasProperty(oldId, "from")) {
		Id const from = graphicalApi.from(oldId);
		graphicalApi.setFrom(oldId, Id::rootId());
		graphicalApi.setFrom(newId, from);
	}
}

void ReplaceTypeTransformation::copyLinks(qrRepo::CommonRepoApi &api, qReal::Id const &oldElement, qReal::Id const &newElement) const
{
	foreach (Id const &link, api.links(oldElement)) {
		if (api.to(link) == oldElement) {
			api.setTo(link, newElement);
		}

		if (api.from(link) == oldElement) {
			api.setFrom(link, newElement);
		}
	}
}

QString ReplaceTypeTransformation::newElementType() const
{
	return mNewElementType;
}

void ReplaceTypeTransformation::setNewElementType(const QString &value)
{
	mNewElementType = value;
}

QString ReplaceTypeTransformation::oldElementType() const
{
	return mOldElementType;
}

void ReplaceTypeTransformation::setOldElementType(const QString &value)
{
	mOldElementType = value;
}

