#include "logicalObject.h"

using namespace qrRepo::details;

LogicalObject::LogicalObject(const qReal::Id &id)
	: Object(id)
{
}

LogicalObject::LogicalObject(const QDomElement &element)
	: Object(element)
{
}

Object *LogicalObject::createClone() const
{
	return new LogicalObject(mId.sameTypeId());
}

bool LogicalObject::isLogicalObject() const
{
	return true;
}
