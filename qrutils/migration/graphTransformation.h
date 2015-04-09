#pragma once

#include <qrutils/graphUtils/baseGraphTransformationUnit.h>
#include <qrgui/plugins/toolPluginInterface/usedInterfaces/logicalModelAssistInterface.h>
#include <qrgui/plugins/toolPluginInterface/usedInterfaces/graphicalModelAssistInterface.h>
#include <qrrepo/graphicalRepoApi.h>

namespace qReal {
namespace migration {

class GraphTransformation : public BaseGraphTransformationUnit
{
public:
	GraphTransformation(LogicalModelAssistInterface &logicalRepoApi
			, GraphicalModelAssistInterface &graphicalRepoApi
			, qrRepo::GraphicalRepoApi &fromTemplate
			, qrRepo::GraphicalRepoApi &toTemplate);

	void apply();

protected:
	bool findMatch() override;

	bool checkRuleMatching() override;
	Id startElement() const override;

	bool compareElementTypesAndProperties(const Id &first, const Id &second) override;

	Id toInRule(const Id &id) const override;
	Id fromInRule(const Id &id) const override;
	IdList linksInRule(const Id &id) const override;

private:
	void analyzeTemplates();
	void elementsFromTemplate(QHash<QString, Id> &elements, qrRepo::GraphicalRepoApi &migrationTemplate
			, const Id &root);
	Id migrationDiagram(qrRepo::GraphicalRepoApi &migrationTemplate) const;

	void saveProperties();
	void createNodes(const Id &root, const Id &createdRoot);
	void createLinks(const Id &root, const Id &diagram);
	void setLogicalProperties(const Id &created, const Id &rule);
	void setNodesGraphicalProperties(const Id &root);
	void setLinksGraphicalProperties(const Id &root);
	void deleteElements();

	qrRepo::GraphicalRepoApi &mFromTemplate;
	qrRepo::GraphicalRepoApi &mToTemplate;
	QHash<QString, QHash<QString, QStringList> > mPropertiesMatches;
	QHash<QString, Id> mIdToFrom;
	QHash<QString, Id> mIdToTo;
	QHash<Id, Id> mToModel;
	QHash<Id, Id> mCurrentMatch;

	QList<QString> mIgnoreProperties;
};

}
}
