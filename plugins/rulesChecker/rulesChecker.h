﻿#pragma once

#include "../../../qrgui/mainwindow/projectManager/projectManagementInterface.h"
#include "../../../qrgui/toolPluginInterface/toolPluginInterface.h"

namespace qReal{
namespace rulesChecker{

//! @class RulesChecker watches current diagram for errors and makes report
class RulesChecker : public QObject
{
	Q_OBJECT

public slots:
	 void check();

public:
	 RulesChecker(qrRepo::GraphicalRepoApi const &graphicalRepoApi
			 , qReal::gui::MainWindowInterpretersInterface &interpretersInterface);
	// std destructor ok
private:
	 enum ErrorsType {
		 NoStartNode,
		 NoEndNode,
		 IncorrectLink
	 };

	 //! starts DFS for all connected component
	 void researchDiagram();
	 //! DFS, removes elements from metamodel list while making detour
	 //! @returns bool reached the final node
	 bool makeDetour(Id const currentNode);
	 //! post-checking for connected component without head-node
	 void findIncorrectLinks();

	 //! filters containers
	 //! @return IdList of head-nodes
	 IdList findStartingElements(IdList &list);

	 void postError(ErrorsType const error, Id badNode);
	 bool isLink(Id const &model);

	 qrRepo::GraphicalRepoApi const *mGRepoApi;
	 qReal::gui::MainWindowInterpretersInterface *mWindowInterface;

	 //! TODO: find better place for that
	 QStringList linkTypes;
	 QStringList containerTypes;

	 //! consists of models from current diagram
	 IdList metamodels;
	 //! main flag
	 bool hasNoErrors;
};

}
}
