#pragma once

#include "qrkernel/ids.h"
#include "qrgui/models/modelsInterface.h"

namespace qReal {
namespace migration {

class Transformation
{
public:
	virtual ~Transformation() {}

	virtual void apply(qReal::models::ModelsInterface *model) = 0;
};

}
}
