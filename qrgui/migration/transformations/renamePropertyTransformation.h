#pragma once

#include "migration/transformations/transformation.h"

namespace qReal {
namespace migration {

class RenamePropertyTransformation : public Transformation
{
public:
	RenamePropertyTransformation(QString const &parent, QString const &oldName, QString const &newName);
};

}
}
