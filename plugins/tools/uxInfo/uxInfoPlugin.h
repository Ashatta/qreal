/* Copyright 2007-2015 QReal Research Group
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License. */

#pragma once

#include <qrgui/plugins/toolPluginInterface/toolPluginInterface.h>

#include "filterObject.h"

class QToolBar;
class QAction;

namespace uxInfo {

/// Logs user actions and collects statistics for future analisys.
class UXInfoPlugin : public QObject, public qReal::ToolPluginInterface
{
	Q_OBJECT
	Q_INTERFACES(qReal::ToolPluginInterface)
	Q_PLUGIN_METADATA(IID "uxInfo.UXInfoPlugin")

public:
	UXInfoPlugin();

	void init(qReal::PluginConfigurator const &configurator) override;
	QStringList defaultSettingsFiles() override;

private:
	void initSettingsUi(qReal::gui::PreferencesPage &behaviourPage);

	void startUsabilityTest();
	void finishUsabilityTest();

	void processEvent(QObject *obj, QEvent *e);

	QToolBar *mUsabilityTestingToolbar;  // Has ownership
	QAction *mStartTest;  // Has ownership via Qt parentship system
	QAction *mFinishTest;  // Has ownership via Qt parentship system
	bool mUsabilityTestingMode;
	FilterObject mFilterObject;
	int mStartTimestamp;
};

}
