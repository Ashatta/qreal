#pragma once

#include "migration/transformations/transformation.h"

namespace qReal {
namespace migration {

class RenameEdgeTransformation : public Transformation
{
public:
	RenameEdgeTransformation(QString const &oldName, QString const &newName);
};

}
}
