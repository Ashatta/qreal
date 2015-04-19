#pragma once

#include <qrutils/migration/logEntries/logEntry.h>

namespace qReal {
namespace migration {

class QRUTILS_EXPORT CreateElementEntry : public LogEntry
{
public:
	CreateElementEntry(const Id &id);

	QString toString() const override;
	void reverse(qrRepo::CommonRepoApi *repo) const;

private:
	Id mId;
};

}
}
