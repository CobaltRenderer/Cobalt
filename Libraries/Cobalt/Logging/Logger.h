// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include <Cobalt/LoggingInterface/LoggingInterface.pkg>
namespace cobalt { namespace logging {
class LogManager;

class Logger : public ILogger
{
public:
	// Friend declarations
	friend class LogManager;

	// Destroy method
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

private:
	// Constructors
	inline Logger(LogManager& logManager, const std::string& scope);

private:
	LogManager& _logManager;
	std::string _scope;
};

}} // namespace cobalt::logging
#include "Logger.inl"
