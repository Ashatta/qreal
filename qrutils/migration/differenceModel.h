#include "qrrepo/repoApi.h"

namespace qReal
{
namespace migration
{

class Transformation;

class DifferenceModel
{
public:
	DifferenceModel(qrRepo::RepoApi *oldRepo, qrRepo::RepoApi *newRepo);

	QList<Transformation *> replacedTypes() const;

private:
	void findReplacedTypes();
	Transformation *transformation(Id const &elem) const;
	QMap<QString, QString> propertyMap(Id const &elem) const;

	qrRepo::RepoApi const * const mOldRepo;
	qrRepo::RepoApi const * const mNewRepo;

	QList<Transformation *> mReplacedTypes;
};

}
}
