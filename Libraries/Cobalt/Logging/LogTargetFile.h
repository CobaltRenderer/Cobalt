// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include "ILogTarget.h"
#if (defined(__cplusplus) && (__cplusplus >= 201703L)) || (defined(_MSVC_LANG) && (_MSVC_LANG >= 201703L))
#include <filesystem>
#endif
#include <fstream>
#include <string>
namespace cobalt { namespace logging {

class LogTargetFile : public ILogTarget
{
public:
	// Constructors
	inline LogTargetFile();
	inline static std::unique_ptr<LogTargetFile, ILogTarget::Deleter> Create();

	// Delete method
	inline void Delete() override;

	// File methods
	inline bool OpenLogFile(const std::string& filePath, bool append = true);
	inline bool OpenLogFile(const std::wstring& filePath, bool append = true);
#if (defined(__cplusplus) && (__cplusplus >= 201703L)) || (defined(_MSVC_LANG) && (_MSVC_LANG >= 201703L))
	inline bool OpenLogFile(const std::filesystem::path& filePath, bool append = true);
#endif
	inline void CloseLogFile();
	inline void FlushAfterWrite(ILogger::SeverityFilter FlushFilter);

	// Logging methods
	inline void LogMessage(const char* scope, size_t scopeLength, ILogger::Severity severity, const char* message, size_t messageLength) override;

protected:
	// Constructors
	inline ~LogTargetFile();

private:
	// Unicode conversion methods
	inline static std::string NativeWideStringToUTF8(const std::wstring& nativeWideString);
	inline static std::wstring UTF8ToNativeWideString(const std::string& stringUTF8);
#ifdef _WIN32
	inline static std::wstring UTF8ToUTF16(const std::string& stringUTF8);
	inline static std::string UTF16ToUTF8(const std::wstring& stringUTF16);
#else
	inline static std::wstring UTF8ToUTF32(const std::string& stringUTF8);
	inline static std::string UTF32ToUTF8(const std::wstring& stringUTF32);
#endif

private:
	bool _fileOpened;
	std::ofstream _logFile;
	ILogger::SeverityFilter _flushFilter = ILogger::SeverityFilter::WarningOrHigher;
};

}} // namespace cobalt::logging
#include "LogTargetFile.inl"
