#include "renameCommand.h"

#include "models/graphicalModelAssistApi.h"
#include "qrrepo/private/logSupport/renameEntry.h"

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
	GraphicalModelAssistInterface *graphicalModel = dynamic_cast<GraphicalModelAssistInterface *>(&mModel);
	Id const logicalId = graphicalModel ? graphicalModel->logicalId(mId) : mId;
	addPostAction(exploser->renameCommands(logicalId, mNewName));
}

bool RenameCommand::rename(QString const &name)
{
	mModel.setName(mId, name);
	return true;
}

QList<qReal::LogEntry *> RenameCommand::logEntries() const
{
	QList<LogEntry *> result;
	result << new RenameEntry(mId, mOldName, mNewName);
	return result;
}
