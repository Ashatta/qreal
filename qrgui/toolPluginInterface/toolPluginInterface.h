#pragma once

#include <QtCore/QObject>
#include <QtCore/QtPlugin>
#include <QtCore/QList>
#include <QtWidgets/QAction>

#include <qrrepo/repoControlInterface.h>
#include "dialogs/preferencesPages/preferencesPage.h"

#include "toolPluginInterface/customizer.h"
#include "toolPluginInterface/pluginConfigurator.h"
#include "toolPluginInterface/actionInfo.h"
#include "toolPluginInterface/hotKeyActionInfo.h"
#include "toolPluginInterface/projectConverter.h"

#include "toolPluginInterface/pluginInterface.h"

namespace qReal {

/// Base class for tool plugins, actually an interface with default implementations.
/// This is all that qrgui knows about tool plugins. Every tool plugin shall have
/// a class derived from this and override needed methods. Default implementations
/// are provided as "do nothing" behavior, so if a plugin doesn't need some features,
/// it shouldn't care about them.
class ToolPluginInterface : public PluginInterface
{
public:

	/// Shall be overriden in concrete plugin to initialize itself.
	/// @param configurator Object that contains all needed information for a plugin, like refs to models.
	virtual void init(PluginConfigurator const &configurator)
	{
		Q_UNUSED(configurator);
	}
};

}

Q_DECLARE_INTERFACE(qReal::ToolPluginInterface, "ru.tepkom.QReal.ToolPluginInterface/0.2")
