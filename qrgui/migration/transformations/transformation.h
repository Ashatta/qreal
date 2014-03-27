#pragma once

#include "qrkernel/ids.h"
#include "qrgui/models/models.h"

namespace qReal {
namespace migration {

class Transformation
{
public:
	virtual ~Transformation() {}

	virtual void apply(qReal::models::Models *model) = 0;
};

}
}
