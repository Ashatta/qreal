#include "subprogramBlock.h"

#include <qrutils/nameNormalizer.h>

using namespace qReal::interpretation::blocks;

SubprogramBlock::SubprogramBlock()
{
}

void SubprogramBlock::run()
{
	const Id logicalId = mGraphicalModelApi->logicalId(id());

	const QString name = mLogicalModelApi->name(logicalId);

	const QString validName = utils::NameNormalizer::normalizeStrongly(name, false);
	if (validName.isEmpty()) {
		error(tr("Please enter valid c-style name for subprogram \"") + name + "\"");
		return;
	}

	const Id logicalDiagram = mLogicalModelApi->logicalRepoApi().outgoingExplosion(logicalId);
	const IdList diagrams = mGraphicalModelApi->graphicalIdsByLogicalId(logicalDiagram);
	if (!diagrams.isEmpty()) {
		emit stepInto(diagrams[0]);
	}
}

void SubprogramBlock::finishedSteppingInto()
{
	emit done(mNextBlockId);
}
