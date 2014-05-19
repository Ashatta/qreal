#include "migration/analyzer.h"

#include "migration/logEntries/logEntry.h"
#include <qDebug>

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
            qDebug() << trans->getOldElementType() << " :: " << trans->getNewElementType() << endl;
            transforms[trans->getOldElementType()][trans->getNewElementType()] = trans;
            newType[trans->getOldElementType()] = trans->getNewElementType();
            oldType[trans->getNewElementType()] = trans->getOldElementType();
        }

        foreach (ReplaceTypeTransformation * const trans, replacedTypes) {
            if (!transforms.contains(trans->getOldElementType()))
                continue;
            QList<ReplaceTypeTransformation *> chain;
            chain.append(trans);
            transforms.remove(trans->getOldElementType());

            QString nextElem = trans->getOldElementType();

            while (oldType.contains(nextElem)) {
                if (!transforms.contains(oldType[nextElem]))
                    break;
                chain.append(transforms[oldType[nextElem]][nextElem]);
                transforms.remove(oldType[nextElem]);
                nextElem = oldType[nextElem];
            }

            nextElem = trans->getNewElementType();

            while (newType.contains(nextElem)) {
                if (!transforms.contains(nextElem))
                    break;
                chain.prepend(transforms[nextElem][newType[nextElem]]);
                transforms.remove(nextElem);
                nextElem = newType[nextElem];
            }

            if (chain.length() > 1 && chain.first()->getNewElementType() == chain.last()->getOldElementType()) {
                QString tempName = chain.first()->getNewElementType();
                chain.first()->setNewElementType(tempName + "temp name that is impossible for regular elem to have");
                chain.append(new ReplaceTypeTransformation(chain.first()->getNewElementType(), tempName, QMap<QString,QString>()));
            }

            foreach (ReplaceTypeTransformation * const trans, chain) {
                qDebug() << " ! " << trans->getOldElementType() << " :: " << trans->getNewElementType() << endl;
                mTransformations[Id()].append(trans);
            }
        }
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
