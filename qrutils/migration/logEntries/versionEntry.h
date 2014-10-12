#pragma once

#include "qrutils/utilsDeclSpec.h"
#include "migration/logEntries/logEntry.h"

namespace qReal {
namespace migration {

class QRUTILS_EXPORT VersionEntry : public LogEntry
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
