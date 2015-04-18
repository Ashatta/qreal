#pragma once

#include "qrutils/utilsDeclSpec.h"
#include "qrkernel/ids.h"
#include "qrgui/models/modelsInterface.h"

namespace qReal {
namespace migration {

class QRUTILS_EXPORT Transformation
{
public:
	virtual ~Transformation() {}

	virtual void apply(qReal::models::ModelsInterface *model, const IdList &ignoredElements) = 0;
};

}
}
