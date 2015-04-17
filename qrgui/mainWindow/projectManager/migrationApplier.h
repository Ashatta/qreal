#pragma once

#include <qrgui/plugins/pluginManager/editorManagerInterface.h>
#include <qrgui/models/modelsInterface.h>

#include <qrutils/migration/migration.h>

namespace qReal {
namespace migration {

class MigrationApplier
{
public:
	static bool runUserMigrations(const EditorManagerInterface &editorManager, models::ModelsInterface *model
			, QWidget *window);

private:
	static void initTemporaryMigrationFiles(const Migration &migration);
};

}
}
