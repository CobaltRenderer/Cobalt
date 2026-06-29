// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include <cstdint>
#include <memory>
#include <string>
#include <type_traits>
namespace cobalt { namespace logging {

class ILogger
{
public:
	// Enumerations
	enum class Severity : uint32_t
	{
		Critical = 0x01, // Fatal error or application crash
		Error = 0x02,    // Recoverable error
		Warning = 0x04,  // Noncritical problem
		Info = 0x08,     // Informational message
		Debug = 0x10,    // Debugging info. Logged to file but not displayed by default.
		Trace = 0x20,    // Trace info. Not recorded by default.
	};

	// Enumerations
	enum class SeverityFilter : uint32_t
	{
		None = 0,
		CriticalOrHigher = (uint32_t)Severity::Critical,
		ErrorOrHigher = (uint32_t)Severity::Critical | (uint32_t)Severity::Error,
		WarningOrHigher = (uint32_t)Severity::Critical | (uint32_t)Severity::Error | (uint32_t)Severity::Warning,
		InfoOrHigher = (uint32_t)Severity::Critical | (uint32_t)Severity::Error | (uint32_t)Severity::Warning | (uint32_t)Severity::Info,
		DebugOrHigher = (uint32_t)Severity::Critical | (uint32_t)Severity::Error | (uint32_t)Severity::Warning | (uint32_t)Severity::Info | (uint32_t)Severity::Debug,
		TraceOrHigher = (uint32_t)Severity::Critical | (uint32_t)Severity::Error | (uint32_t)Severity::Warning | (uint32_t)Severity::Info | (uint32_t)Severity::Debug | (uint32_t)Severity::Trace,
		Critical = (uint32_t)Severity::Critical,
		Error = (uint32_t)Severity::Error,
		Warning = (uint32_t)Severity::Warning,
		Info = (uint32_t)Severity::Info,
		Debug = (uint32_t)Severity::Debug,
		Trace = (uint32_t)Severity::Trace,
		All = TraceOrHigher,
	};

public:
	// Nested types
	struct Deleter
	{
		inline void operator()(ILogger* target)
		{
			target->Delete();
		}
	};
	typedef std::unique_ptr<ILogger, Deleter> unique_ptr;

public:
	// Delete method
	virtual void Delete() = 0;

	// Scope methods
	inline std::string GetScope() const;

	// Severity methods
	inline bool IsLogSeverityEnabled(Severity severity) const;

	// Logger creation methods
	inline unique_ptr CloneLogger() const;
	inline unique_ptr GetLoggerChildScope(const std::string& childScopeName) const;

	// Generic logging methods
	inline void Log(Severity severity, const std::string& message) const;
	inline void Log(Severity severity, const std::wstring& message) const;
	template<class... Args>
	void Log(Severity severity, const std::string& formatString, Args&&... args) const;
	template<class... Args>
	void Log(Severity severity, const std::wstring& formatString, Args&&... args) const;

	// Severity logging methods
	inline void Critical(const std::string& message) const;
	inline void Critical(const std::wstring& message) const;
	template<class... Args>
	void Critical(const std::string& formatString, Args&&... args) const;
	template<class... Args>
	void Critical(const std::wstring& formatString, Args&&... args) const;
	inline void Error(const std::string& message) const;
	inline void Error(const std::wstring& message) const;
	template<class... Args>
	void Error(const std::string& formatString, Args&&... args) const;
	template<class... Args>
	void Error(const std::wstring& formatString, Args&&... args) const;
	inline void Warning(const std::string& message) const;
	inline void Warning(const std::wstring& message) const;
	template<class... Args>
	void Warning(const std::string& formatString, Args&&... args) const;
	template<class... Args>
	void Warning(const std::wstring& formatString, Args&&... args) const;
	inline void Info(const std::string& message) const;
	inline void Info(const std::wstring& message) const;
	template<class... Args>
	void Info(const std::string& formatString, Args&&... args) const;
	template<class... Args>
	void Info(const std::wstring& formatString, Args&&... args) const;
	inline void Debug(const std::string& message) const;
	inline void Debug(const std::wstring& message) const;
	template<class... Args>
	void Debug(const std::string& formatString, Args&&... args) const;
	template<class... Args>
	void Debug(const std::wstring& formatString, Args&&... args) const;
	inline void Trace(const std::string& message) const;
	inline void Trace(const std::wstring& message) const;
	template<class... Args>
	void Trace(const std::string& formatString, Args&&... args) const;
	template<class... Args>
	void Trace(const std::wstring& formatString, Args&&... args) const;

protected:
	// Constructors
	~ILogger() = default;
	ILogger() = default;

	// Scope methods
	virtual const char* GetScopeInternal() const = 0;

	// Severity methods
	virtual bool IsLogSeverityEnabledInternal(Severity severity) const = 0;

	// Logger creation methods
	virtual ILogger* CloneLoggerInternal() const = 0;
	virtual ILogger* GetLoggerChildScopeInternal(const char* childScopeName) const = 0;

	// Logging methods
	virtual void LogInternal(Severity severity, const char* message, size_t messageLength) const = 0;

private:
	// Argument resolver
	template<class T, bool IsEnum = std::is_enum<T>::value, bool IsIntegral = std::is_integral<T>::value>
	class ArgResolver
	{};
	template<class T>
	class ArgResolver<T, false, false>
	{
	public:
		inline static void ResolveArg(const T& arg, std::string& argResolved);
	};
	template<class T>
	class ArgResolver<T, false, true>
	{
	public:
		inline static void ResolveArg(T arg, std::string& argResolved);
	};
	template<class T>
	class ArgResolver<T, true, false>
	{
	public:
		inline static void ResolveArg(T arg, std::string& argResolved);
	};

private:
	// Logging methods
	inline void LogInternal(Severity severity, const std::string& message) const;
	inline void LogInternal(Severity severity, const std::wstring& message) const;
	template<class... Args>
	void LogInternal(Severity severity, const std::string& formatString, Args&&... args) const;
	template<class... Args>
	void LogInternal(Severity severity, const std::wstring& formatString, Args&&... args) const;

	// Format string methods
	inline std::string ResolveFormatString(const std::string& formatString, size_t argCount, const std::string* formatStringArgs) const;
	template<class T>
	void ResolveArgs(std::string* argAsString, T&& arg) const;
	template<class T, class... Args>
	void ResolveArgs(std::string* argAsString, T&& arg, Args&&... args) const;

	// Unicode conversion methods
	inline static std::string NativeWideStringToUTF8(const std::wstring& nativeWideString);
#ifdef _WIN32
	inline static std::string UTF16ToUTF8(const std::wstring& stringUTF16);
#else
	inline static std::string UTF32ToUTF8(const std::wstring& stringUTF32);
#endif
};

}} // namespace cobalt::logging
#include "ILogger.inl"
