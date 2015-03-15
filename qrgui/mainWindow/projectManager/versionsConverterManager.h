#pragma once

#include "models/logicalModelAssistApi.h"
#include "plugins/toolPluginInterface/projectConverter.h"

namespace qReal {

class MainWindow;

/// Performs validation and convertion of models due to editor versions that created them.
/// @todo: This class is not implemented at the moment.
/// It only displays error message if versions mismatch detected.
class VersionsConverterManager
{
public:
	explicit VersionsConverterManager(MainWindow &mainWindow);

	/// Performs validation and convertion of models due to editor versions that created them.
	bool validateCurrentProject();

private:
	bool convertProject(const Version &enviromentVersion
			, const Version &saveVersion
			, QList<ProjectConverter> const &converters);

	void displayTooOldEnviromentError(const Version &saveVersion);
	void displayCannotConvertError();
	void displayTooOldSaveError(const Version &saveVersion);

	void showError(const QString &errorMessage);

	MainWindow &mMainWindow;
};

}
