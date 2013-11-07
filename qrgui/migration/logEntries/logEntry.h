#pragma once

#include <QtCore/QString>

#include "qrkernel/ids.h"
#include "qrrepo/private/repository.h"

namespace qReal {
namespace migration {

class LogEntry
{
public:
	virtual ~LogEntry() {}

	virtual QString toString() const;
	static LogEntry *loadFromString(QString const &string);

	virtual void reverse(qrRepo::details::Repository *repo) const;
};

}
}
