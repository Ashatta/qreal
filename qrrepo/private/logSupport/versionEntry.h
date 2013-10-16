#pragma once

#include "private/logSupport/logEntry.h"

namespace qReal {

class VersionEntry : public LogEntry
{
public:
	VersionEntry(int version);

	virtual QString toString() const;
	int version() const;

private:
	int mVersion;
};

}
