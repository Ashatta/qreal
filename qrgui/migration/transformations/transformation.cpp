#include "migration/transformations/transformation.h"

using namespace qReal::migration;

Transformation::Transformation()
	: mBefore(new qrRepo::RepoApi("", true)), mAfter(new qrRepo::RepoApi("", true))
{
}

Transformation::~Transformation()
{
	delete mBefore;
	delete mAfter;
}

qrRepo::RepoApi *Transformation::before() const
{
	return mBefore;
}

qrRepo::RepoApi *Transformation::after() const
{
	return mAfter;
}

QMap<qReal::Id, qReal::Id> Transformation::elements() const
{
	return mElements;
}

QMap<Transformation::Property, Transformation::Property> Transformation::properties() const
{
	return mProperties;
}
