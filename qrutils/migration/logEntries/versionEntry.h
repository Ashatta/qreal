#pragma once

#include "qrutils/utilsDeclSpec.h"
#include "migration/logEntries/logEntry.h"

namespace qReal {
namespace migration {

class QRUTILS_EXPORT VersionEntry : public LogEntry
{
public:
	VersionEntry(int version, QString const &versionName);

	virtual QString toString() const;
	int version() const;
	QString name() const;

private:
	int mVersion;
	QString mName;
};

}
}
