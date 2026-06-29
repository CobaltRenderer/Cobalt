// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include <utility>
namespace cobalt { namespace logging {

//----------------------------------------------------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------------------------------------------------
LogTargetCallback::LogTargetCallback(std::function<void(const std::string&, ILogger::Severity, const std::string&)>&& callback)
: _callback(std::move(callback))
{}

//----------------------------------------------------------------------------------------------------------------------
std::unique_ptr<LogTargetCallback, ILogTarget::Deleter> LogTargetCallback::Create(std::function<void(const std::string&, ILogger::Severity, const std::string&)> callback)
{
	return std::unique_ptr<LogTargetCallback, ILogTarget::Deleter>(new LogTargetCallback(std::move(callback)));
}

//----------------------------------------------------------------------------------------------------------------------
// Delete method
//----------------------------------------------------------------------------------------------------------------------
void LogTargetCallback::Delete()
{
	delete this;
}

//----------------------------------------------------------------------------------------------------------------------
// Logging methods
//----------------------------------------------------------------------------------------------------------------------
void LogTargetCallback::LogMessage(const char* scope, size_t scopeLength, ILogger::Severity severity, const char* message, size_t messageLength)
{
	_callback(scope, severity, message);
}

}} // namespace cobalt::logging
