#pragma once

#include "qrutils/migration/logEntries/logEntry.h"

namespace qReal {
namespace migration {

class RenameEntry : public LogEntry
{
public:
	RenameEntry(qReal::Id const &id, qReal::Id const &parent, QString const &parentName, QString const &oldName
			, QString const &newName);

	virtual QString toString() const;
	void reverse(qrRepo::CommonRepoApi *repo) const;

	qReal::Id id() const;
	qReal::Id parent() const;
	QString parentName() const;

	QString oldName() const;
	QString newName() const;

private:
	qReal::Id mId;
	qReal::Id mParent;

	QString mParentName;

	QString mOldName;
	QString mNewName;
};

}
}
