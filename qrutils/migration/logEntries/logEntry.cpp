#include "migration/logEntries/logEntry.h"
#include "migration/logEntries/renameEntry.h"
#include "migration/logEntries/versionEntry.h"
#include "migration/logEntries/changePropertyEntry.h"
#include "migration/logEntries/createElementEntry.h"
#include "migration/logEntries/removeElementEntry.h"

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
	} else if (components[0] == "changeProperty") {
		if (components.size() != 5) {
			qDebug() << "incorrect change property entry";
			return new LogEntry();
		}

		return new ChangePropertyEntry(Id::loadFromString(components[1]), components[2], components[3], components[4]);
	} else if (components[0] == "createElement") {
		if (components.size() != 2) {
			qDebug() << "incorrect create element entry";
			return new LogEntry();
		}

		return new CreateElementEntry(Id::loadFromString(components[1]));
	} else if (components[0] == "removeElement") {
		if (components.size() != 4 && components.size() != 5) {
			qDebug() << "incorrect remove element entry";
			return new LogEntry();
		}\

		QMap<QString, QVariant> properties;
		for (const QString property : components[2].split('|')) {
			QStringList propertyParts = property.split('>');
			if (propertyParts.size() != 2) {
				qDebug() << "incorrect remove element entry";
				return new LogEntry();
			}

			properties[propertyParts[0]] = propertyParts[1];
		}

		return new RemoveElementEntry(Id::loadFromString(components[1]), properties, Id::loadFromString(components[3])
				, components.size() == 5 ? Id::loadFromString(components[4]) : Id());
	} else {
		qDebug() << "unsupported log entry type";
		return new LogEntry();
	}
}

void LogEntry::reverse(qrRepo::CommonRepoApi *repo) const
{
	Q_UNUSED(repo)
}
