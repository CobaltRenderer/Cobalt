// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include "ILogTarget.h"
namespace cobalt { namespace logging {

class LogTargetStandardOut : public ILogTarget
{
public:
	// Constructors
	inline explicit LogTargetStandardOut(bool flushAfterOutput);
	inline static std::unique_ptr<LogTargetStandardOut, ILogTarget::Deleter> Create(bool flushAfterOutput);

	// Delete method
	inline void Delete() override;

	// Logging methods
	inline void LogMessage(const char* scope, size_t scopeLength, ILogger::Severity severity, const char* message, size_t messageLength) override;

protected:
	// Constructors
	inline ~LogTargetStandardOut() = default;

private:
	bool _flushAfterOutput;
};

}} // namespace cobalt::logging
#include "LogTargetStandardOut.inl"
