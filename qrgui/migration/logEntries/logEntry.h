#pragma once

#include <QtCore/QString>

#include "qrkernel/ids.h"

namespace qReal {

class LogEntry
{
public:
	virtual ~LogEntry() {}

	virtual QString toString() const;
	static LogEntry *loadFromString(QString const &string);
};

}
