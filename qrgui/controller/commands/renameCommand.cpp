#include "renameCommand.h"

#include "models/graphicalModelAssistApi.h"
#include "qrgui/migration/logEntries/renameEntry.h"

using namespace qReal::commands;

RenameCommand::RenameCommand(details::ModelsAssistInterface &model
		, Id const &id, QString const &newName
		, models::Exploser * const exploser)
	: mModel(model)
	, mId(id)
	, mOldName(mModel.name(mId))
	, mNewName(newName)
{
	initExplosions(exploser);
}

RenameCommand::RenameCommand(details::ModelsAssistInterface &model
		, Id const &id, QString const &oldName, QString const &newName
		, models::Exploser * const exploser)
	: mModel(model)
	, mId(id)
	, mOldName(oldName)
	, mNewName(newName)
{
	initExplosions(exploser);
}

bool RenameCommand::execute()
{
	return rename(mNewName);
}

bool RenameCommand::restoreState()
{
	return rename(mOldName);
}

void RenameCommand::initExplosions(models::Exploser * const exploser)
{
	if (!exploser) {
		return;
	}
	addPostAction(exploser->renameCommands(logicalId(mId), mNewName));
}

bool RenameCommand::rename(QString const &name)
{
	mModel.setName(mId, name);
	return true;
}

QList<qReal::migration::LogEntry *> RenameCommand::logEntries() const
{
	QList<migration::LogEntry *> result;
	Id const parentId = logicalId(mModel.idByIndex(mModel.indexById(mId).parent()));
	result << new migration::RenameEntry(logicalId(mId), parentId, mModel.name(parentId), mOldName, mNewName);
	return result;
}

qReal::Id RenameCommand::logicalId(Id const &id) const
{
	GraphicalModelAssistInterface *graphicalModel = dynamic_cast<GraphicalModelAssistInterface *>(&mModel);
	return (graphicalModel ? graphicalModel->logicalId(id) : id);
}
