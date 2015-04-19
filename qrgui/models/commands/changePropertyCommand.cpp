#include "changePropertyCommand.h"

#include <qrutils/migration/logEntries/changePropertyEntry.h>

using namespace qReal::commands;

ChangePropertyCommand::ChangePropertyCommand(models::LogicalModelAssistApi * const model
		, const QString &property, const Id &id, const QVariant &newValue)
	: mLogicalModel(model)
	, mId(id)
	, mPropertyName(property)
	, mOldValue(mLogicalModel->propertyByRoleName(mId, mPropertyName))
	, mNewValue(newValue)
{
}

bool ChangePropertyCommand::execute()
{
	return setProperty(mNewValue);
}

bool ChangePropertyCommand::restoreState()
{
	return setProperty(mOldValue);
}

bool ChangePropertyCommand::setProperty(const QVariant &value)
{
	mLogicalModel->setPropertyByRoleName(mId, value, mPropertyName);
	return true;
}

QList<qReal::migration::LogEntry *> ChangePropertyCommand::logEntries() const
{
	return { new migration::ChangePropertyEntry(mId, mPropertyName, mOldValue, mNewValue) };
}
