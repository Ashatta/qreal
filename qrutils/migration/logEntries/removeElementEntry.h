#pragma once

#include <qrutils/migration/logEntries/logEntry.h>

namespace qReal {
namespace migration {

class RemoveElementEntry : public LogEntry
{
public:
	RemoveElementEntry(const Id &id, const QMap<QString, QVariant> &properties, const Id &parent
			, const Id &logical = Id());

	QString toString() const override;
	void reverse(qrRepo::CommonRepoApi *repo) const override;

private:
	Id mId;
	QMap<QString, QVariant> mProperties;
	Id mParent;
	Id mLogicalId;
};

}
}
