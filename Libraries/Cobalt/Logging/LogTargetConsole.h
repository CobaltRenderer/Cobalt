// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#ifdef INCLUDE_LOGTARGETCONSOLE
#include "ILogTarget.h"
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
namespace cobalt { namespace logging {

class LogTargetConsole : public ILogTarget
{
public:
	// Constructors
	inline LogTargetConsole();
	inline static std::unique_ptr<LogTargetConsole, ILogTarget::Deleter> Create();

	// Delete method
	inline void Delete() override;

	// Open methods
	inline bool OpenConsoleHandle();
	inline void CloseConsoleHandle();

	// Logging methods
	inline void LogMessage(const char* scope, size_t scopeLength, ILogger::Severity severity, const char* message, size_t messageLength) override;

protected:
	// Constructors
	inline ~LogTargetConsole();

private:
	inline static std::wstring UTF8ToUTF16(const std::string& stringUTF8);

private:
	bool _consoleHandleOpened;
	HANDLE _consoleHandle;
};

}} // namespace cobalt::logging
#include "LogTargetConsole.inl"

#endif
