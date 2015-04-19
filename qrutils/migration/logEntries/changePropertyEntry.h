#pragma once

#include <qrutils/migration/logEntries/logEntry.h>

namespace qReal {
namespace migration {

class QRUTILS_EXPORT ChangePropertyEntry : public LogEntry
{
public:
	ChangePropertyEntry(const Id &id, const QString &property
			, const QVariant &oldValue, const QVariant &newValue);

	QString toString() const override;
	void reverse(qrRepo::CommonRepoApi *repo) const;

private:
	Id mId;
	QString mProperty;
	QVariant mOldValue;
	QVariant mNewValue;
};

}
}
