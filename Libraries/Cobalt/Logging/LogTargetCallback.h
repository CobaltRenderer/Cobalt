// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include "ILogTarget.h"
#include <functional>
namespace cobalt { namespace logging {

class LogTargetCallback : public ILogTarget
{
public:
	// Constructors
	inline explicit LogTargetCallback(std::function<void(const std::string&, ILogger::Severity, const std::string&)>&& callback);
	inline static std::unique_ptr<LogTargetCallback, ILogTarget::Deleter> Create(std::function<void(const std::string&, ILogger::Severity, const std::string&)> callback);

	// Delete method
	inline void Delete() override;

	// Logging methods
	inline void LogMessage(const char* scope, size_t scopeLength, ILogger::Severity severity, const char* message, size_t messageLength) override;

private:
	std::function<void(const std::string&, ILogger::Severity, const std::string&)> _callback;
};

}} // namespace cobalt::logging
#include "LogTargetCallback.inl"
