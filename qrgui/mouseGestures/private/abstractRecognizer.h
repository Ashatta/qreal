#pragma once

#include <QtCore/QString>
#include <QtCore/QMap>
#include <QtWidgets/QWidget>
#include <QtGui/QMouseEvent>

#include <qrkernel/ids.h>

#include "geometricForms.h"
#include "pathCorrector.h"

namespace qReal {
namespace gestures {

class GesturesManager
{
public:
	virtual ~GesturesManager() {}
	virtual void initIdealGestures(QMap<QString, PathVector> const &objects) = 0;
	virtual void setKey(const PathVector &path) = 0;
	virtual qreal getMaxDistance(const QString &object) = 0;
	virtual qreal getDistance(const QString &object) = 0;
	virtual bool isMultistroke() = 0;
};

template <typename TKey>
class GesturesRecognizer : public GesturesManager
{
public:
	GesturesRecognizer(){}

	~GesturesRecognizer() override {}

	qreal getDistance(const QString &item) override
	{
		TKey key = mGestures[item];
		return getDistance(mKey, key);
	}

	void initIdealGestures(QMap<QString, PathVector> const &objects)
	{
		for (const QString &object : objects.keys()) {
			TKey key = getKey(objects[object]);
			mGestures.insert(object, key);
		}
	}

	void setKey(const PathVector &path)
	{
		mKey = getKey(path);
	}

	virtual qreal getMaxDistance(const QString &object) = 0;

protected:
	TKey mKey;
	virtual qreal getDistance(const TKey &key1, const TKey &key2) = 0;
	virtual TKey getKey(const PathVector &path) = 0;
	QMap<QString, TKey> mGestures;
	//maybe to do several lists for multistroke gestures
};

}
}
