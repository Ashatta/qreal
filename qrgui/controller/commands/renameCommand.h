#pragma once

#include "controller/commands/abstractCommand.h"
#include "toolPluginInterface/usedInterfaces/details/modelsAssistInterface.h"
#include "pluginManager/exploser.h"

namespace qReal {
namespace commands {

/// Assigns to element with given id given name using given model (logical or graphical)
class RenameCommand : public AbstractCommand
{
public:
	/// This overload gets old name automatically using models api
	RenameCommand(details::ModelsAssistInterface &model
			, Id const &id, QString const &newName
			, Exploser const * const exploser = nullptr);

	/// This overload alows to specify old name manually
	RenameCommand(details::ModelsAssistInterface &model
			, Id const &id, QString const &oldName, QString const &newName
			, Exploser const * const exploser = nullptr);

protected:
	virtual bool execute();
	virtual bool restoreState();

	virtual QList<migration::LogEntry *> logEntries() const;

private:
	void initExplosions(Exploser const * const exploser);

	bool rename(QString const &name);
	Id logicalId(Id const &id) const;

	details::ModelsAssistInterface &mModel;
	Id const mId;
	QString const mOldName;
	QString const mNewName;
};

}
}
