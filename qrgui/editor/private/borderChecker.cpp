#include "borderChecker.h"

#include "editor/nodeElement.h"

BorderChecker::BorderChecker(const NodeElement* const node)
	: mNode(node), mBorderValues(node->borderValues())
	, mXHor(mBorderValues[0])
	, mYHor(mBorderValues[1])
	, mXVert(mBorderValues[2])
	, mYVert(mBorderValues[3])
{
}

bool BorderChecker::checkLowerBorder(const QPointF &point) const
{
	const qreal checkingPointX = point.x();
	const qreal checkingPointY = point.y();
	const QRectF rc = mNode->boundingRect();
	return (checkingPointX >= rc.x() + mXHor) && (checkingPointX <= rc.x() + rc.width() - mXHor)
			&& (checkingPointY >= rc.y() + rc.height() - mYHor)
			&& (checkingPointY <= rc.y() + rc.height() + mYHor);
}

bool BorderChecker::checkUpperBorder(const QPointF &point) const
{
	const qreal checkingPointX = point.x();
	const qreal checkingPointY = point.y();
	const QRectF rc = mNode->boundingRect();
	return (checkingPointX >= rc.x() + mXHor) && (checkingPointX <= rc.x() + rc.width() - mXHor)
			&& (checkingPointY >= rc.y() - mYHor)
			&& (checkingPointY <= rc.y() + mYHor);
}

bool BorderChecker::checkLeftBorder(const QPointF &point) const
{
	const qreal checkingPointX = point.x();
	const qreal checkingPointY = point.y();
	const QRectF rc = mNode->boundingRect();
	return (checkingPointX >= rc.x() - mXVert) && (checkingPointX <= rc.x() + mXVert)
			&& (checkingPointY >= rc.y() + mYVert)
			&& (checkingPointY <= rc.y() + rc.height() - mYVert);
}

bool BorderChecker::checkRightBorder(const QPointF &point) const
{
	const qreal checkingPointX = point.x();
	const qreal checkingPointY = point.y();
	const QRectF rc = mNode->boundingRect();
	return (checkingPointX >= rc.x() + rc.width() - mXVert) && (checkingPointX <= rc.x() + rc.width() + mXVert)
			&& (checkingPointY >= rc.y() + mYVert)
			&& (checkingPointY <= rc.y() + rc.height() - mYVert);
}

bool BorderChecker::checkNoBorderX(const QPointF &point, qreal y) const
{
	const qreal checkingPointY = point.y();
	const QRectF rc = mNode->boundingRect();
	return (checkingPointY >= rc.y() + y) && (checkingPointY <= rc.y() + rc.height() - y);
}

bool BorderChecker::checkNoBorderY(const QPointF &point, qreal x) const
{
	const qreal checkingPointX = point.x();
	const QRectF rc = mNode->boundingRect();
	return (checkingPointX >= rc.x() + x) && (checkingPointX <= rc.x() + rc.width() - x);
}
