// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include <Cobalt/LoggingInterface/LoggingInterface.pkg>
#include <memory>
namespace cobalt { namespace logging {

class ILogTarget
{
public:
	// Nested types
	struct Deleter
	{
		inline void operator()(ILogTarget* target)
		{
			target->Delete();
		}
	};
	typedef std::unique_ptr<ILogTarget, Deleter> unique_ptr;

public:
	// Delete method
	virtual void Delete() = 0;

	// Logging methods
	virtual void LogMessage(const char* scope, size_t scopeLength, ILogger::Severity severity, const char* message, size_t messageLength) = 0;

protected:
	// Constructors
	inline ~ILogTarget() = default;
};

}} // namespace cobalt::logging
