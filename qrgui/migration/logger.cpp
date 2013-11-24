#include "migration/logger.h"

#include "migration/logEntries/versionEntry.h"
#include "qrrepo/private/repository.h"

using namespace qReal::migration;

Logger::Logger(qrRepo::details::Repository *repo)
	: mRepository(repo)
{
}

Logger::~Logger()
{
	clear();
}

void Logger::addLogEntry(qReal::Id const &diagram, LogEntry * const entry)
{
	mLog[diagram] << entry;
}

void Logger::deleteLogEntry(qReal::Id const &diagram)
{
	removeLastVersions(diagram);
	delete mLog[diagram].last();
	mLog[diagram].removeLast();
}

int Logger::version() const
{
	return mModelVersion;
}

void Logger::clear()
{
	foreach (Id const &id, mLog.keys()) {
		foreach (LogEntry *entry, mLog[id]) {
			delete entry;
		}
	}

	mLog.clear();
	mModelVersion = 1;
}

void Logger::reset(QHash<qReal::Id, QList<LogEntry *> > const &log)
{
	clear();
	mLog = log;

	mModelVersion = 0;
	foreach (qReal::Id const &id, log.keys()) {
		int version = 0;
		foreach (LogEntry * const entry, log[id]) {
			VersionEntry * const versionEntry = dynamic_cast<VersionEntry *>(entry);
			if (versionEntry) {
				version = versionEntry->version();
			}
		}

		mModelVersion = qMax(version, mModelVersion);
	}

	mModelVersion++;
}

QHash<qReal::Id, QList<qReal::migration::LogEntry *> > Logger::log() const
{
	return mLog;
}

QHash<qReal::Id, QList<qReal::migration::LogEntry *> > Logger::logBetween(int startVersion, int endVersion) const
{
	QHash<qReal::Id, QList<qReal::migration::LogEntry *> > result;

	if (startVersion == endVersion) {
		return result;
	}

	if (startVersion > endVersion) {
		qSwap(startVersion, endVersion);
	}

	foreach (qReal::Id const &id, mLog.keys()) {
		QList<LogEntry *>::const_iterator startEntry = mLog[id].begin();
		for (QList<LogEntry *>::const_iterator it = mLog[id].begin(); it != mLog[id].end(); it++) {
			VersionEntry * const versionEntry = dynamic_cast<VersionEntry *>(*it);
			if (versionEntry) {
				if (versionEntry->version() == startVersion) {
					startEntry = it;
					break;
				} else if (versionEntry->version() > startVersion) {
					break;
				}
			}
		}

		QList<LogEntry *>::const_iterator endEntry = mLog[id].end();
		for (QList<LogEntry *>::const_iterator it = mLog[id].end(); it != mLog[id].begin();) {
			it--;
			VersionEntry * const versionEntry = dynamic_cast<VersionEntry *>(*it);
			if (versionEntry) {
				if (versionEntry->version() == endVersion) {
					endEntry = it;
					break;
				} else if (versionEntry->version() < endVersion) {
					break;
				}
			}
		}

		for (QList<LogEntry *>::const_iterator it = startEntry; it != endEntry; it++) {
			if (!dynamic_cast<VersionEntry *>(*it)) {
				result[id] << *it;
			}
		}
	}

	return result;
}

void Logger::createNewVersion()
{
	if (needNewVersion()) {
		foreach (qReal::Id const &id, mLog.keys()) {
			mLog[id] << new VersionEntry(mModelVersion);
		}

		mModelVersion++;
	}
}

bool Logger::needNewVersion() const
{
	foreach (qReal::Id const &id, mLog.keys()) {
		if (mLog[id].empty() || !dynamic_cast<VersionEntry *>(mLog[id].last())) {
			return true;
		}
	}

	return false;
}

void Logger::removeLastVersions(Id const &diagram)
{
	int firstVersionToRemove = mModelVersion;
	for (QList<LogEntry *>::iterator it = mLog[diagram].end(); it != mLog[diagram].begin();) {
		--it;
		VersionEntry * const entry = dynamic_cast<VersionEntry *>(*it);
		if (!entry) {
			break;
		}

		firstVersionToRemove = entry->version();
	}

	mModelVersion = firstVersionToRemove;

	foreach (Id const &id, mLog.keys()) {
		for (QList<LogEntry *>::iterator it = mLog[id].end(); it != mLog[id].begin();) {
			--it;
			VersionEntry const * const entry = dynamic_cast<VersionEntry *>(*it);
			if (entry && entry->version() >= firstVersionToRemove) {
				delete *it;
				it = mLog[id].erase(it);
			}
		}
	}
}

void Logger::rollBackTo(int version)
{
	if (version >= mModelVersion) {
		return;
	}

	foreach (Id const &id, mLog.keys()) {
		for (QList<LogEntry *>::iterator it = mLog[id].end(); it != mLog[id].begin();) {
			--it;
			VersionEntry const * const entry = dynamic_cast<VersionEntry *>(*it);
			if (entry && entry->version() <= version) {
				break;
			}

			if (!entry || entry->version() > version) {
				(*it)->reverse(mRepository);
				delete *it;
				it = mLog[id].erase(it);
			}

		}
	}

	mModelVersion = version + 1;
}
