#include "migration/logEntries/logEntry.h"
#include "migration/logEntries/renameEntry.h"
#include "migration/logEntries/versionEntry.h"

using namespace qReal::migration;

QString LogEntry::toString() const
{
	return QString();
}

LogEntry * LogEntry::loadFromString(QString const &string)
{
	QStringList const components = string.split("@");
	if (components.empty()) {
		qDebug() << "incorrect log string";
		return new LogEntry();
	}

	if (components[0] == "version") {
		if (components.size() != 3) {
			qDebug() << "incorrect version entry";
			return new LogEntry();
		}

		bool ok = true;
		int const version = components[1].toInt(&ok);
		if (!ok) {
			qDebug() << "incorrect version entry";
			return new LogEntry();
		}

		return new VersionEntry(version, components[2]);
	} else if (components[0] == "rename") {
		if (components.size() != 5) {
			qDebug() << "incorrect rename entry";
			return new LogEntry();
		}

		QStringList const names = components[2].split(">");
		if (names.size() != 2) {
			qDebug() << "incorrect rename entry";
			return new LogEntry();
		}

		return new RenameEntry(Id::loadFromString(components[1]), Id::loadFromString(components[3].split("=")[1])
				, components[4].split("=")[1], names[0], names[1]);
	} else {
		qDebug() << "unsupported log entry type";
		return new LogEntry();
	}
}

void LogEntry::reverse(qrRepo::CommonRepoApi *repo) const
{
	Q_UNUSED(repo)
}
