#pragma once

#include "qrgui/migration/logEntries/logEntry.h"

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
