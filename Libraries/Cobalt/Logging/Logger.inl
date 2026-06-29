// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "LogManager.h"
namespace cobalt { namespace logging {

//----------------------------------------------------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------------------------------------------------
Logger::Logger(LogManager& logManager, const std::string& scope)
: _logManager(logManager), _scope(scope)
{}

//----------------------------------------------------------------------------------------------------------------------
// Destroy method
//----------------------------------------------------------------------------------------------------------------------
void Logger::Delete()
{
	delete this;
}

//----------------------------------------------------------------------------------------------------------------------
// Scope methods
//----------------------------------------------------------------------------------------------------------------------
const char* Logger::GetScopeInternal() const
{
	return _scope.c_str();
}

//----------------------------------------------------------------------------------------------------------------------
// Severity methods
//----------------------------------------------------------------------------------------------------------------------
bool Logger::IsLogSeverityEnabledInternal(Severity severity) const
{
	return _logManager.IsLogSeverityEnabled(severity);
}

//----------------------------------------------------------------------------------------------------------------------
// Logger creation methods
//----------------------------------------------------------------------------------------------------------------------
ILogger* Logger::CloneLoggerInternal() const
{
	return new Logger(_logManager, _scope);
}

//----------------------------------------------------------------------------------------------------------------------
ILogger* Logger::GetLoggerChildScopeInternal(const char* childScopeName) const
{
	std::string newScope = (_scope.empty() ? std::string(childScopeName) : (_scope + "." + childScopeName));
	return new Logger(_logManager, newScope);
}

//----------------------------------------------------------------------------------------------------------------------
// Logging methods
//----------------------------------------------------------------------------------------------------------------------
void Logger::LogInternal(Severity severity, const char* message, size_t messageLength) const
{
	_logManager.LogMessage(_scope.c_str(), _scope.size(), severity, message, messageLength);
}

}} // namespace cobalt::logging
