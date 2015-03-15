#pragma once

#include "loopBlockRuleBase.h"

namespace generatorBase {
namespace semantics {

/// This rule is applied to blocks with loop semantics with non-iteration branch visited.
/// If branch comes into the node in the same with current loop block zone creates
/// infinite cycle, otherwise this rule is considered non-applicable.
class LoopWithNextVisitedRule : public LoopBlockRuleBase
{
public:
	LoopWithNextVisitedRule(SemanticTree *tree, const qReal::Id &id
			, const LinkInfo &iterationLink, const LinkInfo &nextLink);

	virtual bool apply();
};

}
}
