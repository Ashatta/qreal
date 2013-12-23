#pragma once

#include "qrkernel/ids.h"
#include "qrgui/models/models.h"

namespace qReal {
namespace migration {

class Transformation
{
public:
	typedef QPair<qReal::Id, QString> Property;

	Transformation();
	virtual ~Transformation();

	qrRepo::RepoApi *before() const;
	qrRepo::RepoApi *after() const;

	QMap<qReal::Id, qReal::Id> elements() const;
	QMap<Property, Property> properties() const;

protected:
	qrRepo::RepoApi *mBefore;
	qrRepo::RepoApi *mAfter;

	QMap<qReal::Id, qReal::Id> mElements;
	QMap<Property, Property> mProperties;
};

}
}
