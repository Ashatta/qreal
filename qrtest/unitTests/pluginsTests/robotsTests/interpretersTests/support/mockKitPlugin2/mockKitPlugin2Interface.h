#pragma once

#include <QtCore/QObject>

#include <interpreterBase/kitPluginInterface.h>
#include <interpreterBase/robotModel/commonRobotModel.h>

namespace qrTest {
namespace mockKitPlugin2 {

class MockKitPlugin2Interface : public QObject, public interpreterBase::KitPluginInterface
{
	Q_OBJECT
	Q_INTERFACES(interpreterBase::KitPluginInterface)
	Q_PLUGIN_METADATA(IID "mockKitPlugin2.MockKitPlugin2Interface")

public:
	QString kitId() const override;

	QString friendlyKitName() const override;

	QList<interpreterBase::robotModel::RobotModelInterface *> robotModels() override;

	// Transfers ownership.
	interpreterBase::AdditionalPreferences *settingsWidget() override;

	interpreterBase::blocksBase::BlocksFactoryInterface *blocksFactoryFor(
			interpreterBase::robotModel::RobotModelInterface const *model) override;

	QList<qReal::ActionInfo> customActions() override;

	QList<qReal::HotKeyActionInfo> hotKeyActions() override;

	QIcon iconForFastSelector(interpreterBase::robotModel::RobotModelInterface const &robotModel) const override;
};

}
}
