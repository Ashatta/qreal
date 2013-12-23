#pragma once

#include "qrutils/graphUtils/baseGraphTransformationUnit.h"
#include "migration/transformations/transformation.h"

namespace qReal {
namespace migration {

class TransformationFinder : public BaseGraphTransformationUnit
{
public:
	TransformationFinder(LogicalModelAssistInterface &logicalModelApi
			, GraphicalModelAssistInterface &graphicalModelApi
			, gui::MainWindowInterpretersInterface &interpretersInterface);

	void apply(Transformation *transformation);

	virtual bool findMatch();

protected:
	virtual Id startElement() const;
	virtual bool checkRuleMatching();

	IdList allChildren(Id const &id) const;

	virtual bool compareElements(Id const &first, Id const &second) const;
	virtual bool compareElementTypesAndProperties(Id const &first, Id const &second) const;
	virtual bool compareChildren(Id const &first, Id const &second) const;

	virtual Id toInRule(Id const &id) const;
	virtual Id fromInRule(Id const &id) const;
	virtual IdList linksInRule(Id const &id) const;

	Transformation *mCurrentTransformation;
};

}
}
