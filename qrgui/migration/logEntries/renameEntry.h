#pragma once

#include "qrgui/migration/logEntries/logEntry.h"

namespace qReal {
namespace migration {

class RenameEntry : public LogEntry
{
public:
	RenameEntry(qReal::Id const &id, QString const &oldName, QString const &newName);

	virtual QString toString() const;

private:
	qReal::Id mId;

	QString mOldName;
	QString mNewName;
};

}
}
