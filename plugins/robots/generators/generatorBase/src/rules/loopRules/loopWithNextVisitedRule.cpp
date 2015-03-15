#include "loopWithNextVisitedRule.h"

using namespace generatorBase::semantics;
using namespace qReal;

LoopWithNextVisitedRule::LoopWithNextVisitedRule(SemanticTree *tree
		, const Id &id, const LinkInfo &iterationLink, const LinkInfo &nextLink)
	: LoopBlockRuleBase(tree, id, iterationLink, nextLink)
{
}

bool LoopWithNextVisitedRule::apply()
{
	if (alreadyCreated(mIterationLink) || !alreadyCreated(mNextLink)) {
		return false;
	}

	LoopNode * const thisNode = static_cast<LoopNode *>(mTree->findNodeFor(mId));
	NonZoneNode * const nextNode = mTree->findNodeFor(mNextLink.target);

	if (thisNode->parentZone() != nextNode->parentZone()) {
		return false;
	}

	makeLoopStartingFrom(nextNode);

	SemanticNode * const iterationNode = mTree->produceNodeFor(mIterationLink.target);
	thisNode->bodyZone()->appendChild(iterationNode);

	return true;
}
