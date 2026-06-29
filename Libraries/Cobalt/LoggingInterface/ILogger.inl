// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include <cassert>
#include <iostream>
#include <sstream>
#include <utility>
namespace cobalt { namespace logging {

//----------------------------------------------------------------------------------------------------------------------
// Scope methods
//----------------------------------------------------------------------------------------------------------------------
std::string ILogger::GetScope() const
{
	return GetScopeInternal();
}

//----------------------------------------------------------------------------------------------------------------------
// Severity methods
//----------------------------------------------------------------------------------------------------------------------
bool ILogger::IsLogSeverityEnabled(Severity severity) const
{
	return IsLogSeverityEnabledInternal(severity);
}

//----------------------------------------------------------------------------------------------------------------------
// Logger creation methods
//----------------------------------------------------------------------------------------------------------------------
ILogger::unique_ptr ILogger::CloneLogger() const
{
	return ILogger::unique_ptr(CloneLoggerInternal());
}

//----------------------------------------------------------------------------------------------------------------------
ILogger::unique_ptr ILogger::GetLoggerChildScope(const std::string& childScopeName) const
{
	return ILogger::unique_ptr(GetLoggerChildScopeInternal(childScopeName.c_str()));
}

//----------------------------------------------------------------------------------------------------------------------
// Generic logging methods
//----------------------------------------------------------------------------------------------------------------------
void ILogger::Log(Severity severity, const std::string& message) const
{
	LogInternal(severity, message);
}

//----------------------------------------------------------------------------------------------------------------------
void ILogger::Log(Severity severity, const std::wstring& message) const
{
	LogInternal(severity, message);
}

//----------------------------------------------------------------------------------------------------------------------
template<class... Args>
void ILogger::Log(Severity severity, const std::string& formatString, Args&&... args) const
{
	LogInternal(severity, formatString, std::forward<Args>(args)...);
}

//----------------------------------------------------------------------------------------------------------------------
template<class... Args>
void ILogger::Log(Severity severity, const std::wstring& formatString, Args&&... args) const
{
	LogInternal(severity, formatString, std::forward<Args>(args)...);
}

//----------------------------------------------------------------------------------------------------------------------
// Severity logging methods
//----------------------------------------------------------------------------------------------------------------------
void ILogger::Critical(const std::string& message) const
{
	LogInternal(Severity::Critical, message);
}

//----------------------------------------------------------------------------------------------------------------------
void ILogger::Critical(const std::wstring& message) const
{
	LogInternal(Severity::Critical, message);
}

//----------------------------------------------------------------------------------------------------------------------
template<class... Args>
void ILogger::Critical(const std::string& formatString, Args&&... args) const
{
	LogInternal(Severity::Critical, formatString, std::forward<Args>(args)...);
}

//----------------------------------------------------------------------------------------------------------------------
template<class... Args>
void ILogger::Critical(const std::wstring& formatString, Args&&... args) const
{
	LogInternal(Severity::Critical, formatString, std::forward<Args>(args)...);
}

//----------------------------------------------------------------------------------------------------------------------
void ILogger::Error(const std::string& message) const
{
	LogInternal(Severity::Error, message);
}

//----------------------------------------------------------------------------------------------------------------------
void ILogger::Error(const std::wstring& message) const
{
	LogInternal(Severity::Error, message);
}

//----------------------------------------------------------------------------------------------------------------------
template<class... Args>
void ILogger::Error(const std::string& formatString, Args&&... args) const
{
	LogInternal(Severity::Error, formatString, std::forward<Args>(args)...);
}

//----------------------------------------------------------------------------------------------------------------------
template<class... Args>
void ILogger::Error(const std::wstring& formatString, Args&&... args) const
{
	LogInternal(Severity::Error, formatString, std::forward<Args>(args)...);
}

//----------------------------------------------------------------------------------------------------------------------
void ILogger::Warning(const std::string& message) const
{
	LogInternal(Severity::Warning, message);
}

//----------------------------------------------------------------------------------------------------------------------
void ILogger::Warning(const std::wstring& message) const
{
	LogInternal(Severity::Warning, message);
}

//----------------------------------------------------------------------------------------------------------------------
template<class... Args>
void ILogger::Warning(const std::string& formatString, Args&&... args) const
{
	LogInternal(Severity::Warning, formatString, std::forward<Args>(args)...);
}

//----------------------------------------------------------------------------------------------------------------------
template<class... Args>
void ILogger::Warning(const std::wstring& formatString, Args&&... args) const
{
	LogInternal(Severity::Warning, formatString, std::forward<Args>(args)...);
}

//----------------------------------------------------------------------------------------------------------------------
void ILogger::Info(const std::string& message) const
{
	LogInternal(Severity::Info, message);
}

//----------------------------------------------------------------------------------------------------------------------
void ILogger::Info(const std::wstring& message) const
{
	LogInternal(Severity::Info, message);
}

//----------------------------------------------------------------------------------------------------------------------
template<class... Args>
void ILogger::Info(const std::string& formatString, Args&&... args) const
{
	LogInternal(Severity::Info, formatString, std::forward<Args>(args)...);
}

//----------------------------------------------------------------------------------------------------------------------
template<class... Args>
void ILogger::Info(const std::wstring& formatString, Args&&... args) const
{
	LogInternal(Severity::Info, formatString, std::forward<Args>(args)...);
}

//----------------------------------------------------------------------------------------------------------------------
void ILogger::Debug(const std::string& message) const
{
	LogInternal(Severity::Debug, message);
}

//----------------------------------------------------------------------------------------------------------------------
void ILogger::Debug(const std::wstring& message) const
{
	LogInternal(Severity::Debug, message);
}

//----------------------------------------------------------------------------------------------------------------------
template<class... Args>
void ILogger::Debug(const std::string& formatString, Args&&... args) const
{
	LogInternal(Severity::Debug, formatString, std::forward<Args>(args)...);
}

//----------------------------------------------------------------------------------------------------------------------
template<class... Args>
void ILogger::Debug(const std::wstring& formatString, Args&&... args) const
{
	LogInternal(Severity::Debug, formatString, std::forward<Args>(args)...);
}

//----------------------------------------------------------------------------------------------------------------------
void ILogger::Trace(const std::string& message) const
{
	LogInternal(Severity::Trace, message);
}

//----------------------------------------------------------------------------------------------------------------------
void ILogger::Trace(const std::wstring& message) const
{
	LogInternal(Severity::Trace, message);
}

//----------------------------------------------------------------------------------------------------------------------
template<class... Args>
void ILogger::Trace(const std::string& formatString, Args&&... args) const
{
	LogInternal(Severity::Trace, formatString, std::forward<Args>(args)...);
}

//----------------------------------------------------------------------------------------------------------------------
template<class... Args>
void ILogger::Trace(const std::wstring& formatString, Args&&... args) const
{
	LogInternal(Severity::Trace, formatString, std::forward<Args>(args)...);
}

//----------------------------------------------------------------------------------------------------------------------
template<class... Args>
void ILogger::LogInternal(Severity severity, const std::wstring& formatString, Args&&... args) const
{
	// If no log targets are set to log messages of the target severity, abort any further processing.
	if (!IsLogSeverityEnabled(severity))
	{
		return;
	}

	// Convert the format wstring, and process the log message as a string.
	LogInternal(severity, NativeWideStringToUTF8(formatString), std::forward<Args>(args)...);
}

//----------------------------------------------------------------------------------------------------------------------
template<class... Args>
void ILogger::LogInternal(Severity severity, const std::string& formatString, Args&&... args) const
{
	// If no log targets are set to log messages of the target severity, abort any further processing.
	if (!IsLogSeverityEnabled(severity))
	{
		return;
	}

	// Convert all supplied arguments to a string representation
	const size_t argCount = sizeof...(Args);
	std::string argsResolved[argCount];
	ResolveArgs(argsResolved, std::forward<Args>(args)...);

	// Resolve the format string and argument values down to a single string
	std::string messageResolved = ResolveFormatString(formatString, argCount, argsResolved);

	// Log the resolved string with the requested severity level
	LogInternal(severity, messageResolved);
}

//----------------------------------------------------------------------------------------------------------------------
void ILogger::LogInternal(Severity severity, const std::wstring& message) const
{
	// If no log targets are set to log messages of the target severity, abort any further processing.
	if (!IsLogSeverityEnabled(severity))
	{
		return;
	}

	// Convert the message string, and process the log message as a wstring.
	LogInternal(severity, NativeWideStringToUTF8(message));
}

//----------------------------------------------------------------------------------------------------------------------
void ILogger::LogInternal(Severity severity, const std::string& message) const
{
	LogInternal(severity, message.c_str(), message.size());
}

//----------------------------------------------------------------------------------------------------------------------
// Format string methods
//----------------------------------------------------------------------------------------------------------------------
std::string ILogger::ResolveFormatString(const std::string& formatString, size_t argCount, const std::string* formatStringArgs) const
{
	// Allocate a variable to hold our resolved string content
	std::string resolvedString;
	resolvedString.reserve(formatString.size() * 2);

	// Parse the format string, storing literal text and argument values in our resolved string.
	size_t formatStringCurrentPos = 0;
	while (formatStringCurrentPos < formatString.size())
	{
		// Attempt to find the next argument insert position
		size_t formatStringStartPos = formatString.find_first_of('{', formatStringCurrentPos);
		if (formatStringStartPos == std::string::npos)
		{
			break;
		}
		size_t formatStringEndPos = formatString.find_first_of('}', formatStringStartPos);
		if (formatStringEndPos == std::string::npos)
		{
			break;
		}

		// Retrieve the argument index number
		size_t argIndex;
		std::string argIndexAsString = formatString.substr(formatStringStartPos + 1, formatStringEndPos - (formatStringStartPos + 1));
		argIndex = (size_t)std::stoi(argIndexAsString);

		// Ensure the argument index number is valid
		assert(argIndex < argCount);

		// Append any text leading up to this argument in the format string, and the string representation of the target
		// argument.
		resolvedString.append(formatString.data() + formatStringCurrentPos, formatStringStartPos - formatStringCurrentPos);
		if (argIndex >= argCount)
		{
			resolvedString.append("{MISSING ARG " + argIndexAsString + "}");
		}
		else
		{
			resolvedString.append(formatStringArgs[argIndex]);
		}

		// Advance past the argument in the format string
		formatStringCurrentPos = formatStringEndPos + 1;
	}
	resolvedString.append(formatString.data() + formatStringCurrentPos);

	// Return the resolved string to the caller
	return resolvedString;
}

//----------------------------------------------------------------------------------------------------------------------
template<class T>
void ILogger::ResolveArgs(std::string* argAsString, T&& arg) const
{
	ArgResolver<typename std::remove_cv<typename std::remove_reference<T>::type>::type>::ResolveArg(arg, *argAsString);
}

//----------------------------------------------------------------------------------------------------------------------
template<class T, class... Args>
void ILogger::ResolveArgs(std::string* argAsString, T&& arg, Args&&... args) const
{
	ArgResolver<typename std::remove_cv<typename std::remove_reference<T>::type>::type>::ResolveArg(arg, *(argAsString++));
	ResolveArgs(argAsString, std::forward<Args>(args)...);
}

//----------------------------------------------------------------------------------------------------------------------
// Unicode conversion methods
//----------------------------------------------------------------------------------------------------------------------
std::string ILogger::NativeWideStringToUTF8(const std::wstring& nativeWideString)
{
#ifdef _WIN32
	return UTF16ToUTF8(nativeWideString);
#else
	return UTF32ToUTF8(nativeWideString);
#endif
}

//----------------------------------------------------------------------------------------------------------------------
#ifdef _WIN32
std::string ILogger::UTF16ToUTF8(const std::wstring& stringUTF16)
{
	// Convert the encoding of the supplied string
	std::string stringUTF8;
	size_t sourceStringPos = 0;
	size_t sourceStringSize = stringUTF16.size();
	stringUTF8.reserve(sourceStringSize * 2);
	while (sourceStringPos < sourceStringSize)
	{
		// Check if a surrogate pair is used for this character
		bool usesSurrogatePair = (((unsigned int)stringUTF16[sourceStringPos] & 0xF800) == 0xD800);

		// Ensure that the requested number of code units are left in the source string
		if (usesSurrogatePair && ((sourceStringPos + 2) > sourceStringSize))
		{
			break;
		}

		// Decode the character from UTF-16 encoding
		unsigned int unicodeCodePoint;
		if (usesSurrogatePair)
		{
			unicodeCodePoint = 0x10000 + ((((unsigned int)stringUTF16[sourceStringPos] & 0x03FF) << 10) | ((unsigned int)stringUTF16[sourceStringPos + 1] & 0x03FF));
		}
		else
		{
			unicodeCodePoint = (unsigned int)stringUTF16[sourceStringPos];
		}

		// Encode the character into UTF-8 encoding
		if (unicodeCodePoint <= 0x7F)
		{
			stringUTF8.push_back((char)unicodeCodePoint);
		}
		else if (unicodeCodePoint <= 0x07FF)
		{
			char convertedCodeUnit1 = (char)(0xC0 | (unicodeCodePoint >> 6));
			char convertedCodeUnit2 = (char)(0x80 | (unicodeCodePoint & 0x3F));
			stringUTF8.push_back(convertedCodeUnit1);
			stringUTF8.push_back(convertedCodeUnit2);
		}
		else if (unicodeCodePoint <= 0xFFFF)
		{
			char convertedCodeUnit1 = (char)(0xE0 | (unicodeCodePoint >> 12));
			char convertedCodeUnit2 = (char)(0x80 | ((unicodeCodePoint >> 6) & 0x3F));
			char convertedCodeUnit3 = (char)(0x80 | (unicodeCodePoint & 0x3F));
			stringUTF8.push_back(convertedCodeUnit1);
			stringUTF8.push_back(convertedCodeUnit2);
			stringUTF8.push_back(convertedCodeUnit3);
		}
		else
		{
			char convertedCodeUnit1 = (char)(0xF0 | (unicodeCodePoint >> 18));
			char convertedCodeUnit2 = (char)(0x80 | ((unicodeCodePoint >> 12) & 0x3F));
			char convertedCodeUnit3 = (char)(0x80 | ((unicodeCodePoint >> 6) & 0x3F));
			char convertedCodeUnit4 = (char)(0x80 | (unicodeCodePoint & 0x3F));
			stringUTF8.push_back(convertedCodeUnit1);
			stringUTF8.push_back(convertedCodeUnit2);
			stringUTF8.push_back(convertedCodeUnit3);
			stringUTF8.push_back(convertedCodeUnit4);
		}

		// Advance past the converted code units
		sourceStringPos += (usesSurrogatePair) ? 2 : 1;
	}

	// Return the converted string to the caller
	return stringUTF8;
}
#endif

//----------------------------------------------------------------------------------------------------------------------
#ifndef _WIN32
std::string ILogger::UTF32ToUTF8(const std::wstring& stringUTF32)
{
	// Convert the encoding of the supplied string
	std::string stringUTF8;
	size_t sourceStringPos = 0;
	size_t sourceStringSize = stringUTF32.size();
	stringUTF8.reserve(sourceStringSize * 4);
	while (sourceStringPos < sourceStringSize)
	{
		// Each character in the wstring is a unicdoe code point
		auto unicodeCodePoint = (unsigned int)(int)stringUTF32[sourceStringPos];

		// Encode the character into UTF-8 encoding
		if (unicodeCodePoint <= 0x7F)
		{
			stringUTF8.push_back((char)unicodeCodePoint);
		}
		else if (unicodeCodePoint <= 0x07FF)
		{
			char convertedCodeUnit1 = (char)(0xC0 | (unicodeCodePoint >> 6));
			char convertedCodeUnit2 = (char)(0x80 | (unicodeCodePoint & 0x3F));
			stringUTF8.push_back(convertedCodeUnit1);
			stringUTF8.push_back(convertedCodeUnit2);
		}
		else if (unicodeCodePoint <= 0xFFFF)
		{
			char convertedCodeUnit1 = (char)(0xE0 | (unicodeCodePoint >> 12));
			char convertedCodeUnit2 = (char)(0x80 | ((unicodeCodePoint >> 6) & 0x3F));
			char convertedCodeUnit3 = (char)(0x80 | (unicodeCodePoint & 0x3F));
			stringUTF8.push_back(convertedCodeUnit1);
			stringUTF8.push_back(convertedCodeUnit2);
			stringUTF8.push_back(convertedCodeUnit3);
		}
		else
		{
			char convertedCodeUnit1 = (char)(0xF0 | (unicodeCodePoint >> 18));
			char convertedCodeUnit2 = (char)(0x80 | ((unicodeCodePoint >> 12) & 0x3F));
			char convertedCodeUnit3 = (char)(0x80 | ((unicodeCodePoint >> 6) & 0x3F));
			char convertedCodeUnit4 = (char)(0x80 | (unicodeCodePoint & 0x3F));
			stringUTF8.push_back(convertedCodeUnit1);
			stringUTF8.push_back(convertedCodeUnit2);
			stringUTF8.push_back(convertedCodeUnit3);
			stringUTF8.push_back(convertedCodeUnit4);
		}

		// Move to next character
		++sourceStringPos;
	}

	// Return the converted string to the caller
	return stringUTF8;
}
#endif

//----------------------------------------------------------------------------------------------------------------------
// Argument resolver
//----------------------------------------------------------------------------------------------------------------------
template<>
class ILogger::ArgResolver<float, false, true>
{
public:
	inline static void ResolveArg(float arg, std::string& argResolved);
};

//----------------------------------------------------------------------------------------------------------------------
template<>
class ILogger::ArgResolver<double, false, true>
{
public:
	inline static void ResolveArg(double arg, std::string& argResolved);
};

//----------------------------------------------------------------------------------------------------------------------
template<>
class ILogger::ArgResolver<std::string, false, false>
{
public:
	inline static void ResolveArg(const std::string& arg, std::string& argResolved);
};

//----------------------------------------------------------------------------------------------------------------------
template<>
class ILogger::ArgResolver<std::wstring, false, false>
{
public:
	inline static void ResolveArg(const std::wstring& arg, std::string& argResolved);
};

//----------------------------------------------------------------------------------------------------------------------
template<class T>
void ILogger::ArgResolver<T, false, false>::ResolveArg(const T& arg, std::string& argResolved)
{
	std::ostringstream stringStream;
	stringStream << arg;
	argResolved = stringStream.str();
}

//----------------------------------------------------------------------------------------------------------------------
template<>
inline void ILogger::ArgResolver<unsigned char, false, true>::ResolveArg(unsigned char arg, std::string& argResolved)
{
	std::ostringstream stringStream;
	stringStream << (unsigned int)arg;
	argResolved = stringStream.str();
}

//----------------------------------------------------------------------------------------------------------------------
template<class T>
void ILogger::ArgResolver<T, false, true>::ResolveArg(T arg, std::string& argResolved)
{
	std::ostringstream stringStream;
	stringStream << arg;
	argResolved = stringStream.str();
}

//----------------------------------------------------------------------------------------------------------------------
template<class T>
void ILogger::ArgResolver<T, true, false>::ResolveArg(T arg, std::string& argResolved)
{
	std::ostringstream stringStream;
	stringStream << (unsigned int)arg;
	argResolved = stringStream.str();
}

//----------------------------------------------------------------------------------------------------------------------
void ILogger::ArgResolver<float, false, true>::ResolveArg(float arg, std::string& argResolved)
{
	std::ostringstream stringStream;
	stringStream << std::fixed << arg;
	argResolved = stringStream.str();
}

//----------------------------------------------------------------------------------------------------------------------
void ILogger::ArgResolver<double, false, true>::ResolveArg(double arg, std::string& argResolved)
{
	std::ostringstream stringStream;
	stringStream << std::fixed << arg;
	argResolved = stringStream.str();
}

//----------------------------------------------------------------------------------------------------------------------
void ILogger::ArgResolver<std::wstring, false, false>::ResolveArg(const std::wstring& arg, std::string& argResolved)
{
	argResolved = NativeWideStringToUTF8(arg);
}

//----------------------------------------------------------------------------------------------------------------------
void ILogger::ArgResolver<std::string, false, false>::ResolveArg(const std::string& arg, std::string& argResolved)
{
	argResolved = arg;
}

//----------------------------------------------------------------------------------------
// Enumeration operators
//----------------------------------------------------------------------------------------
inline ILogger::SeverityFilter operator|(ILogger::SeverityFilter left, ILogger::SeverityFilter right)
{
	return (ILogger::SeverityFilter)((std::underlying_type<ILogger::SeverityFilter>::type)left | (std::underlying_type<ILogger::SeverityFilter>::type)right);
}

//----------------------------------------------------------------------------------------
inline ILogger::SeverityFilter& operator|=(ILogger::SeverityFilter& left, ILogger::SeverityFilter right)
{
	left = (left | right);
	return left;
}

//----------------------------------------------------------------------------------------
inline ILogger::SeverityFilter operator&(ILogger::SeverityFilter left, ILogger::SeverityFilter right)
{
	return (ILogger::SeverityFilter)((std::underlying_type<ILogger::SeverityFilter>::type)left & (std::underlying_type<ILogger::SeverityFilter>::type)right);
}

//----------------------------------------------------------------------------------------
inline ILogger::SeverityFilter& operator&=(ILogger::SeverityFilter& left, ILogger::SeverityFilter right)
{
	left = (left & right);
	return left;
}

}} // namespace cobalt::logging
