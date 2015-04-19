#include "createElementCommand.h"

#include <qrutils/migration/logEntries/createElementEntry.h>
#include <qrgui/models/graphicalModelAssistApi.h>

using namespace qReal::commands;

CreateElementCommand::CreateElementCommand(
		models::LogicalModelAssistApi &logicalApi
		, models::GraphicalModelAssistApi &graphicalApi
		, const models::Exploser &exploser
		, const Id &logicalParent
		, const Id &graphicalParent
		, const Id &id
		, bool isFromLogicalModel
		, const QString &name
		, const QPointF &position)
	: mImpl(logicalApi, graphicalApi, exploser, logicalParent
			, graphicalParent, id, isFromLogicalModel, name, position)
	, mGraphicalApi(graphicalApi)
{
}

qReal::Id CreateElementCommand::result() const
{
	return mResult;
}

void CreateElementCommand::setNewPosition(const QPointF &position)
{
	mImpl.setNewPosition(position);
}

bool CreateElementCommand::execute()
{
	mResult = mImpl.create();
	return true;
}

bool CreateElementCommand::restoreState()
{
	mImpl.remove();
	return true;
}

QList<qReal::migration::LogEntry *> CreateElementCommand::logEntries() const {
	QList<migration::LogEntry *> result;
	result << new migration::CreateElementEntry(mResult);
	if (mGraphicalApi.graphicalIdsByLogicalId(mGraphicalApi.logicalId(mResult)).size() == 1) {
		result << new migration::CreateElementEntry(mGraphicalApi.logicalId(mResult));
	}

	return result;
}
