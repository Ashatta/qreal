#include "xmlParser.h"

#include <QtCore/QDebug>
#include <QtCore/QUuid>
#include <QtCore/QPointF>
#include <QtCore/QProcess>
#include <QtWidgets/QMessageBox>
#include <QtGui/QPolygonF>
#include <QtXml/QDomDocument>

#include "math.h"

#include "../../../qrrepo/repoApi.h"
#include "../../../qrutils/xmlUtils.h"
#include "../../../qrkernel/exception/exception.h"

using namespace qReal;
using namespace metaEditor;

XmlParser::XmlParser(qrRepo::LogicalRepoApi &logicalApi, qrRepo::GraphicalRepoApi &graphicalApi)
		: mLogicalApi(logicalApi)
		, mGraphicalApi(graphicalApi)
		, mElementsColumn(0)
		, mElementCurrentColumn(0)
		, mMoveWidth(180)
		, mMoveHeight(100)
		, mCurrentWidth(0)
		, mCurrentHeight(0)
		, mParentPositionX(280)
{
	mLogicalToGraphical[Id::rootId()] = Id::rootId();
}

void XmlParser::parseFile(const QString &fileName)
{
	QFileInfo directoryName(fileName);

	// search for the directory, where xml-file is
	QDir fileDir = directoryName.absoluteDir();
	QString filePathName = fileDir.absolutePath();
	QString fileBaseName = directoryName.baseName();

	QString fileProFullName = filePathName + "/" + fileBaseName + ".pro";
	QFile file(fileProFullName);
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
		return;

	QString pathToQRealSourceFiles;
	QTextStream in(&file);
	while (!in.atEnd()) {
		QString line = in.readLine();
		if (line.contains("ROOT")) {
			pathToQRealSourceFiles = line.mid(line.indexOf("=") + 1);
			pathToQRealSourceFiles = pathToQRealSourceFiles.trimmed();
		}
	}
	file.close();

	if (containsName(fileBaseName))
		return;
	QDomDocument const doc = utils::xmlUtils::loadDocument(fileName);

	initMetamodel(doc, filePathName, fileBaseName, pathToQRealSourceFiles);

	QDomNodeList const listeners = doc.elementsByTagName("listener");
	int listenerPositionY = 100;
	for (int i = 0; i < listeners.length(); ++i) {
		QDomElement listener = listeners.at(i).toElement();
		Id id = initListener("(Listener)", listener.attribute("class", ""), listener.attribute("file", ""));
		mLogicalApi.setProperty(id, "position", QPointF(0,listenerPositionY));
		listenerPositionY += 90;
	}

	QDomNodeList const diagrams = doc.elementsByTagName("diagram");

	mElementsColumn = ceil(sqrt(static_cast<qreal>(diagrams.length())));

	for (int i = 0; i < diagrams.length(); ++i) {
		QDomElement diagram = diagrams.at(i).toElement();
		initDiagram(diagram, mMetamodel, diagram.attribute("name", "Unknown Diagram"),
				diagram.attribute("displayedName", "Unknown Diagram"));
	}

	initGraphicalApi();
	initEdgesConfiguration();

	clear();
}

void XmlParser::clear()
{
	mElementsColumn = 0;
	mElementCurrentColumn = 0;
	mMoveWidth = 180;
	mMoveHeight = 100;
	mCurrentWidth = 0;
	mCurrentHeight = 0;
	mParentPositionX = 280;
	mElements.clear();
	mParents.clear();
	mContainers.clear();
	mLogicalToGraphical.clear();
	mLogicalToGraphical[Id::rootId()] = Id::rootId();
}

QStringList XmlParser::getIncludeList(const QString &fileName)
{
	QDomDocument const doc = utils::xmlUtils::loadDocument(fileName);

	QDomNodeList const includeList = doc.elementsByTagName("include");
	QStringList includeFilesList;
	for (int i = 0; i < includeList.length(); ++i) {
		QDomElement include = includeList.at(i).toElement();
		QFileInfo info(fileName);
		QFileInfo name(include.text());
		if (!containsName(name.baseName())) {
			includeFilesList.append(getIncludeList(info.absoluteDir().path() + "/" + include.text()));
			includeFilesList.append(info.absoluteDir().path() + "/" + include.text());
		}
	}
	return includeFilesList;
}

void XmlParser::loadIncludeList(const QString &fileName)
{
	QStringList includeList = getIncludeList(fileName);
	if (includeList.isEmpty())
		return;
	if (QMessageBox::question(NULL, QObject::tr("loading.."),"Do you want to load connected metamodels?", QMessageBox::Yes, QMessageBox::No) == QMessageBox::Yes) {
		foreach (QString const &include, includeList) {
			if (!containsName(include))
				parseFile(include);
		}
	}
}

bool XmlParser::containsName(const QString &name)
{
	IdList idList = mLogicalApi.children(Id::rootId());
	foreach (Id const &id, idList) {
		if (mLogicalApi.name(id) == name)
			return true;
	}
	return false;
}

Id XmlParser::getPackageId()
{
	IdList const children = mLogicalApi.children(Id::rootId());
	foreach (Id id, children) {
		if (id.element() == "PackageDiagram")
			return id;
	}
	Id const packageId("MetaEditor", "MetaEditor", "PackageDiagram",
					   QUuid::createUuid().toString());
	setStandartConfigurations(packageId, Id::rootId(), "Package", "Package");
	return packageId;
}

void XmlParser::initMetamodel(const QDomDocument &document, const QString &directoryName, const QString &baseName, const QString &pathToRoot)
{
	QDomNodeList const includeList = document.elementsByTagName("include");
	QString includeListString = "";
	for (int i = 0; i < includeList.length(); ++i) {
		QDomElement include = includeList.at(i).toElement();
		includeListString += include.text() + ", ";
	}

	mMetamodel = Id("MetaEditor", "MetaEditor", "MetamodelDiagram",
					QUuid::createUuid().toString());
	setStandartConfigurations(mMetamodel, Id::rootId(), baseName, "");
	mLogicalApi.setProperty(mMetamodel, "version", document.documentElement().attribute("version"));
	mLogicalApi.setProperty(mMetamodel, "include", includeListString);
	mLogicalApi.setProperty(mMetamodel, "name of the directory", directoryName);
	mLogicalApi.setProperty(mMetamodel, "path to QReal Source Files", pathToRoot);
}

Id XmlParser::initListener(const QString &name, const QString &className, const QString &fileName)
{
	Id listenerId("MetaEditor", "MetaEditor", "Listener",
			QUuid::createUuid().toString());
	setStandartConfigurations(listenerId, mMetamodel, name, name);
	mLogicalApi.setProperty(listenerId, "class", className);
	mLogicalApi.setProperty(listenerId, "file", fileName);
	return listenerId;
}

void XmlParser::initDiagram(const QDomElement &diagram, const Id &parent,
		const QString &name, const QString &displayedName)
{
	Id diagramId("MetaEditor", "MetaEditor", "MetaEditorDiagramNode",
			QUuid::createUuid().toString());

	setStandartConfigurations(diagramId, parent, name, displayedName);
	mLogicalApi.setProperty(diagramId, "nodeName", diagram.attribute("nodeName", ""));

	createDiagramAttributes(diagram, diagramId);

	setElementPosition(diagramId);
}

void XmlParser::createDiagramAttributes(const QDomElement &diagram, const Id &diagramId)
{
	QDomNodeList diagramAttributes = diagram.childNodes();

	for (int i = 0; i < diagramAttributes.length(); ++i) {
		QDomElement type = diagramAttributes.at(i).toElement();
		if (type.tagName() == "nonGraphicTypes")
			createNonGraphicElements(type, diagramId);
		if (type.tagName() == "graphicTypes")
			createGraphicElements(type, diagramId);
	}

	initGroupNodesTypes(diagramId);

	QString const diagramName = mLogicalApi.name(diagramId);
	initGeneralization(diagramName);
	initContainer(diagramName);
	initExplosion(diagramName);
}

void XmlParser::createNonGraphicElements(const QDomElement &type, const Id &diagramId)
{
	QDomNodeList enums = type.childNodes();

	for (int i = 0; i < enums.length(); ++i) {
		QDomElement element = enums.at(i).toElement();
		if (element.tagName() == "enum") {
			initEnum(element, diagramId);
		} else if (element.tagName() == "groups") {
			QDomNodeList groups = element.childNodes();
			for (int j = 0; j < groups.length(); ++j) {
				initGroup(groups.at(j).toElement(), diagramId);
			}
		}
	}
}

void XmlParser::createGraphicElements(const QDomElement &type, const Id &diagramId)
{
	QDomNodeList graphicElements = type.childNodes();

	for (int i = 0; i < graphicElements.length(); ++i) {
		QDomElement graphicElement = graphicElements.at(i).toElement();
		if (graphicElement.tagName() == "node")
			initNode(graphicElement, diagramId);
		if (graphicElement.tagName() == "edge")
			initEdge(graphicElement, diagramId);
		if (graphicElement.tagName() == "import")
			initImport(graphicElement, diagramId);
	}
}

void XmlParser::initEnum(const QDomElement &enumElement, const Id &diagramId)
{
	Id enumId("MetaEditor", "MetaEditor", "MetaEntityEnum",
			QUuid::createUuid().toString());

	setStandartConfigurations(enumId, diagramId, enumElement.attribute("name", ""),
			enumElement.attribute("displayedName", ""));
	mLogicalApi.setProperty(enumId, "editable", enumElement.attribute("editable", "false"));

	setEnumAttributes(enumElement, enumId);
}

void XmlParser::initGroup(const QDomElement &group, const Id &diagramId)
{
	Id groupId("MetaEditor", "MetaEditor", "MetaEntityGroup"
			, QUuid::createUuid().toString());

	QString name = group.attribute("name", "");
	setStandartConfigurations(groupId, diagramId, name, "");
	mElements.insert(name, groupId);
	mLogicalApi.setProperty(groupId, "rootNode", group.attribute("rootNode"));

	QDomNodeList groupNodes = group.childNodes();
	for (int i = 0; i < groupNodes.length(); ++i) {
		initGroupNode(groupNodes.at(i).toElement(), groupId);
	}
}

void XmlParser::initGroupNode(const QDomElement &groupNode, const Id &groupId)
{
	Id groupNodeId("MetaEditor", "MetaEditor", "MetaEntityGroupNode"
			, QUuid::createUuid().toString());

	setStandartConfigurations(groupNodeId, groupId, groupNode.attribute("name", ""), "");

	mLogicalApi.setProperty(groupNodeId, "xPosition", groupNode.attribute("xPosition", "0").toInt());
	mLogicalApi.setProperty(groupNodeId, "yPosition", groupNode.attribute("yPosition", "0").toInt());
	mLogicalApi.setProperty(groupNodeId, "parent", groupNode.attribute("parent", ""));
	mGroupNodesTypes[groupNodeId] = groupNode.attribute("type");
}

void XmlParser::initNode(const QDomElement &node, const Id &diagramId)
{
	QString const nodeName = node.attribute("name", "");

	if (!(mElements.contains(nodeName))) {
		Id nodeId("MetaEditor", "MetaEditor", "MetaEntityNode",
				QUuid::createUuid().toString());
		mElements.insert(nodeName, nodeId);

		setStandartConfigurations(nodeId, diagramId, nodeName, node.attribute("displayedName", ""));
		mLogicalApi.setProperty(nodeId, "path", node.attribute("path", ""));
		mLogicalApi.setProperty(nodeId, "description", node.attribute("description", ""));
		mLogicalApi.setProperty(nodeId, "abstract", node.attribute("abstract", "false"));

		setNodeAttributes(node, nodeId);
	}
}

void XmlParser::initEdge(const QDomElement &edge, const Id &diagramId)
{
	QString const edgeName = edge.attribute("name", "");

	if (!(mElements.contains(edgeName))) {
		Id edgeId("MetaEditor", "MetaEditor", "MetaEntityEdge",
				QUuid::createUuid().toString());
		mElements.insert(edgeName, edgeId);

		setStandartConfigurations(edgeId, diagramId, edgeName, edge.attribute("displayedName", ""));
		mLogicalApi.setProperty(edgeId, "path", edge.attribute("path", ""));
		mLogicalApi.setProperty(edgeId, "description", edge.attribute("description", ""));

		setEdgeAttributes(edge, edgeId);
	}
}

void XmlParser::initImport(const QDomElement &import, const Id &diagramId)
{
	QString const importName = import.attribute("name", "");

	if (!(mElements.contains(importName))) {
		Id importId("MetaEditor", "MetaEditor", "MetaEntityImport",
				QUuid::createUuid().toString());
		QStringList nameList = import.attribute("name", "").split("::", QString::SkipEmptyParts);
		setStandartConfigurations(importId, diagramId, nameList.at(1),
				import.attribute("displayedName", ""));
		mLogicalApi.setProperty(importId, "as", import.attribute("as", ""));
		mLogicalApi.setProperty(importId, "importedFrom", nameList.at(0));
		mElements.insert(importName, importId);
	}
}

void XmlParser::setEnumAttributes(QDomElement const &enumElement, Id const &enumId)
{
	QDomNodeList const values = enumElement.childNodes();

	for (int i = 0; i < values.length(); ++i) {
		QDomElement value = values.at(i).toElement();
		if (value.tagName() == "value"){
			Id const valueId("MetaEditor", "MetaEditor", "MetaEntityValue"
					, QUuid::createUuid().toString());

			setStandartConfigurations(valueId, enumId, value.attribute("name", ""),
					value.attribute("displayedName", ""));

			mLogicalApi.setProperty(valueId, "valueName", value.attribute("name", ""));
		}
	}
}

void XmlParser::setNodeAttributes(const QDomElement &node, const Id &nodeId)
{
	QDomNodeList nodeList = node.childNodes();

	for (int i = 0; i < nodeList.length(); ++i) {
		QDomElement tag = nodeList.at(i).toElement();
		if (tag.tagName() == "logic")
			setNodeConfigurations(tag, nodeId);
		if (tag.tagName() == "graphics") {
			QDomDocument document;
			document.createElement("document");
			QDomNode nodeCopy = nodeList.at(i).cloneNode();
			document.importNode(nodeList.at(i), true);
			document.appendChild(nodeCopy);
			mLogicalApi.setProperty(nodeId, "shape", document.toString());
		}
	}
}

void XmlParser::setEdgeAttributes(const QDomElement &edge, const Id &edgeId)
{
	QDomNodeList edgeList = edge.childNodes();

	for (int i = 0; i < edgeList.length(); ++i) {
		QDomElement tag = edgeList.at(i).toElement();
		if (tag.tagName() == "graphics")
			setLineType(tag, edgeId);
		if (tag.tagName() == "logic")
			setEdgeConfigurations(tag, edgeId);
	}
}

void XmlParser::setNodeConfigurations(const QDomElement &tag, const Id &nodeId)
{
	QDomNodeList nodeAttributes = tag.childNodes();

	for (int i = 0; i < nodeAttributes.length(); ++i) {
		QDomElement attribute = nodeAttributes.at(i).toElement();
		if (attribute.tagName() == "generalizations") {
			setGeneralization(attribute, nodeId);
		} else if (attribute.tagName() == "properties") {
			setProperties(attribute, nodeId);
		} else if (attribute.tagName() == "container") {
			setContainers(attribute, nodeId);
		} else if (attribute.tagName() == "action") {
			setAction(nodeId);
		} else if (attribute.tagName() == "bonusContextMenuFields") {
			setFields(attribute, nodeId);
		} else if (attribute.tagName() == "explodesTo") {
			setExplosion(attribute, nodeId);
		}
	}
}

void XmlParser::setLineType(const QDomElement &tag, const Id &edgeId)
{
	QDomNodeList graphics = tag.childNodes();

	if (graphics.length() > 0) {
		QDomElement lineType = graphics.at(0).toElement();
		mLogicalApi.setProperty(edgeId, "lineType", lineType.attribute("type", ""));
	}

	if (graphics.length() > 1) {
		QDomElement shape = graphics.at(1).toElement();
		mLogicalApi.setProperty(edgeId, "shape", shape.attribute("type"));
	}

	// quick workaround for #349, just saving a part of XML into `labels' property
	// TODO: make it somehow more elegant
	for(int i = 0; i < graphics.length(); ++i){
		QDomElement element = graphics.at(i).toElement();
		if (element.tagName() == "labels") {
			QDomNodeList labelNodes = element.childNodes();
			for (int j = 0; j < labelNodes.length(); ++j) {
				QDomElement label = labelNodes.at(i).toElement();
				if (label.hasAttribute("text")) {
					mLogicalApi.setProperty(edgeId, "labelText", label.attribute("text"));
					mLogicalApi.setProperty(edgeId, "labelType", "staticText");
				} else {
					mLogicalApi.setProperty(edgeId, "labelText", label.attribute("textBinded"));
					mLogicalApi.setProperty(edgeId, "labelType", "dynamicText");
				}

				bool hardLabel = label.attribute("hard", "true") == "true";
				mLogicalApi.setProperty(edgeId, "hardLabel", hardLabel);
			}
		}
	}

}

void XmlParser::setEdgeConfigurations(const QDomElement &tag, const Id &edgeId)
{
	QDomNodeList edgeAttributes = tag.childNodes();

	for (int i = 0; i < edgeAttributes.length(); ++i) {
		QDomElement attribute = edgeAttributes.at(i).toElement();
		if (attribute.tagName() == "generalizations") {
			setGeneralization(attribute, edgeId);
		} else if (attribute.tagName() == "properties") {
			setProperties(attribute, edgeId);
		} else if (attribute.tagName() == "associations") {
			setAssociations(attribute, edgeId);
		} else if (attribute.tagName() == "possibleEdges") {
			setPossibleEdges(attribute, edgeId);
		} else if (attribute.tagName() == "dividability") {
			mLogicalApi.setProperty(edgeId, "isDividable", attribute.attribute("isDividable", "true"));
		}
	}
}

void XmlParser::setGeneralization(const QDomElement &element, const Id &elementId)
{
	QDomNodeList generalizations = element.childNodes();
	QStringList parents;

	for (int i = 0; i < generalizations.length(); ++i) {
		QDomElement generalization = generalizations.at(i).toElement();
		if (generalization.tagName() == "parent")
			parents.insert(0, generalization.attribute("parentName", ""));
		mParents.insert(elementId, parents);
	}
}

void XmlParser::setProperties(const QDomElement &element, const Id &elementId)
{
	QDomNodeList properties = element.childNodes();

	for (int i = 0; i < properties.length(); ++i) {
		QDomElement property = properties.at(i).toElement();
		if (property.tagName() == "property")
			initProperty(property, elementId);
	}
}

void XmlParser::setFields(const QDomElement &element, const Id &elementId)
{
	QDomNodeList fields = element.childNodes();

	for (int i = 0; i < fields.length(); ++i) {
		QDomElement field = fields.at(i).toElement();
		if (field.tagName() == "field") {
			Id fieldId("MetaEditor", "MetaEditor", "MetaEntityContextMenuField", QUuid::createUuid().toString());
			setStandartConfigurations(fieldId, elementId, field.attribute("name", ""),
					field.attribute("displayedName", ""));
		}
	}
}

void XmlParser::setContainers(const QDomElement &element, const Id &elementId)
{
	QDomNodeList containsElements = element.childNodes();
	QStringList containers;
	for (int i = 0; i < containsElements.length(); ++i) {
		QDomElement contains = containsElements.at(i).toElement();
		if (contains.tagName() == "contains") {
			QString type = contains.attribute("type", "");
			containers.insert(0, type);
			/*QString existingContainers;
				if (mApi.hasProperty(elementId, "container"))
				existingContainers = mApi.stringProperty(elementId, "container");
				if (!existingContainers.isEmpty())
					existingContainers += ",";
				existingContainers += type;

				mApi.setProperty(elementId, "container", existingContainers);*/
		}
		if (contains.tagName() == "properties")
			setContainerProperties(contains, elementId);
	}
	mContainers.insert(elementId, containers);
}

void XmlParser::setContainerProperties(const QDomElement &element, const Id &elementId)
{
	QDomNodeList properties = element.childNodes();
	if (properties.size() > 0) {
		Id containerProperties("MetaEditor", "MetaEditor",
				"MetaEntityPropertiesAsContainer", QUuid::createUuid().toString());
		setStandartConfigurations(containerProperties, elementId, "properties", "");
		for (int i = 0; i < properties.length(); ++i) {
			QDomElement property = properties.at(i).toElement();
			setBoolValuesForContainer("sortContainer", property, containerProperties);
			setBoolValuesForContainer("minimizeToChildren", property, containerProperties);
			setBoolValuesForContainer("maximizeChildren", property, containerProperties);
			setBoolValuesForContainer("banChildrenMove", property, containerProperties);

			setSizesForContainer("forestalling", property, containerProperties);
			setSizesForContainer("childrenForestalling", property, containerProperties);
		}
	}
}

void XmlParser::setExplosion(QDomElement const &element, Id const &elementId)
{
	QDomElement target = element.childNodes().at(0).toElement();

	Id explosion("MetaEditor", "MetaEditor", "Explosion", QUuid::createUuid().toString());

	QString name = mLogicalApi.name(elementId) + "_ExplodesTo_" + target.attribute("type");
	setStandartConfigurations(explosion, mMetamodel, name, "");
	mLogicalApi.setProperty(explosion, "makeReusable", target.attribute("makeReusable"));
	mLogicalApi.setProperty(explosion, "requireImmediateLinkage", target.attribute("requireImmediateLinkage"));
	mLogicalApi.setTo(explosion, elementId);

	mExplosions[explosion] = target.attribute("type");
}

void XmlParser::setBoolValuesForContainer(const QString &tagName, const QDomElement &property, const Id &id)
{
	if (property.tagName() == tagName)
		mLogicalApi.setProperty(id, tagName, "true");
}

void XmlParser::setSizesForContainer(const QString &tagName, const QDomElement &property, const Id &id)
{
	if (property.tagName() == tagName)
		mLogicalApi.setProperty(id, tagName + "Size", property.attribute("size", "0"));
}

void XmlParser::setAssociations(const QDomElement &element, const Id &elementId)
{
	Id associationId("MetaEditor", "MetaEditor", "MetaEntityAssociation",
			QUuid::createUuid().toString());
	QDomNodeList associations = element.childNodes();

	QDomElement association = associations.at(0).toElement();

	setStandartConfigurations(associationId, elementId, association.attribute("name", ""),
			association.attribute("displayedName", ""));

	mLogicalApi.setProperty(associationId, "beginType", element.attribute("beginType", ""));
	mLogicalApi.setProperty(associationId, "endType", element.attribute("endType", ""));
	mLogicalApi.setProperty(associationId, "beginName", association.attribute("beginName", ""));
	mLogicalApi.setProperty(associationId, "endName", association.attribute("endName", ""));
}

void XmlParser::setPossibleEdges(const QDomElement &element, const Id &elementId)
{
	QDomNodeList possibleEdges = element.childNodes();

	for (int i = 0; i < possibleEdges.length(); ++i) {
		QDomElement possibleEdge = possibleEdges.at(i).toElement();
		if (possibleEdge.tagName() == "possibleEdge")
			initPossibleEdge(possibleEdge, elementId);
	}
}

void XmlParser::setAction(const Id &elementId)
{
	mLogicalApi.setProperty(elementId, "isAction", "true");
}

void XmlParser::initPossibleEdge(const QDomElement &possibleEdge, const Id &elementId)
{
	Id possibleEdgeId("MetaEditor", "MetaEditor", "MetaEntityPossibleEdge",
			QUuid::createUuid().toString());

	setStandartConfigurations(possibleEdgeId, elementId, possibleEdge.attribute("name", ""),
			possibleEdge.attribute("displayedName", ""));

	mLogicalApi.setProperty(possibleEdgeId, "beginName", possibleEdge.attribute("beginName", ""));
	mLogicalApi.setProperty(possibleEdgeId, "endName", possibleEdge.attribute("endName", ""));
	mLogicalApi.setProperty(possibleEdgeId, "directed", possibleEdge.attribute("directed", "false"));
}

void XmlParser::initProperty(const QDomElement &property, const Id &elementId)
{
	Id propertyId("MetaEditor", "MetaEditor", "MetaEntity_Attribute",
			QUuid::createUuid().toString());

	setStandartConfigurations(propertyId, elementId, property.attribute("name", ""),
			property.attribute("displayedName", ""));

	mLogicalApi.setProperty(propertyId, "attributeType", property.attribute("type", "0"));

	QDomNodeList defaultValue = property.childNodes();
	if (!defaultValue.isEmpty())
		mLogicalApi.setProperty(propertyId, "defaultValue",
				defaultValue.at(0).toElement().text());
}

void XmlParser::initGeneralization(const QString &diagramName)
{
	foreach (Id const id, mParents.keys()) {
		setParents(id, diagramName);
	}
}

void XmlParser::initContainer(const QString &diagramName)
{
	foreach (Id const id, mContainers.keys()) {
		setContains(id, diagramName);
	}
}

void XmlParser::initExplosion(QString const &diagramName)
{
	for (Id const &id : mExplosions.keys()) {
		setExplodes(id, diagramName);
	}
}

void XmlParser::initGroupNodesTypes(qReal::Id const &diagramId)
{
	for (Id const &id : mGroupNodesTypes.keys()) {
		for (Id const &child : mLogicalApi.children(diagramId)) {
			if (mGroupNodesTypes[id] == mLogicalApi.name(child)) {
				mLogicalApi.setProperty(id, "type", child.toString());
				break;
			}
		}
	}
}

void XmlParser::setParents(const Id &id, const QString &diagramName)
{
	IdList parents;
	foreach (QString const elementName, mParents[id]) {
		QStringList name = elementName.split("::");
		QString baseElementName;
		if (name.size() < 2)
			baseElementName = elementName;
		else
			baseElementName = (name[0] == diagramName) ? name[1] : elementName;
		if ((mElements.contains(baseElementName))) {
			initInheritance(mElements[baseElementName], id);
		}
		else {
			Id const parentId = getParentId(baseElementName);
			parents.append(parentId);
			initInheritance(parentId, id);
			mElements.insert(baseElementName, parentId);
		}
	}
	if (!parents.isEmpty())
		manageParents(parents);
}

void XmlParser::initInheritance(const Id &idFrom, const Id &idTo)
{
	Id inheritanceId("MetaEditor", "MetaEditor", "Inheritance",
			QUuid::createUuid().toString());

	QString const name = mLogicalApi.name(idTo) + "_Inherits_" + mLogicalApi.name(idFrom);

	setStandartConfigurations(inheritanceId, mMetamodel, name, "");
	mLogicalApi.setFrom(inheritanceId, idFrom);
	mLogicalApi.setTo(inheritanceId, idTo);
}

Id XmlParser::getParentId(const QString &elementName)
{
	Id parentId("MetaEditor", "MetaEditor", "MetaEntityImport",
			QUuid::createUuid().toString());

	setStandartConfigurations(parentId, mMetamodel, elementName, elementName);
	return parentId;
}

void XmlParser::setContains(const Id &id, const QString &diagramName)
{
	foreach (QString const elementName, mContainers[id]) {
		QStringList name = elementName.split("::");
		QString baseElementName;
		if (name.size() < 2)
			baseElementName = elementName;
		else
			baseElementName = (name[0] == diagramName) ? name[1] : elementName;
		if ((mElements.contains(baseElementName))) {
			initContains(id, mElements[baseElementName]);
		}
	}
}

void XmlParser::initContains(const Id &idFrom, const Id &idTo)
{
	Id containerId("MetaEditor", "MetaEditor", "Container",
			QUuid::createUuid().toString());

	QString const name = mLogicalApi.name(idFrom) + "_Contains_" + mLogicalApi.name(idTo);

	setStandartConfigurations(containerId, mMetamodel, name, "");
	mLogicalApi.setFrom(containerId, idFrom);
	mLogicalApi.setTo(containerId, idTo);
}

void XmlParser::setExplodes(const Id &id, const QString &diagramName)
{
	QString elementName = mExplosions[id];
	QStringList name = elementName.split("::");
	QString baseElementName;
	if (name.size() < 2)
		baseElementName = elementName;
	else
		baseElementName = (name[0] == diagramName) ? name[1] : elementName;
	if ((mElements.contains(baseElementName))) {
		mLogicalApi.setFrom(id, mElements[baseElementName]);
	} else {
		mLogicalApi.removeChild(mMetamodel, id);
		mLogicalApi.removeElement(id);
	}
}

void XmlParser::manageParents(const IdList &parents)
{
	foreach (Id const id, parents) {
		mLogicalApi.setProperty(id, "position", QPointF(mParentPositionX, 0));
		mParentPositionX += 120;
	}
}

void XmlParser::setStandartConfigurations(Id const &id, Id const &parent,
		const QString &name, const QString &displayedName)
{
	mLogicalApi.addChild(parent, id);
	mLogicalApi.setProperty(id, "name", name);
	if (displayedName != "")
		mLogicalApi.setProperty(id, "displayedName", displayedName);
	mLogicalApi.setFrom(id, Id::rootId());
	mLogicalApi.setTo(id, Id::rootId());
	mLogicalApi.setProperty(id, "fromPort", 0.0);
	mLogicalApi.setProperty(id, "toPort", 0.0);
	mLogicalApi.setProperty(id, "links", IdListHelper::toVariant(IdList()));
	mLogicalApi.setProperty(id, "outgoingExplosion", IdListHelper::toVariant(IdList()));
	mLogicalApi.setProperty(id, "incomingExplosions", IdListHelper::toVariant(IdList()));

	mLogicalApi.setProperty(id, "position", QPointF(0,0));
	mLogicalApi.setProperty(id, "configuration", QVariant(QPolygon()));
}

void XmlParser::setChildrenPositions(const Id &id, unsigned cellWidth, unsigned cellHeight)
{
	int rowWidth = ceil(sqrt(static_cast<qreal>(mLogicalApi.children(id).count())));
	int currentRow = 0;
	int currentColumn = 0;
	int sizeyElement = 100;
	int sizeyElements = 0;

	foreach(Id element, mLogicalApi.children(id)) {
		mLogicalApi.setProperty(element, "position", QPointF(currentColumn * (cellWidth + 40) + 50, sizeyElement));
		if (mLogicalApi.children(element).isEmpty())
			sizeyElement += 180;
		else
			sizeyElement += cellHeight * mLogicalApi.children(element).length() + 80;
		++currentRow;
		if (currentRow >= rowWidth) {
			currentRow = 0;
			++currentColumn;
			if (sizeyElement > sizeyElements)
				sizeyElements = sizeyElement;
			sizeyElement = 100;
		}
	}
	mCurrentWidth = rowWidth * cellWidth + 30;
	mCurrentHeight = sizeyElements;
}

void XmlParser::checkIndex()
{
	++mElementCurrentColumn;
	if (mElementCurrentColumn >= mElementsColumn) {
		mElementCurrentColumn = 0;
		mMoveHeight = 0;
		mMoveWidth += mCurrentWidth;
	}
}

void XmlParser::setElementPosition(const Id &id)
{
	mMoveHeight += mCurrentHeight;
	mLogicalApi.setProperty(id, "position", QPointF(mMoveWidth, mMoveHeight));
	setChildrenPositions(id, 160, 50);
	QRect const value = QRect(mMoveWidth, mMoveHeight, mCurrentWidth, mCurrentHeight);
	mLogicalApi.setProperty(id, "configuration", QVariant(QPolygon(value, false)));
	checkIndex();
}

void XmlParser::initEdgesConfiguration()
{
	initEdgesConfiguration(Id("MetaEditor", "MetaEditor", "Inheritance"));
	initEdgesConfiguration(Id("MetaEditor", "MetaEditor", "Container"));
	initEdgesConfiguration(Id("MetaEditor", "MetaEditor", "Explosion"));
}

void XmlParser::initEdgesConfiguration(Id const &type)
{
	for (Id const &edge : mGraphicalApi.graphicalElements(type)) {
		QPoint const fromPos = mGraphicalApi.property(mGraphicalApi.from(edge), "position").toPoint();
		QPoint const toPos = mGraphicalApi.property(mGraphicalApi.to(edge), "position").toPoint();
		QPoint const fromParentPos = mGraphicalApi.property(mGraphicalApi.parent(mGraphicalApi.from(edge)), "position")
				.toPoint();
		QPoint const toParentPos = mGraphicalApi.property(mGraphicalApi.parent(mGraphicalApi.to(edge)), "position")
				.toPoint();
		QPolygon configuration(QVector<QPoint>({ QPoint(0, 0), toParentPos + toPos + QPoint(50, 50) }));
		mGraphicalApi.setProperty(edge, "position", QVariant(fromParentPos + fromPos + QPoint(50, 50)));
		mGraphicalApi.setProperty(edge, "configuration", QVariant(configuration));
	}
}

void XmlParser::initGraphicalApi()
{
	for (qReal::Id const &id : mLogicalApi.children(qReal::Id::rootId())) {
		createGraphicalElement(id);
	}
}

void XmlParser::createGraphicalElement(Id const &logicalId)
{
	Id graphicalId(logicalId.editor(), logicalId.diagram(), logicalId.element(), QUuid::createUuid().toString());
	mGraphicalApi.addChild(mLogicalToGraphical[mLogicalApi.parent(logicalId)], graphicalId, logicalId);
	mLogicalToGraphical[logicalId] = graphicalId;

	QMapIterator<QString, QVariant> propertiesIterator = mLogicalApi.propertiesIterator(logicalId);
	while (propertiesIterator.hasNext()) {
		propertiesIterator.next();
		if (propertiesIterator.key() == "from" || propertiesIterator.key() == "to") {
			mGraphicalApi.setProperty(graphicalId, propertiesIterator.key()
					, mLogicalToGraphical[propertiesIterator.value().value<Id>()].toVariant());
		} else {
			mGraphicalApi.setProperty(graphicalId, propertiesIterator.key(), propertiesIterator.value());
		}
	}

	for (Id const &child : mLogicalApi.children(logicalId)) {
		createGraphicalElement(child);
	}
}
