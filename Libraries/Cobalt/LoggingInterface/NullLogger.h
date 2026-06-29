// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include "ILogger.h"
namespace cobalt { namespace logging {

class NullLogger : public ILogger
{
public:
	// Delete method
	inline void Delete() override;

protected:
	// Scope methods
	inline const char* GetScopeInternal() const override;

	// Severity methods
	inline bool IsLogSeverityEnabledInternal(Severity severity) const override;

	// Logger creation methods
	inline ILogger* CloneLoggerInternal() const override;
	inline ILogger* GetLoggerChildScopeInternal(const char* childScopeName) const override;

	// Logging methods
	inline void LogInternal(Severity severity, const char* message, size_t messageLength) const override;
};

}} // namespace cobalt::logging
#include "NullLogger.inl"
