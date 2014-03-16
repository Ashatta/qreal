#pragma once

#include "migration/transformations/transformation.h"

namespace qReal {
namespace migration {

class RenameNodeTransformation : public Transformation
{
public:
	RenameNodeTransformation(QString const &oldName, QString const &newName);
};

}
}
