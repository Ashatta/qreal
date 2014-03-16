#pragma once

#include "qrkernel/ids.h"
#include "qrgui/models/models.h"

namespace qReal {
namespace migration {

class Transformation
{
public:
	Transformation();
	virtual ~Transformation();

	virtual void apply(qReal::models::Models *model);

protected:
	virtual IdList findMatching(qReal::Id const &id = qReal::Id::rootId()) const;
	virtual bool checkPropertiesMatch(qReal::Id const &id) const;

	Id createLogical(Id const &oldLogical) const;
	void createGraphical(Id const &logical, Id const &oldGraphical) const;
	void removeOldLogical(Id const &id) const;

	void initProperties(Id const &oldElement, Id const &newElement) const;
	void setProperty(Id const &id, QString const &property, QVariant const &value) const;
	void copyLinkProperties(Id const &oldId, Id const &newId) const;
	void copyLinks(qrRepo::CommonRepoApi &api, Id const &oldElement, Id const &newElement) const;

	qReal::models::Models *mModel;

	QString mOldElementType;
	QString mNewElementType;
	QMap<QString, QVariant> mSearchProperties;
	QMap<QString, QString> mPropertyMap;
};

}
}
