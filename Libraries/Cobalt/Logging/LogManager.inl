// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "Logger.h"
namespace cobalt { namespace logging {

//----------------------------------------------------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------------------------------------------------
LogManager::LogManager()
: _severityFilter(ILogger::SeverityFilter::All)
{}

//----------------------------------------------------------------------------------------------------------------------
// Severity methods
//----------------------------------------------------------------------------------------------------------------------
ILogger::SeverityFilter LogManager::GetIncludeSeverity() const
{
	return _severityFilter;
}

//----------------------------------------------------------------------------------------------------------------------
void LogManager::SetIncludeSeverity(ILogger::SeverityFilter value)
{
	_severityFilter = value;
}

//----------------------------------------------------------------------------------------------------------------------
bool LogManager::IsLogSeverityEnabled(ILogger::Severity severity) const
{
	return SeverityPassesFilter(severity, _severityFilter);
}

//----------------------------------------------------------------------------------------------------------------------
// Logger creation methods
//----------------------------------------------------------------------------------------------------------------------
std::unique_ptr<ILogger, ILogger::Deleter> LogManager::GetLogger(const std::string& scopeName)
{
	return std::unique_ptr<ILogger, ILogger::Deleter>(new Logger(*this, scopeName));
}

//----------------------------------------------------------------------------------------------------------------------
// Log target methods
//----------------------------------------------------------------------------------------------------------------------
void LogManager::AddLogTarget(ILogTarget::unique_ptr logTarget, ILogger::SeverityFilter severityFilter)
{
	LogTargetEntry entry;
	entry.logTarget = std::move(logTarget);
	entry.severityFilter = severityFilter;
	std::lock_guard<std::mutex> lock(_accessMutex);
	_logTargetEntries.push_back(std::move(entry));
}

//----------------------------------------------------------------------------------------------------------------------
// Logging methods
//----------------------------------------------------------------------------------------------------------------------
void LogManager::LogMessage(const char* scope, size_t scopeLength, ILogger::Severity severity, const char* message, size_t messageLength)
{
	// If no log targets are set to log messages of the target severity, abort any further processing.
	if (!IsLogSeverityEnabled(severity))
	{
		return;
	}

	// Pass this message to each bound log target
	std::lock_guard<std::mutex> lock(_accessMutex);
	for (const auto& logTargetEntry : _logTargetEntries)
	{
		if (!SeverityPassesFilter(severity, logTargetEntry.severityFilter))
		{
			continue;
		}
		logTargetEntry.logTarget->LogMessage(scope, scopeLength, severity, message, messageLength);
	}
}

//----------------------------------------------------------------------------------------------------------------------
// Severity filter methods
//----------------------------------------------------------------------------------------------------------------------
bool LogManager::SeverityPassesFilter(ILogger::Severity severity, ILogger::SeverityFilter severityFilter)
{
	return (((unsigned int)severityFilter & (unsigned int)severity) != 0);
}

}} // namespace cobalt::logging
