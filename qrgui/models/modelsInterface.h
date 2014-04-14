#pragma once

#include <QtCore/QAbstractItemModel>

namespace qrRepo {
	class RepoControlInterface;
	class LogicalRepoApi;
	class GraphicalRepoApi;
}

namespace qReal {
namespace models {
	class GraphicalModelAssistApi;
	class LogicalModelAssistApi;
namespace details {
	class GraphicalModel;
	class LogicalModel;
}

class ModelsInterface
{
public:
	virtual ~ModelsInterface() {}

	virtual QAbstractItemModel *graphicalModel() const = 0;
	virtual QAbstractItemModel *logicalModel() const = 0;

	virtual GraphicalModelAssistApi &graphicalModelAssistApi() const = 0;
	virtual LogicalModelAssistApi &logicalModelAssistApi() const = 0;

	virtual qrRepo::RepoControlInterface &repoControlApi() const = 0;

	virtual qrRepo::LogicalRepoApi const &logicalRepoApi() const = 0;
	virtual qrRepo::LogicalRepoApi &mutableLogicalRepoApi() const = 0;

	virtual qrRepo::GraphicalRepoApi const &graphicalRepoApi() const = 0;

	virtual void reinit() = 0;
};

}
}
