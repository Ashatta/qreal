#pragma once

#include <QtCore/QFile>
#include <QtCore/QDir>
#include <QtCore/QTextStream>

#include "qrutils/utilsDeclSpec.h"

namespace utils {

/// Helper class for reading data from a file.
class QRUTILS_EXPORT InFile
{
public:
	/// Read all data from given file.
	/// @param fileName Name of the file to be read.
	/// @returns All contents of a file.
	static QString readAll(const QString &fileName);
};

}
