#pragma once

#include <QtCore/QString>

#include "qrutils/utilsDeclSpec.h"
#include "qrkernel/ids.h"
#include "qrrepo/commonRepoApi.h"

namespace qReal {
namespace migration {

class QRUTILS_EXPORT LogEntry
{
public:
	virtual ~LogEntry() {}

	virtual QString toString() const;
	static LogEntry *loadFromString(QString const &string);

	virtual void reverse(qrRepo::CommonRepoApi *repo) const;
};

}
}