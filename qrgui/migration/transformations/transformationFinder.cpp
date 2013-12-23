#include "transformationFinder.h"

using namespace qReal::migration;

TransformationFinder::TransformationFinder(LogicalModelAssistInterface &logicalModelApi
		, GraphicalModelAssistInterface &graphicalModelApi
		, gui::MainWindowInterpretersInterface &interpretersInterface)
	: BaseGraphTransformationUnit(logicalModelApi, graphicalModelApi, interpretersInterface)
{
}

void TransformationFinder::apply(Transformation *transformation)
{
	mCurrentTransformation = transformation;
	mMatches.clear();
	checkRuleMatching();
}

bool TransformationFinder::findMatch()
{
	return checkRuleMatching();
}

qReal::Id TransformationFinder::startElement() const
{
	IdList topmostElements = mCurrentTransformation->before()->children(Id::rootId());
	foreach (Id const &elem, topmostElements) {
		if (elem.element() == "Element") {
			return elem;
		}
	}

	return topmostElements.first();
}

bool TransformationFinder::checkRuleMatching()
{
	IdList elements;
	foreach (Id const &diagram, mGraphicalModelApi.children(Id::rootId())) {
		elements.append(allChildren(diagram));
	}

	return BaseGraphTransformationUnit::checkRuleMatching(elements);
}

qReal::IdList TransformationFinder::allChildren(qReal::Id const &id) const
{
	IdList result = mGraphicalModelApi.children(id);

	foreach (Id const &child, mGraphicalModelApi.children(id)) {
		result.append(allChildren(child));
	}

	return result;
}

bool TransformationFinder::compareElements(Id const &first, Id const &second) const
{
	return compareElementTypesAndProperties(first, second) && compareChildren(first, second);
}

bool TransformationFinder::compareElementTypesAndProperties(Id const &first, Id const &second) const
{
	if (mCurrentTransformation->before()->name(second) == first.element()) {
		QMap<QString, QVariant> properties = mCurrentTransformation->before()->properties(second);
		foreach (QString const &property, properties.keys()) {
			if (mDefaultProperties.contains(property)) {
				continue;
			}

			qrRepo::LogicalRepoApi const &logicalRepoApi = mLogicalModelApi.logicalRepoApi();
			Id const firstLogical = mGraphicalModelApi.logicalId(first);
			if (!logicalRepoApi.hasProperty(firstLogical, property)
					|| (mCurrentTransformation->before()->property(second, property)
							!= logicalRepoApi.property(firstLogical, property))) {
				return false;
			}
		}

		return true;
	}

	return false;
}

bool TransformationFinder::compareChildren(Id const &first, Id const &second) const
{
	return true;
}

qReal::Id TransformationFinder::toInRule(qReal::Id const &id) const
{
	return mCurrentTransformation->before()->to(id);
}

qReal::Id TransformationFinder::fromInRule(qReal::Id const &id) const
{
	return mCurrentTransformation->before()->from(id);
}

qReal::IdList TransformationFinder::linksInRule(qReal::Id const &id) const
{
	return mCurrentTransformation->before()->links(id);
}
