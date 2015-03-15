#pragma once

#include "qrgui/models/exploser.h"

namespace qReal {
namespace commands {

class QRGUI_MODELS_EXPORT CreateRemoveCommandImplementation
{
public:
	CreateRemoveCommandImplementation(
			models::LogicalModelAssistApi &logicalApi
			, models::GraphicalModelAssistApi &graphicalApi
			, const models::Exploser &exploser
			, const Id &logicalParent
			, const Id &graphicalParent
			, const Id &id
			, bool isFromLogicalModel
			, const QString &name
			, const QPointF &position);

	Id create();
	void remove();

	Id id() const;

	bool equals(const CreateRemoveCommandImplementation &other) const;

	/// @todo: Bad method, required only for linkers. Get rid of it.
	/// Modifies command setting new creation position.
	void setNewPosition(const QPointF &position);

private:
	void refreshAllPalettes();

	models::LogicalModelAssistApi &mLogicalApi;
	models::GraphicalModelAssistApi &mGraphicalApi;
	const models::Exploser &mExploser;
	const Id mLogicalParent;
	const Id mGraphicalParent;
	Id mId;
	const bool mIsFromLogicalModel;
	const QString mName;
	QPointF mPosition;
	QMap<QString, QVariant> mLogicalPropertiesSnapshot;
	QMap<QString, QVariant> mGraphicalPropertiesSnapshot;
	Id mOldLogicalId;
};

inline bool operator==(const CreateRemoveCommandImplementation &i1
		, const CreateRemoveCommandImplementation &i2)
{
	return i1.equals(i2);
}

}
}
