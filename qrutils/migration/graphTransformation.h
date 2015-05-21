#pragma once

#include <QtCore/QRect>

#include <qrutils/graphUtils/baseGraphTransformationUnit.h>
#include <qrutils/migration/migration.h>
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
			, qrRepo::GraphicalRepoApi &toTemplate
			, const IdList &allowedTypes, qReal::migration::Migration::Policy policy);

	void apply();
	IdList createdElements() const;

protected:
	bool findMatch() override;

	bool checkRuleMatching() override;
	Id startElement() const override;

	bool compareElementTypesAndProperties(const Id &first, const Id &second) override;
	virtual bool compareWildCardProperties(const Id &model, const Id &pattern);

	Id toInRule(const Id &id) const override;
	Id fromInRule(const Id &id) const override;
	IdList linksInRule(const Id &id) const override;

private:
	void analyzeTemplates();
	void analyzeTemplate(QHash<QString, Id> &elements, qrRepo::GraphicalRepoApi &migrationTemplate, QRect &bounds);
	void elementsFromTemplate(QHash<QString, Id> &elements, qrRepo::GraphicalRepoApi &migrationTemplate
			, const Id &root);
	Id migrationDiagram(qrRepo::GraphicalRepoApi &migrationTemplate) const;

	void resolveOverlaps();

	void setMatch(const QHash<Id, Id> &match);
	void saveProperties();
	void addPropertyValue(const QString &migrationId, const QString &propertyName
			, const QString &regExp, const QString &value);

	void createNodes(const Id &root, const Id &createdRoot);
	void createLinks(const Id &root, const Id &diagram);
	void createWildCard(const Id &pattern, const Id &createdRoot);
	Id createElement(const Id &parent, const Id &type);

	void setWildCardProperties(const Id &pattern);
	void setLogicalProperties(const Id &created, const Id &rule);
	QString getNewValue(const QString &migrationId, const QString &propertyName, QString valueTemplate);
	void setNodesGraphicalProperties(const Id &root);
	void setLinksGraphicalProperties(const Id &root);

	void deleteElements();

	qrRepo::GraphicalRepoApi &mFromTemplate;
	qrRepo::GraphicalRepoApi &mToTemplate;
	IdList mAllowedTypes;
	QHash<QString, QHash<QString, QStringList> > mPropertiesMatches;
	QHash<QString, Id> mIdToFrom;
	QHash<QString, Id> mIdToTo;
	QHash<Id, Id> mToModel;
	QHash<Id, Id> mCurrentMatch;
	QHash<Id, QHash<Id, QString> > mAnyPropertyToReal;

	QList<QString> mIgnoreProperties;

	IdList mCreatedElements;
	bool mRepeat;

	QRect mFromRect;
	QRect mToRect;
	QRect mMatchRect;
};

}
}
