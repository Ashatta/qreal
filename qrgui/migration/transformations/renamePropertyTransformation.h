#pragma once

#include "migration/transformations/transformation.h"

namespace qReal {
namespace migration {

class RenamePropertyTransformation : public Transformation
{
public:
	RenamePropertyTransformation(qReal::Id const &parent, QString const &oldName, QString const &newName);

private:
	qReal::Id mParent;

	QString mOldName;
	QString mNewName;
};

}
}
