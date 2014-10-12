#include "migration/analyzer.h"

#include "migration/logEntries/logEntry.h"
#include <QtCore/QDebug>

using namespace qReal::migration;

Analyzer::Analyzer(QHash<qReal::Id, QList<qReal::migration::LogEntry *> > const &log
		, QList<DifferenceModel *> const &differenceModels)
	: mLog(log), mDifferenceModels(differenceModels)
{
}

Analyzer::~Analyzer()
{
	foreach (Id const &id, mTransformations.keys()) {
		foreach (Transformation *transform, mTransformations[id]) {
			delete transform;
		}
	}
}

void Analyzer::analyze()
{
	handleRenames();
}

void Analyzer::handleRenames()
{
	foreach (DifferenceModel const * const diff, mDifferenceModels) {
		QList<ReplaceTypeTransformation *> replacedTypes = diff->replacedTypes();
		QHash<QString, QHash<QString, ReplaceTypeTransformation *> > transforms;
		QHash<QString, QString> newType;
		QHash<QString, QString> oldType;

		foreach (ReplaceTypeTransformation * const trans, replacedTypes) {
			qDebug() << trans->oldElementType() << " :: " << trans->newElementType() << endl;
			transforms[trans->oldElementType()][trans->newElementType()] = trans;
			newType[trans->oldElementType()] = trans->newElementType();
			oldType[trans->newElementType()] = trans->oldElementType();
		}

		QList<ReplaceTypeTransformation *> chain;

		foreach (ReplaceTypeTransformation * const trans, replacedTypes) {
			if (!transforms.contains(trans->oldElementType()))
				continue;

			findChain(trans, chain, transforms, newType, oldType);

			foreach (ReplaceTypeTransformation * const trans, chain) {
				qDebug() << " ! " << trans->oldElementType() << " :: " << trans->newElementType() << endl;
				mTransformations[Id()].append(trans);
			}

			chain.clear();
		}
	}
}

void Analyzer::findChain(
	ReplaceTypeTransformation * const trans
	, QList<ReplaceTypeTransformation *> &chain
	, QHash<QString, QHash<QString, ReplaceTypeTransformation *> > &transforms
	, QHash<QString, QString> const &newType
	, QHash<QString, QString> const &oldType
	) const
{
	chain.append(trans);
	transforms.remove(trans->oldElementType());

	QString nextElem = trans->oldElementType();

	while (oldType.contains(nextElem)) {
		if (!transforms.contains(oldType[nextElem]))
			break;
		chain.append(transforms[oldType[nextElem]][nextElem]);
		transforms.remove(oldType[nextElem]);
		nextElem = oldType[nextElem];
	}

	nextElem = trans->newElementType();

	while (newType.contains(nextElem)) {
		if (!transforms.contains(nextElem))
			break;
		chain.prepend(transforms[nextElem][newType[nextElem]]);
		transforms.remove(nextElem);
		nextElem = newType[nextElem];
	}

	if (chain.length() > 1 && chain.first()->newElementType() == chain.last()->oldElementType()) {
		QString tempName = chain.first()->newElementType();
		chain.first()->setNewElementType(tempName + "temp name that is impossible for regular elem to have");
		chain.append(new ReplaceTypeTransformation(chain.first()->newElementType(), tempName, QMap<QString,QString>()));
	}
}

QList<qReal::migration::Transformation *> Analyzer::transformations() const
{
	QList<Transformation *> result;
	foreach (QList<Transformation *> const &list, mTransformations.values()) {
		result.append(list);
	}

	return result;
}
