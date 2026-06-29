// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
namespace cobalt { namespace logging {

//----------------------------------------------------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------------------------------------------------
LogTargetStandardOut::LogTargetStandardOut(bool flushAfterOutput)
: _flushAfterOutput(flushAfterOutput)
{}

//----------------------------------------------------------------------------------------------------------------------
std::unique_ptr<LogTargetStandardOut, ILogTarget::Deleter> LogTargetStandardOut::Create(bool flushAfterOutput)
{
	return std::unique_ptr<LogTargetStandardOut, ILogTarget::Deleter>(new LogTargetStandardOut(flushAfterOutput));
}

//----------------------------------------------------------------------------------------------------------------------
// Delete method
//----------------------------------------------------------------------------------------------------------------------
void LogTargetStandardOut::Delete()
{
	delete this;
}

//----------------------------------------------------------------------------------------------------------------------
// Logging methods
//----------------------------------------------------------------------------------------------------------------------
void LogTargetStandardOut::LogMessage(const char* scope, size_t scopeLength, ILogger::Severity severity, const char* message, size_t messageLength)
{
	// Write the severity
	switch (severity)
	{
	case ILogger::Severity::Critical:
		std::cout << "C! ";
		break;
	case ILogger::Severity::Error:
		std::cout << "E: ";
		break;
	case ILogger::Severity::Warning:
		std::cout << "W: ";
		break;
	case ILogger::Severity::Info:
		std::cout << "i: ";
		break;
	case ILogger::Severity::Debug:
		std::cout << "d: ";
		break;
	case ILogger::Severity::Trace:
		std::cout << "t: ";
		break;
	}

	// Write the scope if required
	if (scopeLength > 0)
	{
		std::cout.write(scope, scopeLength);
		std::cout << " : ";
	}

	// Write the message
	std::cout.write(message, messageLength);
	std::cout << "\n";

	// Flush the output buffer if requested
	if (_flushAfterOutput)
	{
		std::cout.flush();
	}
}

}} // namespace cobalt::logging
