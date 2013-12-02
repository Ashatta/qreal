#pragma once

#include "qrgui/migration/logEntries/logEntry.h"

namespace qReal {
namespace migration {

class RenameEntry : public LogEntry
{
public:
	RenameEntry(qReal::Id const &id, qReal::Id const &parent, QString const &oldName, QString const &newName);

	virtual QString toString() const;
	void reverse(qrRepo::details::Repository *repo) const;

	qReal::Id id() const;
	qReal::Id parent() const;

	QString oldName() const;
	QString newName() const;

private:
	qReal::Id mId;
	qReal::Id mParentId;

	QString mOldName;
	QString mNewName;
};

}
}
