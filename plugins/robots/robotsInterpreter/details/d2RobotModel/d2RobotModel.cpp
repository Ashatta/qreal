#include "d2RobotModel.h"
#include "../tracer.h"
#include <qrkernel/settingsManager.h>
#include <qrutils/mathUtils/math.h>
#include <qrutils/mathUtils/gaussNoise.h>
#include <qrutils/mathUtils/vectorUtils.h>

using namespace qReal::interpreters::robots;
using namespace details;
using namespace d2Model;
using namespace mathUtils;

uint const black   = 0xFF000000;
uint const white   = 0xFFFFFFFF;
uint const red     = 0xFFFF0000;
uint const green   = 0xFF008000;
uint const blue    = 0xFF0000FF;
uint const yellow  = 0xFFFFFF00;
uint const cyan    = 0xFF00FFFF;
uint const magenta = 0xFFFF00FF;

unsigned const touchSensorPressedSignal = 1;
unsigned const touchSensorNotPressedSignal = 0;

qreal const spoilColorDispersion = 2.0;
qreal const spoilLightDispersion = 1.0;
qreal const spoilSonarDispersion = 1.5;
qreal const varySpeedDispersion = 0.0125;
qreal const percentSaltPepperNoise = 20.0;

D2RobotModel::D2RobotModel(QObject *parent)
	: QObject(parent)
	, mD2ModelWidget(NULL)
	, mMotorA(NULL)
	, mMotorB(NULL)
	, mMotorC(NULL)
	, mDisplay(new NxtDisplay)
	, mTimeline(new Timeline(this))
	, mNoiseGen()
	, mNeedSync(false)
	, mNeedSensorNoise(SettingsManager::value("enableNoiseOfSensors").toBool())
	, mNeedMotorNoise(SettingsManager::value("enableNoiseOfMotors").toBool())
	, mPos(QPointF(0,0))
	, mAngle(0)
	, mInertialMoment(100)
	, mFric(0)
	, mForceMoment(0)
	, mMass(1000)
	, mAngularVelocity(0)
{
	mNoiseGen.setApproximationLevel(SettingsManager::value("approximationLevel").toUInt());
	connect(mTimeline, SIGNAL(tick()), this, SLOT(recalculateParams()), Qt::UniqueConnection);
	connect(mTimeline, SIGNAL(nextFrame()), this, SLOT(nextFragment()), Qt::UniqueConnection);
	initPosition();
}

D2RobotModel::~D2RobotModel()
{
}

void D2RobotModel::initPosition()
{
	if (mMotorA) {
		delete mMotorA;
	}
	if (mMotorB) {
		delete mMotorB;
	}
	if (mMotorC) {
		delete mMotorC;
	}
	mMotorA = initMotor(robotWheelDiameterInPx / 2, 0, 0, 0, false);
	mMotorB = initMotor(robotWheelDiameterInPx / 2, 0, 0, 1, false);
	mMotorC = initMotor(robotWheelDiameterInPx / 2, 0, 0, 2, false);
	setBeep(0, 0);
	mPos = mD2ModelWidget ? mD2ModelWidget->robotPos() : QPointF(0, 0);
}

void D2RobotModel::clear()
{
	initPosition();
	mAngle = 0;
	mPos = QPointF(0,0);
}

D2RobotModel::Motor* D2RobotModel::initMotor(int radius, int speed, long unsigned int degrees, int port, bool isUsed)
{
	Motor *motor = new Motor();
	motor->motorFactor = 0;
	motor->radius = radius;
	motor->speed = speed;
	motor->degrees = degrees;
	motor->isUsed = isUsed;
	if (degrees == 0) {
		motor->activeTimeType = DoInf;
	} else {
		motor->activeTimeType = DoByLimit;
	}
	mMotors[port] = motor;
	mTurnoverMotors[port] = 0;
	return motor;
}

void D2RobotModel::setBeep(unsigned freq, unsigned time)
{
	mBeep.freq = freq;
	mBeep.time = time;
}

void D2RobotModel::setNewMotor(int speed, uint degrees, const int port)
{
	mMotors[port]->speed = speed;
	mMotors[port]->degrees = degrees;
	mMotors[port]->isUsed = true;
	if (degrees) {
		mMotors[port]->activeTimeType = DoByLimit;
	} else {
		mMotors[port]->activeTimeType = DoInf;
	}

	if (speed) {
		mMotors[port]->motorFactor = 0.5;
	}
}

int D2RobotModel::varySpeed(int const speed) const
{
	qreal const ran = mNoiseGen.generate(mNoiseGen.approximationLevel(), varySpeedDispersion);
	return truncateToInterval(-100, 100, round(speed * (1 + ran)));
}

void D2RobotModel::countMotorTurnover()
{
	foreach (Motor * const motor, mMotors) {
		int const port = mMotors.key(motor);
		qreal const degrees = Timeline::timeInterval * motor->spoiledSpeed * onePercentAngularVelocity;
		mTurnoverMotors[port] += degrees;
		if (motor->isUsed && (motor->activeTimeType == DoByLimit) && (mTurnoverMotors[port] >= motor->degrees)) {
			motor->speed = 0;
			motor->activeTimeType = End;
			emit d2MotorTimeout();
		}
	}
}

int D2RobotModel::readEncoder(int/*inputPort::InputPortEnum*/ const port) const
{
	return mTurnoverMotors[port];
}

void D2RobotModel::resetEncoder(int/*inputPort::InputPortEnum*/ const port)
{
	mTurnoverMotors[port] = 0;
}

SensorsConfiguration &D2RobotModel::configuration()
{
	return mSensorsConfiguration;
}

D2ModelWidget *D2RobotModel::createModelWidget()
{
	mD2ModelWidget = new D2ModelWidget(this, &mWorldModel, mDisplay);
	return mD2ModelWidget;
}

QPair<QPointF, qreal> D2RobotModel::countPositionAndDirection(robots::enums::inputPort::InputPortEnum const port) const
{
	QVector<SensorItem *> items = mD2ModelWidget->sensorItems();
	SensorItem *sensor = items[port];
	QPointF const position = sensor ? sensor->scenePos() : QPointF();
	qreal const direction = sensor ? items[port]->rotation() + mAngle : 0;
	return QPair<QPointF, qreal>(position, direction);
}

int D2RobotModel::readTouchSensor(robots::enums::inputPort::InputPortEnum const port)
{
	if (mSensorsConfiguration.type(port) != robots::enums::sensorType::touchBoolean
			&& mSensorsConfiguration.type(port) != robots::enums::sensorType::touchRaw)
	{
		return touchSensorNotPressedSignal;
	}
	QPair<QPointF, qreal> neededPosDir = countPositionAndDirection(port);
	QPointF sensorPosition(neededPosDir.first);
	qreal const width = sensorWidth / 2.0;
	QRectF const scanningRect = QRectF(
			sensorPosition.x() - width - touchSensorStrokeIncrement / 2.0
			, sensorPosition.y() - width - touchSensorStrokeIncrement / 2.0
			, 2 * width + touchSensorStrokeIncrement
			, 2 * width + touchSensorStrokeIncrement);

	QPainterPath sensorPath;
	sensorPath.addRect(scanningRect);
	bool const res = mWorldModel.checkCollision(sensorPath, touchSensorWallStrokeIncrement);

	return res ? touchSensorPressedSignal : touchSensorNotPressedSignal;
}

int D2RobotModel::readSonarSensor(robots::enums::inputPort::InputPortEnum const port) const
{
	QPair<QPointF, qreal> neededPosDir = countPositionAndDirection(port);
	int const res = mWorldModel.sonarReading(neededPosDir.first, neededPosDir.second);

	return mNeedSensorNoise ? spoilSonarReading(res) : res;
}

int D2RobotModel::spoilSonarReading(int const distance) const
{
	qreal const ran = mNoiseGen.generate(
			mNoiseGen.approximationLevel()
			, spoilSonarDispersion
			);

	return truncateToInterval(0, 255, round(distance + ran));
}

int D2RobotModel::readColorSensor(robots::enums::inputPort::InputPortEnum const port) const
{
	QImage const image = printColorSensor(port);
	QHash<uint, int> countsColor;

	uint const *data = reinterpret_cast<uint const *>(image.bits());
	int const n = image.byteCount() / 4;
	for (int i = 0; i < n; ++i) {
		uint const color = mNeedSensorNoise ? spoilColor(data[i]) : data[i];
		++countsColor[color];
	}

	switch (mSensorsConfiguration.type(port)) {
	case robots::enums::sensorType::colorFull:
		return readColorFullSensor(countsColor);
	case robots::enums::sensorType::colorNone:
		return readColorNoneSensor(countsColor, n);
	case robots::enums::sensorType::colorRed:
		return readSingleColorSensor(red, countsColor, n);
	case robots::enums::sensorType::colorGreen:
		return readSingleColorSensor(green, countsColor, n);
	case robots::enums::sensorType::colorBlue:
		return readSingleColorSensor(blue, countsColor, n);
	default:
		return 0;
	}
}

uint D2RobotModel::spoilColor(uint const color) const
{
	qreal const ran = mNoiseGen.generate(
			mNoiseGen.approximationLevel()
			, spoilColorDispersion
			);

	int r = round(((color >> 16) & 0xFF) + ran);
	int g = round(((color >> 8) & 0xFF) + ran);
	int b = round(((color >> 0) & 0xFF) + ran);
	int const a = (color >> 24) & 0xFF;

	r = truncateToInterval(0, 255, r);
	g = truncateToInterval(0, 255, g);
	b = truncateToInterval(0, 255, b);

	return ((r & 0xFF) << 16) + ((g & 0xFF) << 8) + (b & 0xFF) + ((a & 0xFF) << 24);
}

QImage D2RobotModel::printColorSensor(robots::enums::inputPort::InputPortEnum const port) const
{
	if (mSensorsConfiguration.type(port) == robots::enums::sensorType::unused) {
		return QImage();
	}

	QPair<QPointF, qreal> const neededPosDir = countPositionAndDirection(port);
	QPointF const position = neededPosDir.first;
	qreal const width = sensorWidth / 2.0;
	QRectF const scanningRect = QRectF(position.x() - width, position.y() - width
			, 2 * width, 2 * width);

	QImage image(scanningRect.size().toSize(), QImage::Format_RGB32);
	QPainter painter(&image);

	QBrush brush(Qt::SolidPattern);
	brush.setColor(Qt::white);
	painter.setBrush(brush);
	painter.setPen(QPen(Qt::white));
	painter.drawRect(scanningRect.translated(-scanningRect.topLeft()));

	bool const wasSelected = mD2ModelWidget->sensorItems()[port]->isSelected();
	mD2ModelWidget->setSensorVisible(port, false);
	mD2ModelWidget->scene()->render(&painter, QRectF(), scanningRect);
	mD2ModelWidget->setSensorVisible(port, true);
	mD2ModelWidget->sensorItems()[port]->setSelected(wasSelected);

	return image;
}

int D2RobotModel::readColorFullSensor(QHash<uint, int> const &countsColor) const
{
	if (countsColor.isEmpty()) {
		return 0;
	}

	QList<int> const values = countsColor.values();
	int maxValue = INT_MIN;
	foreach (int value, values) {
		if (value > maxValue) {
			maxValue = value;
		}
	}

	uint const maxColor = countsColor.key(maxValue);
	switch (maxColor) {
	case (black):
		Tracer::debug(tracer::enums::d2Model, "D2RobotModel::readColorFullSensor", "BLACK");
		return 1;
	case (red):
		Tracer::debug(tracer::enums::d2Model, "D2RobotModel::readColorFullSensor", "RED");
		return 5;
	case (green):
		Tracer::debug(tracer::enums::d2Model, "D2RobotModel::readColorFullSensor", "GREEN");
		return 3;
	case (blue) :
		Tracer::debug(tracer::enums::d2Model, "D2RobotModel::readColorFullSensor", "BLUE");
		return 2;
	case (yellow):
		Tracer::debug(tracer::enums::d2Model, "D2RobotModel::readColorFullSensor", "YELLOW");
		return 4;
	case (white):
		Tracer::debug(tracer::enums::d2Model, "D2RobotModel::readColorFullSensor", "WHITE");
		return 6;
	case (cyan):
		Tracer::debug(tracer::enums::d2Model, "D2RobotModel::readColorFullSensor", "CYAN");
		return 7;
	case (magenta):
		Tracer::debug(tracer::enums::d2Model, "D2RobotModel::readColorFullSensor", "MAGENTA");
		return 8;
	default:
		Tracer::debug(tracer::enums::d2Model, "D2RobotModel::readColorFullSensor", "Other Color");
		return 0;
	}
}

int D2RobotModel::readSingleColorSensor(uint color, QHash<uint, int> const &countsColor, int n) const
{
	return (static_cast<double>(countsColor[color]) / static_cast<double>(n)) * 100.0;
}

int D2RobotModel::readColorNoneSensor(QHash<uint, int> const &countsColor, int n) const
{
	double allWhite = static_cast<double>(countsColor[white]);

	QHashIterator<uint, int> i(countsColor);
	while(i.hasNext()) {
		i.next();
		uint const color = i.key();
		if (color != white) {
			int const b = (color >> 0) & 0xFF;
			int const g = (color >> 8) & 0xFF;
			int const r = (color >> 16) & 0xFF;
			qreal const k = qSqrt(static_cast<qreal>(b * b + g * g + r * r)) / 500.0;
			allWhite += static_cast<qreal>(i.value()) * k;
		}
	}

	return (allWhite / static_cast<qreal>(n)) * 100.0;
}

int D2RobotModel::readLightSensor(robots::enums::inputPort::InputPortEnum const port) const
{
	// Must return 1023 on white and 0 on black normalized to percents
	// http://stackoverflow.com/questions/596216/formula-to-determine-brightness-of-rgb-color

	QImage const image = printColorSensor(port);
	if (image.isNull()) {
		return 0;
	}

	uint sum = 0;
	uint const *data = reinterpret_cast<uint const *>(image.bits());
	int const n = image.byteCount() / 4;

	for (int i = 0; i < n; ++i) {
		int const color = mNeedSensorNoise ? spoilLight(data[i]) : data[i];
		int const b = (color >> 0) & 0xFF;
		int const g = (color >> 8) & 0xFF;
		int const r = (color >> 16) & 0xFF;
		// brightness in [0..256]
		int const brightness = 0.2126 * r + 0.7152 * g + 0.0722 * b;

		sum += 4 * brightness; // 4 = max sensor value / max brightness value
	}
	qreal const rawValue = sum / n; // Average by whole region
	return rawValue * 100 / maxLightSensorValur; // Normalizing to percents
}

uint D2RobotModel::spoilLight(uint const color) const
{
	qreal const ran = mNoiseGen.generate(
			mNoiseGen.approximationLevel()
			, spoilLightDispersion
			);

	if (ran > (1.0 - percentSaltPepperNoise / 100.0)) {
		return white;
	} else if (ran < (-1.0 + percentSaltPepperNoise / 100.0)) {
		return black;
	}

	return color;
}

void D2RobotModel::startInit()
{
	initPosition();
	mTimeline->start();
}

void D2RobotModel::startInterpretation()
{
	startInit();
	mD2ModelWidget->startTimelineListening();
}

void D2RobotModel::stopRobot()
{
	mMotorA->speed = 0;
	mMotorB->speed = 0;
	mMotorC->speed = 0;
	mD2ModelWidget->stopTimelineListening();
}

void D2RobotModel::countBeep()
{
	if (mBeep.time > 0) {
		mD2ModelWidget->drawBeep(true);
		mBeep.time -= Timeline::frameLength;
	} else {
		mD2ModelWidget->drawBeep(false);
	}
}

void D2RobotModel::setVelocity(QVector2D const &velocity)
{
	QVector2D result = velocity;
	qreal const oldVelocity = fullSpeed();
	if (result.length() > oldVelocity) {
		qreal const x = oldVelocity / result.length();
		result *= x;
	}

	mVelocity = result;
}

qreal D2RobotModel::inertialMoment() const
{
	return mInertialMoment;
}

void D2RobotModel::setForce(QVector2D const &force)
{
	mForce = force;
}

void D2RobotModel::setForceMoment(qreal forceMoment)
{
	mForceMoment = forceMoment;
}

qreal D2RobotModel::fullSpeed() const
{
	return mFullSpeed;
}

qreal D2RobotModel::fullSpeedA() const
{
	return mFullSpeedA;
}

qreal D2RobotModel::fullSpeedB() const
{
	return mFullSpeedB;
}

QLineF D2RobotModel::nearRobotLine(WallItem const &wall, QPointF const &p)
{
	QList<qreal> len;
	qreal min=0;
	for (int i = 0; i < 4; ++i) {
		QLineF l;
		l= wall.line(i);
		QPointF normPoint = normalPoint(l.p1(), l.p2(), p);
		QLineF k (p, normPoint);
		len.push_back( k.length());
		if (min == 0){
			min = k.length();
		}
		if (k.length() < min){
			min = k.length();
		}
	}

	int j = len.indexOf(min);
	return wall.line(j);
}

QPointF D2RobotModel::normalPoint(QPointF const &a, QPointF const &b, QPointF const &c)
{
	qreal const x1 = a.x();
	qreal const y1 = a.y();
	qreal const x2 = b.x();
	qreal const y2 = b.y();
	qreal const x3 = c.x();
	qreal const y3 = c.y();

	if (x1 == x2) {
		return QPointF(x2, y3);
	}

	qreal const x = (x1 * Math::sqr(y2 - y1) + x3 * Math::sqr(x2 - x1) + (x2 - x1) * (y2 - y1) * (y3 - y1))
			/ (Math::sqr(y2 - y1) + Math::sqr(x2 - x1));
	qreal const y = ((y2 - y1) * (x - x1) / (x2 - x1)) + y1;

	return QPointF(x, y);
}

void D2RobotModel::updateCoord()
{
	qreal const x = mPos.x() + 25;
	qreal const y = mPos.y() + 25;
	int const size = 50;
	qreal const nnx = cos(mAngle * M_PI / 180) * (size / 2);
	qreal const nny = sin(mAngle * M_PI / 180) * (size / 2);

	mVertices[0] = QPointF(x + nnx + nny, y + nny - nnx);
	mVertices[1] = QPointF(x + nnx - nny, y + nny + nnx);
	mVertices[2] = QPointF(x - nnx - nny, y - nny + nnx);
	mVertices[3] = QPointF(x - nnx + nny, y - nny - nnx);

	for (int i = 0; i < 4; ++i) {
		mBorders[i] = QLineF(mVertices[i], mVertices[(i + 1) % 4]);
	}
}

bool D2RobotModel::isRobotWallCollision(WallItem const &wall)
{
	QPainterPath const boundingRegion = mD2ModelWidget->robotBoundingPolygon(mPos, mAngle);
	return boundingRegion.intersects(wall.path());
}

bool D2RobotModel::wallContainsRobotPoints(WallItem const &wall)
{
	return ((wall.contains(mVertices[0]))||(wall.contains(mVertices[1]))||(wall.contains(mVertices[2]))||(wall.contains(mVertices[3])));
}

void D2RobotModel::findCollision(WallItem const &wall)
{
	mInsidePoints.clear();
	QPainterPath const boundingRegion = mD2ModelWidget->robotBoundingPolygon(mPos, mAngle);
	if (isRobotWallCollision(wall)) {
		if (wall.isCircle()) {
			if (wallContainsRobotPoints(wall)) {
				for(int i = 0; i < 4; i++) {
					if (wall.contains(mVertices[i])) {
						QLineF border = tangentLine(wall);
						QPointF normPoint = normalPoint(border.p1(), border.p2(), mVertices[i]);
						QVector2D n (mVertices[i] - normPoint);
						n = n.normalized();
						QVector2D V = mVelocity;
						qreal k = mathUtils::VectorUtils::scalarProduct(V, n);
						QVector2D V1 (V - n * k);
						setVelocity(V1);
						setWall(i, &wall);
					}
				}
			} else {
				for(int i = 0; i < 4; i++) {
					QLineF l = mBorders[i];
					QPainterPath p(l.p1());
					p.lineTo(l.p2());
					if (wall.path().intersects(p)) {
						QLineF border = interWallLine(wall);
						QPointF normPoint = normalPoint(border.p1(), border.p2(), wall.center());
						QVector2D n (normPoint - wall.center());
						n = n.normalized();
						QPointF p = QPointF(wall.center() + n.toPointF() * wall.width());
						QVector2D V1 (0,0);
						setVelocity(V1);
						mInsidePoints.push_back(p);
						mAngularVelocity=0;
						setEdgeWall(i, &wall);
					}
				}
			}
		} else {
			for(int i = 0; i < 4; i++)
			{
				QPointF new_p = mVertices[i];
				if (wall.path().contains(new_p)) {
					if(mRobotWalls[i] == NULL) {
						QLineF border = interRobotLine(wall);
						QPointF normPoint = normalPoint(border.p1(), border.p2(), mVertices[i]);
						QVector2D n (mVertices[i] - normPoint);
						n = n.normalized();
						QVector2D V = mVelocity;
						qreal k = mathUtils::VectorUtils::scalarProduct(V, n);
						QVector2D V1 (V - n * k);
						setVelocity(V1);
					}
					setWall(i, &wall);
				} else {
					if (mRobotWalls[i] == &wall){
						setWall(i, NULL);
					}
					QPointF p = wall.point(i);
					if (boundingRegion.contains(p)) {
						QLineF border = interWallLine(wall);
						QPointF normPoint = normalPoint(border.p1(), border.p2(), mVertices[i]);
						QVector2D n (p - normPoint);
						n = n.normalized();
						QVector2D V1 (0,0);
						setVelocity(V1);
						mInsidePoints.push_back(p);
						mAngularVelocity=0;
					} else {
						if (mRobotEdgeWalls[i] == &wall){
							setEdgeWall(i, NULL);
						}
					}
				}
			}
		}
	} else {
		for (int i = 0; i < 4; i++) {

			if (mRobotWalls[i] == &wall) {
				mRobotWalls[i] = NULL;
				mAngularVelocity = 0;
			}
		}
	}
}

void D2RobotModel::countNewCoord()
{
	updateCoord();

	Motor *motor1 = mMotorA;
	Motor *motor2 = mMotorB;
	Motor *motor3 = mMotorC;

	if (mMotorC->isUsed) {
		if (!mMotorA->isUsed) {
			motor1 = mMotorC;
		} else if (!mMotorB->isUsed) {
			motor2 = mMotorC;
		}
	}

	motor1->spoiledSpeed= mNeedMotorNoise ? varySpeed(motor1->speed) : motor1->speed;
	motor2->spoiledSpeed = mNeedMotorNoise ? varySpeed(motor2->speed) : motor2->speed;

	mFullSpeedA = motor1->spoiledSpeed * 2 * M_PI * motor1->radius * onePercentAngularVelocity / 360;
	mFullSpeedB = motor2->spoiledSpeed * 2 * M_PI * motor2->radius * onePercentAngularVelocity / 360;
	mFullSpeed = motor3->speed * 2 * M_PI * motor3->radius * onePercentAngularVelocity / 360;

	setForce(QVector2D());
	setForceMoment(0);
	qreal rotationalFricFactor = mVelocity.length() * 1500;
	qreal angularVelocityFricFactor = fabs(mAngularVelocity * 1000);
	QVector2D direction(cos(mAngle * M_PI / 180), sin(mAngle * M_PI / 180));
	qreal oldVelocityModule = fullSpeed();

	qreal tmpA = (fullSpeedA() - mathUtils::VectorUtils::scalarProduct(mVelocityA, direction)) * mMotorB->motorFactor;
	qreal tmpB = (fullSpeedB() - mathUtils::VectorUtils::scalarProduct(mVelocityB, direction)) * mMotorC->motorFactor;
	qreal tmp2 = (oldVelocityModule - mathUtils::VectorUtils::scalarProduct(mVelocity, direction)) * mMotorA->motorFactor;

	QPointF p0 (mPos.rx() + 25, mPos.ry() + 25);

	mForce = direction * tmp2;
	mForce += direction * tmpA;
	mForce += direction * tmpB;

	qreal const forceMomentA = mathUtils::VectorUtils::vectorProduct(direction * tmpA, QVector2D(mVertices[0] - p0));
	qreal const forceMomentB = mathUtils::VectorUtils::vectorProduct(direction * tmpB, QVector2D(mVertices[1] - p0));

	mForceMoment -= forceMomentA;
	mForceMoment -= forceMomentB;

	calculateForceMoment();

	QVector2D V1 = mVelocity + mForce / mMass * Timeline::timeInterval;
	setVelocity(V1);
	mAngularVelocity += mForceMoment /  inertialMoment() * Timeline::timeInterval;
	qreal fric = angularVelocityFricFactor /  inertialMoment() * Timeline::timeInterval;
	qreal tmpAngVel = mAngularVelocity;

	mAngularVelocity -= fric * Math::sign(mAngularVelocity);

	if (tmpAngVel * mAngularVelocity <= 0) {
		mAngularVelocity = 0;
	}

	QVector2D rotationalFrictionF(-direction.y(), direction.x());
	rotationalFrictionF.normalize();
	QVector2D normalizedVelocity = mVelocity;
	if (!normalizedVelocity.isNull()) {
		normalizedVelocity.normalize();
	}

	qreal const sinus = mathUtils::VectorUtils::vectorProduct(normalizedVelocity, rotationalFrictionF);
	rotationalFrictionF = rotationalFrictionF *(sinus * rotationalFricFactor);
	if (mathUtils::VectorUtils::scalarProduct(rotationalFrictionF, mVelocity) > 0) {
		rotationalFrictionF = -rotationalFrictionF;
	}

	QVector2D newV = mVelocity + rotationalFrictionF / mMass * Timeline::timeInterval;
	qreal sc_1 = mathUtils::VectorUtils::scalarProduct(newV, rotationalFrictionF);
	if (sc_1 > 0) {
		qreal sc_2 = -mathUtils::VectorUtils::scalarProduct(mVelocity, rotationalFrictionF);
		qreal dt_tmp = Timeline::timeInterval *sc_2/(sc_2 + sc_1);
		QVector2D V1 = mVelocity + rotationalFrictionF / mMass * dt_tmp;
		setVelocity(V1);
	}  else {
		setVelocity(newV);
	}

	if (mVelocity.length() > oldVelocityModule) {
		newV = mVelocity.normalized() * oldVelocityModule;
		setVelocity(newV);
	}
}


void D2RobotModel::calculateForceMoment()
{
	QPointF p0 (mPos.rx() + 25, mPos.ry() + 25);
	QVector2D napr (cos(mAngle * M_PI / 180), sin (mAngle * M_PI / 180));
	for (int i = 0; i < 4; i++) {
		if (mRobotEdgeWalls[i] != NULL) {
			for (int j = 0; j < mInsidePoints.length(); j++) {
				QVector2D tmp(mInsidePoints.at(j) - p0);
				QVector2D F_norm = mForce;
				F_norm *= -1;
				qreal a = mathUtils::VectorUtils::vectorProduct(F_norm, tmp);
				mForceMoment -= a;
			}
		}
	}

	for (int i = 0; i < 4; i++) {
		if (mRobotWalls[i] != NULL){
			QVector2D tmp(mVertices[i] - p0);
			QLineF bord = nearRobotLine(*mRobotWalls[i], mVertices[i]);
			qreal ang = bord.angle();
			QVector2D vectorParalStene;
			vectorParalStene = QVector2D(cos(ang*M_PI/180),-sin(ang*M_PI/180));
			qreal scpr = mathUtils::VectorUtils::scalarProduct(vectorParalStene, napr);
			if (scpr < 0) {
				vectorParalStene *= (-1);
			}

			vectorParalStene = vectorParalStene.normalized();
			QVector2D F_norm = mForce;
			double sc1 = mathUtils::VectorUtils::scalarProduct(mForce, vectorParalStene);
			F_norm -= vectorParalStene * sc1;
			F_norm *= -1;
			QVector2D F_fr_wall;
			vectorParalStene *= -1;
			F_fr_wall = vectorParalStene;
			F_fr_wall *= F_norm.length() * 0.2;
			mForce += F_norm;
			mForce += F_fr_wall;
			setForceMoment(mForceMoment - mathUtils::VectorUtils::vectorProduct(F_fr_wall, tmp));
			setForceMoment(mForceMoment - mathUtils::VectorUtils::vectorProduct(F_norm , tmp));
		}
	}
}

void D2RobotModel::getRobotFromWall(WallItem const &wall, int index)
{
	QPainterPath boundingRegion = mD2ModelWidget->robotBoundingPolygon(mPos, mAngle);
	if (isRobotWallCollision(wall)) {
		if (wall.isCircle()) {
			for (int i = 0; i < 4; ++i) {
				if (isCollision(wall, i)) {
					QVector2D n(robotPos() - wall.center());
					n = n.normalized();
					QPointF pntInter = wall.center() + n.toPointF()*wall.width();
					QPointF p(pntInter - mVertices[i]);
					mPos = (robotPos() + p);
					return;
				}
			}
			QLineF border = interWallLine(wall);
			QPointF normPoint = normalPoint(border.p1(), border.p2(), wall.center());
			QVector2D n (normPoint - wall.center());
			n = n.normalized();
			QPointF p = QPointF(wall.center() + n.toPointF() * wall.width());
			mPos = robotPos() + p - normPoint;
		} else {
			if (boundingRegion.intersects(wall.path())){
				QPointF p = mVertices[index];
				QLineF border = nearRobotLine(wall, p);
				QPointF pntIntersect = normalPoint(border.p1(), border.p2(), p);
				QPointF k;
				k = pntIntersect -= p;
				QPointF curPos = robotPos();
				mPos = (curPos+=k);
				updateCoord();
			}
		}
	}
}

void D2RobotModel::getEdgeRobotFromWall(WallItem const &wall, int index)
{
	QPainterPath const boundingRegion = mD2ModelWidget->robotBoundingPolygon(mPos, mAngle);
	if (boundingRegion.intersects(wall.path())) {
		if (wall.isCircle()) {
			QLineF l = interWallLine(wall);
			QLineF l2 = intersectRobotLine(wall);
			QPointF pntIntersect = normalPoint(l.p1(), l.p2(), wall.center());
			QPointF pntIntersect2 = normalPoint(l2.p1(), l2.p2(), wall.center());
			QVector2D n(pntIntersect - wall.center());
			QVector2D n2(pntIntersect2 - wall.center());
			n = n - n2;
			mPos += n.toPointF();
			return;
		} else {
			QLineF l = mBorders[index];
			for (int i = 0; i<4; i++)
			{
				QPointF p = wall.point(i);
				if (boundingRegion.contains(p)){
					QPointF pntIntersect = normalPoint(l.p1(), l.p2(), p);
					QPointF k (p - pntIntersect);
					QPointF curPos = robotPos();
					mPos = (curPos+=k);
					updateCoord();
				}
			}
		}
	}
}

QLineF D2RobotModel::tangentLine(WallItem const &wall)
{
	QVector2D n(robotPos() - wall.center());
	QLineF lineN(robotPos(), wall.center());
	n = n.normalized();
	QPointF pntInter = wall.center() + n.toPointF()*wall.width();
	lineN = lineN.normalVector();
	QPointF p2(lineN.p1() - lineN.p2());
	return QLineF(pntInter, pntInter + p2);
}


QLineF D2RobotModel::interRobotLine(WallItem const &wall)
{
	QPainterPath const boundingRegion = mD2ModelWidget->robotBoundingPolygon(mPos, mAngle);
	QLineF tmpLine;
	for (int i = 0; i < 4; ++i) {
		QLineF l = wall.line(i);
		QPainterPath p(l.p1());
		p.lineTo(l.p2());
		if (boundingRegion.intersects(p)) {
			tmpLine = l;
		}
	}
	return tmpLine;
}

QLineF D2RobotModel::intersectRobotLine(WallItem const &wall)
{
	for (int i = 0; i < 4; ++i) {
		if (isEdgeCollision(wall, i)) {
			return mBorders[i];
		}
	}

	return QLineF();
}

QLineF D2RobotModel::interWallLine(WallItem const &wall)
{
	if (wall.isCircle()) {
		for (int i = 0; i < 4; ++i) {
			if (isEdgeCollision(wall, i)) {
				QLineF l = mBorders[i];
				QPointF pntIntersect = normalPoint(l.p1(), l.p2(), wall.center());
				QVector2D n(pntIntersect - wall.center());
				QLineF lineN(pntIntersect , wall.center());
				n = n.normalized();
				QPointF pntInter = wall.center() + n.toPointF() * wall.width();
				lineN = lineN.normalVector();
				QPointF p2(lineN.p1() - lineN.p2());
				return QLineF(pntInter, pntInter + p2);
			}
		}
	} else {
		QLineF tmpLine;
		for (int i = 0; i < 4; ++i) {
			if (isEdgeCollision(wall, i)) {
				tmpLine = mBorders[i];
				setEdgeWall(i, &wall);
			}
		}

		return tmpLine;
	}

	return QLineF();
}

bool D2RobotModel::isCollision(WallItem const &wall, int i)
{
	return wall.path().contains(mVertices[i]);
}

bool D2RobotModel::isEdgeCollision(WallItem const &wall, int i)
{
	QPainterPath path(mBorders[i].p1());
	path.lineTo(mBorders[i].p2());
	return wall.path().intersects(path);
}

void D2RobotModel::nextStep()
{
	QVector2D curPos(mPos);
	curPos += mVelocity * Timeline::timeInterval;
	mPos = QPointF(curPos.x(), curPos.y());
	qreal timeInterval = Timeline::timeInterval;
	mAngle += mAngularVelocity * timeInterval;
	updateCoord();
}

void D2RobotModel::recalculateParams()
{
	// do nothing until robot gets back on the ground
	if (!mD2ModelWidget->isRobotOnTheGround()) {
		mNeedSync = true;
		return;
	}

	synchronizePositions();
	for (int i = 0; i < 4; ++i) {
		setWall(i, NULL);
		setEdgeWall(i, NULL);
	}

	for (int i = 0; i < mWorldModel.wallsCount(); ++i) {
		findCollision(*(mWorldModel.wallAt(i)));
		updateCoord();
	}

	countNewCoord();
	getFromWalls();
	nextStep();
	countMotorTurnover();
}

void D2RobotModel::getFromWalls()
{
	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < mWorldModel.wallsCount(); ++j) {
			if (isCollision(*(mWorldModel.wallAt(j)), i)) {
				getRobotFromWall(*(mWorldModel.wallAt(j)), i);
			}

			if (isEdgeCollision(*(mWorldModel.wallAt(j)), i)) {
				if (mRobotEdgeWalls[i]) {
					getEdgeRobotFromWall(*(mWorldModel.wallAt(j)), i);
				}
			}
		}
	}
}

void D2RobotModel::setWall(int index, WallItem const *wall)
{
	mRobotWalls[index] = wall;
}

void D2RobotModel::setEdgeWall(int index, WallItem const *wall)
{
	mRobotEdgeWalls[index] = wall;
}

void D2RobotModel::nextFragment()
{
	if (!mD2ModelWidget->isRobotOnTheGround()) {
		return;
	}
	synchronizePositions();
	countBeep();
	mD2ModelWidget->draw(mPos, mAngle);
	mNeedSync = true;
}

void D2RobotModel::synchronizePositions()
{
	if (mNeedSync) {
		mPos = mD2ModelWidget->robotPos();
		mNeedSync = false;
	}
}

void D2RobotModel::showModelWidget()
{
	mD2ModelWidget->init(true);
}

void D2RobotModel::setRotation(qreal angle)
{
	mPos = mD2ModelWidget ? mD2ModelWidget->robotPos() : QPointF(0, 0);
	mAngle = fmod(angle, 360);
	mD2ModelWidget->draw(mPos, mAngle);
}

double D2RobotModel::rotateAngle() const
{
	return mAngle;
}

void D2RobotModel::setSpeedFactor(qreal speedMul)
{
	mSpeedFactor = speedMul;
	mTimeline->setSpeedFactor(speedMul);
}

void D2RobotModel::setRobotPos(QPointF const &newPos)
{
	mPos = newPos;
	mD2ModelWidget->draw(mPos, mAngle);
}

QPointF D2RobotModel::robotPos()
{
	return mPos;
}

void D2RobotModel::serialize(QDomDocument &target)
{
	QDomElement robot = target.createElement("robot");
	robot.setAttribute("position", QString::number(mPos.x()) + ":" + QString::number(mPos.y()));
	robot.setAttribute("direction", mAngle);
	configuration().serialize(robot, target);
	target.firstChildElement("root").appendChild(robot);
}

void D2RobotModel::deserialize(QDomElement const &robotElement)
{
	QString const positionStr = robotElement.attribute("position", "0:0");
	QStringList const splittedStr = positionStr.split(":");
	qreal const x = static_cast<qreal>(splittedStr[0].toDouble());
	qreal const y = static_cast<qreal>(splittedStr[1].toDouble());
	mPos = QPointF(x, y);
	mAngle = robotElement.attribute("direction", "0").toDouble();
	configuration().deserialize(robotElement);
	mNeedSync = false;
	nextFragment();
}

Timeline *D2RobotModel::timeline() const
{
	return mTimeline;
}

details::NxtDisplay *D2RobotModel::display()
{
	return mDisplay;
}

void D2RobotModel::setNoiseSettings()
{
	mNeedSensorNoise = SettingsManager::value("enableNoiseOfSensors").toBool();
	mNeedMotorNoise = SettingsManager::value("enableNoiseOfMotors").toBool();
	mNoiseGen.setApproximationLevel(SettingsManager::value("approximationLevel").toUInt());
}

int D2RobotModel::truncateToInterval(int const a, int const b, int const res) const
{
	return (res >= a && res <= b) ? res : (res < a ? a : b);
}

