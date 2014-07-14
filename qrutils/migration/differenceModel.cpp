#include "migration/differenceModel.h"
#include "migration/transformations/replaceTypeTransformation.h"

using namespace qReal::migration;

DifferenceModel::DifferenceModel(qrRepo::RepoApi *oldRepo, qrRepo::RepoApi *newRepo)
	: mOldRepo(oldRepo), mNewRepo(newRepo)
{
	findReplacedTypes();
}

QList<ReplaceTypeTransformation *> DifferenceModel::replacedTypes() const
{
	return mReplacedTypes;
}

void DifferenceModel::findReplacedTypes()
{
	IdList typeIds = mOldRepo->elementsByType("MetaEntityNode");
	typeIds.append(mOldRepo->elementsByType("MetaEntityEdge"));
	foreach (Id const &typeElem, typeIds) {
		if (mOldRepo->isLogicalElement(typeElem)) {
			continue;
		}

		ReplaceTypeTransformation *transform = transformation(typeElem);
		if (transform) {
			mReplacedTypes.append(transform);
		}
	}
}

ReplaceTypeTransformation * DifferenceModel::transformation(Id const &elem) const //this would probably be changed to Transformation
{
	if (!mNewRepo->exist(elem)) {
		return nullptr;
	}

	QString const oldName = mOldRepo->name(elem);
	QString const newName = mNewRepo->name(elem);

	QMap<QString, QString> properties = propertyMap(elem);

	if ((oldName == newName) && properties.empty()) {
		return nullptr;
	}

	return new ReplaceTypeTransformation(oldName, newName, properties);
}

QMap<QString, QString> DifferenceModel::propertyMap(Id const &elem) const
{
	QMap<QString, QString> result;

	foreach (Id const &child, mOldRepo->children(elem)) {
		if ((child.element() != "MetaEntity_Attribute") || !mNewRepo->exist(child)) {
			continue;
		}

		QString const oldName = mOldRepo->name(child);
		QString const newName = mNewRepo->name(child);

		if (oldName != newName) {
			result.insert(oldName, newName);
		}
	}

	return result;
}
