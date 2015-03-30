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

	bool compareElements(const Id &first, const Id &second) override;
	bool compareLinks(const Id &first, const Id &second) override;
	bool compareElementTypesAndProperties(const Id &first, const Id &second) override;

	Id toInRule(const Id &id) const override;
	Id fromInRule(const Id &id) const override;
	IdList linksInRule(const Id &id) const override;

private:
	void analyzeTemplates();
	void elementsFromTemplate(IdList &elements, qrRepo::GraphicalRepoApi &migrationTemplate, const Id &root);
	Id migrationDiagram(qrRepo::GraphicalRepoApi &migrationTemplate) const;

	void deleteElement(const Id &id);
	void createElement(const Id &templateElement, const QHash<Id, Id> &match);
	void setProperties(const Id &toElement);

	qrRepo::GraphicalRepoApi &mFromTemplate;
	qrRepo::GraphicalRepoApi &mToTemplate;
	QHash<Id, QHash<QString, QStringList> > mPropertiesMatches;
	QHash<Id, QMap<QString, QVariant> > mOldGraphicalProperties;
	QHash<Id, QMap<QString, QVariant> > mOldLogicalProperties;
	IdList mFromElements;
	IdList mToElements;
	QHash<Id, Id> mFromToMap;
	QHash<Id, Id> mToFromMap;
	QHash<Id, Id> mToModelMap;

	QList<QString> mIgnoreProperties;
};

}
}
