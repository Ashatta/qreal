#pragma once

#include "migration/logEntries/logEntry.h"

namespace qReal {
namespace migration {

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
}
