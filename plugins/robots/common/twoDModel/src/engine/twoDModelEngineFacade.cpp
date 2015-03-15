#include "twoDModel/engine/twoDModelEngineFacade.h"

#include "twoDModel/engine/view/d2ModelWidget.h"
#include "model/model.h"
#include "twoDModelEngineApi.h"

using namespace twoDModel::engine;

TwoDModelEngineFacade::TwoDModelEngineFacade(twoDModel::robotModel::TwoDRobotModel &robotModel)
	: mRobotModelName(robotModel.name())
	, mTwoDModelActionInfo(
			new QAction(QIcon(":/icons/2d-model.svg"), QObject::tr("2d model"), nullptr)
			, "interpreters"
			, "tools")
	, mModel(new model::Model())
	, mView(new view::D2ModelWidget(*mModel.data()))
	, mApi(new TwoDModelEngineApi(*mModel.data(), *mView.data()))
{
	mModel.data()->addRobotModel(robotModel);
	connect(mTwoDModelActionInfo.action(), &QAction::triggered, mView.data(), &view::D2ModelWidget::init);

	connect(mView.data(), &view::D2ModelWidget::runButtonPressed, this, &TwoDModelEngineFacade::runButtonPressed);
	connect(mView.data(), &view::D2ModelWidget::stopButtonPressed, this, &TwoDModelEngineFacade::stopButtonPressed);
	connect(mView.data(), &view::D2ModelWidget::widgetClosed, this, &TwoDModelEngineFacade::stopButtonPressed);
}

TwoDModelEngineFacade::~TwoDModelEngineFacade()
{
}

void TwoDModelEngineFacade::init(const kitBase::EventsForKitPluginInterface &eventsForKitPlugin
		, const qReal::SystemEvents &systemEvents
		, qReal::GraphicalModelAssistInterface &graphicalModel
		, qReal::LogicalModelAssistInterface &logicalModel
		, qReal::gui::MainWindowInterpretersInterface &interpretersInterface
		, kitBase::InterpreterControlInterface &interpreterControl)
{
	mModel->init(*interpretersInterface.errorReporter(), interpreterControl);

	const auto onActiveTabChanged = [this, &graphicalModel, &logicalModel, &interpretersInterface] (const qReal::Id &id)
	{
		mView->setEnabled(!id.isNull());
		const qReal::Id logicalId = graphicalModel.logicalId(id);
		const QString xml = logicalId.isNull()
				? QString()
				: logicalModel.propertyByRoleName(logicalId, "worldModel").toString();
		QDomDocument worldModel;
		QString errorMessage;
		int errorLine, errorColumn;
		if (!xml.isEmpty() && !worldModel.setContent(xml, &errorMessage, &errorLine, &errorColumn)) {
			interpretersInterface.errorReporter()->addError(QString("%1:%2: %3")
					.arg(QString::number(errorLine), QString::number(errorColumn), errorMessage));
		}

		mView->loadXml(worldModel);
	};

	auto connectTwoDModel = [this, &eventsForKitPlugin, &interpreterControl]()
	{
		connect(&eventsForKitPlugin, &kitBase::EventsForKitPluginInterface::interpretationStarted
				, this, &twoDModel::TwoDModelControlInterface::onStartInterpretation
				, Qt::UniqueConnection);

		connect(&eventsForKitPlugin, &kitBase::EventsForKitPluginInterface::interpretationStopped
				, this, &twoDModel::TwoDModelControlInterface::onStopInterpretation
				, Qt::UniqueConnection);

		connect(this, &twoDModel::TwoDModelControlInterface::runButtonPressed
				, &interpreterControl, &kitBase::InterpreterControlInterface::interpret
				, Qt::UniqueConnection);

		connect(this, &twoDModel::TwoDModelControlInterface::stopButtonPressed
				, &interpreterControl, &kitBase::InterpreterControlInterface::stopRobot
				, Qt::UniqueConnection);
	};

	auto disconnectTwoDModel = [this, &eventsForKitPlugin, &interpreterControl]()
	{
		disconnect(&eventsForKitPlugin, &kitBase::EventsForKitPluginInterface::interpretationStarted
				, this, &twoDModel::TwoDModelControlInterface::onStartInterpretation);

		disconnect(&eventsForKitPlugin, &kitBase::EventsForKitPluginInterface::interpretationStopped
				, this, &twoDModel::TwoDModelControlInterface::onStopInterpretation);

		disconnect(this, &twoDModel::TwoDModelControlInterface::runButtonPressed
				, &interpreterControl, &kitBase::InterpreterControlInterface::interpret);

		disconnect(this, &twoDModel::TwoDModelControlInterface::stopButtonPressed
				, &interpreterControl, &kitBase::InterpreterControlInterface::stopRobot);
	};

	connect(&systemEvents, &qReal::SystemEvents::activeTabChanged, onActiveTabChanged);

	connect(mModel.data(), &model::Model::modelChanged, [this, &graphicalModel, &logicalModel
			, &interpreterControl, &interpretersInterface] (const QDomDocument &xml) {
				const qReal::Id logicalId = graphicalModel.logicalId(interpretersInterface.activeDiagram());
				if (!logicalId.isNull() && logicalId != qReal::Id::rootId()) {
					logicalModel.setPropertyByRoleName(logicalId, xml.toString(4), "worldModel");
				}
	});

	connect(&systemEvents, &qReal::SystemEvents::closedMainWindow, [=](){ mView.reset(); });

	connect(&eventsForKitPlugin
			, &kitBase::EventsForKitPluginInterface::robotModelChanged
			, [this, connectTwoDModel, disconnectTwoDModel](const QString &modelName) {
				const bool isCurrentModel = modelName == mRobotModelName;
				showTwoDModelWidgetActionInfo().action()->setVisible(isCurrentModel);
				if (isCurrentModel) {
					connectTwoDModel();
				} else {
					disconnectTwoDModel();
					mView->close();
				}
			}
			);
}

qReal::ActionInfo &TwoDModelEngineFacade::showTwoDModelWidgetActionInfo()
{
	return mTwoDModelActionInfo;
}

kitBase::DevicesConfigurationProvider &TwoDModelEngineFacade::devicesConfigurationProvider()
{
	return *mView.data();
}

TwoDModelEngineInterface &TwoDModelEngineFacade::engine()
{
	return *mApi.data();
}

void TwoDModelEngineFacade::onStartInterpretation()
{
	mModel->timeline().start();
}

void TwoDModelEngineFacade::onStopInterpretation()
{
	mModel->timeline().stop();
}
