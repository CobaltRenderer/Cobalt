// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include "ILogTarget.h"
#include <Cobalt/LoggingInterface/LoggingInterface.pkg>
#include <atomic>
#include <mutex>
#include <vector>
namespace cobalt { namespace logging {

class LogManager
{
public:
	// Friend declarations
	friend class Logger;

public:
	// Constructors
	inline LogManager();

	// Severity methods
	inline ILogger::SeverityFilter GetIncludeSeverity() const;
	inline void SetIncludeSeverity(ILogger::SeverityFilter value);
	inline bool IsLogSeverityEnabled(ILogger::Severity severity) const;

	// Logger creation methods
	inline ILogger::unique_ptr GetLogger(const std::string& scopeName);

	// Log target methods
	inline void AddLogTarget(ILogTarget::unique_ptr logTarget, ILogger::SeverityFilter severityFilter = ILogger::SeverityFilter::All);

protected:
	// Logging methods
	inline void LogMessage(const char* scope, size_t scopeLength, ILogger::Severity severity, const char* message, size_t messageLength);

private:
	// Structures
	struct LogTargetEntry
	{
		ILogTarget::unique_ptr logTarget;
		ILogger::SeverityFilter severityFilter = {};
	};

private:
	// Severity filter methods
	static inline bool SeverityPassesFilter(ILogger::Severity severity, ILogger::SeverityFilter severityFilter);

private:
	mutable std::mutex _accessMutex;
	std::vector<LogTargetEntry> _logTargetEntries;
	std::atomic<ILogger::SeverityFilter> _severityFilter;
};

}} // namespace cobalt::logging
#include "LogManager.inl"
