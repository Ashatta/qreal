#pragma once

#include "qrutils/utilsDeclSpec.h"
#include "qrutils/migration/transformations/transformation.h"

namespace qrRepo {
	class CommonRepoApi;
}

namespace qReal
{
namespace migration
{

class QRUTILS_EXPORT ReplaceTypeTransformation : public Transformation
{
public:
	ReplaceTypeTransformation(QString const &oldTypeName, QString const &newTypeName
			, QMap<QString, QString> const &propertyMap);

	void apply(models::ModelsInterface *model);

    QString getOldElementType() const;
    void setOldElementType(const QString &value);
    QString getNewElementType() const;
    void setNewElementType(const QString &value);

private:
    IdList findMatching(qReal::Id const &id = qReal::Id::rootId()) const;
    bool checkPropertiesMatch(qReal::Id const &id) const;

	Id createLogical(Id const &oldLogical) const;
	void createGraphical(Id const &logical, Id const &oldGraphical) const;
	void removeOldLogical(Id const &id) const;

	void initProperties(Id const &oldElement, Id const &newElement) const;
	void setProperty(Id const &id, QString const &property, QVariant const &value) const;
	void copyLinkProperties(Id const &oldId, Id const &newId) const;
	void copyLinks(qrRepo::CommonRepoApi &api, Id const &oldElement, Id const &newElement) const;

	qReal::models::ModelsInterface *mModel;

	QString mOldElementType;
	QString mNewElementType;
	QMap<QString, QVariant> mSearchProperties;
	QMap<QString, QString> mPropertyMap;
};

}
}
