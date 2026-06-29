// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
namespace cobalt { namespace logging {

//----------------------------------------------------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------------------------------------------------
LogTargetConsole::LogTargetConsole()
: _consoleHandleOpened(false)
{}

//----------------------------------------------------------------------------------------------------------------------
LogTargetConsole::~LogTargetConsole()
{
	CloseConsoleHandle();
}

//----------------------------------------------------------------------------------------------------------------------
std::unique_ptr<LogTargetConsole, ILogTarget::Deleter> LogTargetConsole::Create()
{
	return std::unique_ptr<LogTargetConsole, ILogTarget::Deleter>(new LogTargetConsole());
}

//----------------------------------------------------------------------------------------------------------------------
// Delete method
//----------------------------------------------------------------------------------------------------------------------
void LogTargetConsole::Delete()
{
	delete this;
}

//----------------------------------------------------------------------------------------------------------------------
// Open methods
//----------------------------------------------------------------------------------------------------------------------
bool LogTargetConsole::OpenConsoleHandle()
{
	// If a console handle has already been opened, return true.
	if (_consoleHandleOpened)
	{
		return true;
	}

	// Attempt to open a handle to the attached console window. Note that we don't assume the console window is bound to
	// the standard output handle here.
	HANDLE consoleHandle = CreateFileW(L"CONOUT$", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
	if (consoleHandle == INVALID_HANDLE_VALUE)
	{
		return false;
	}

	// Flag that we've successfully bound to the console, and return true to the caller.
	_consoleHandle = consoleHandle;
	_consoleHandleOpened = true;
	return true;
}

//----------------------------------------------------------------------------------------------------------------------
void LogTargetConsole::CloseConsoleHandle()
{
	// If we're not bound to the console, abort any further processing.
	if (!_consoleHandleOpened)
	{
		return;
	}

	// Close our handle to the console window
	CloseHandle(_consoleHandle);
	_consoleHandleOpened = false;
}

//----------------------------------------------------------------------------------------------------------------------
// Logging methods
//----------------------------------------------------------------------------------------------------------------------
void LogTargetConsole::LogMessage(const char* scope, size_t scopeLength, ILogger::Severity severity, const char* message, size_t messageLength)
{
	// If we failed to open a handle to the console, abort any further processing.
	if (!_consoleHandleOpened)
	{
		return;
	}

	// Retrieve the current console colour settings
	CONSOLE_SCREEN_BUFFER_INFO consoleScreenBufferInfo;
	GetConsoleScreenBufferInfo(_consoleHandle, &consoleScreenBufferInfo);

	// Update the console colour settings based on the severity
	WORD newColorSettings = consoleScreenBufferInfo.wAttributes;
	switch (severity)
	{
	case ILogger::Severity::Critical:
		newColorSettings = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY | BACKGROUND_RED;
		break;
	case ILogger::Severity::Error:
		newColorSettings = FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_INTENSITY;
		break;
	case ILogger::Severity::Warning:
		newColorSettings = FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY;
		break;
	case ILogger::Severity::Info:
		newColorSettings = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY;
		break;
	case ILogger::Severity::Debug:
		newColorSettings = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
		break;
	case ILogger::Severity::Trace:
		newColorSettings = FOREGROUND_GREEN | FOREGROUND_INTENSITY;
		break;
	}
	SetConsoleTextAttribute(_consoleHandle, newColorSettings);

	// Write the message to the console window
	DWORD charsWritten;
	if (scopeLength > 0)
	{
		std::wstring wscope = UTF8ToUTF16(std::string(scope, scopeLength));
		WriteConsole(_consoleHandle, wscope.c_str(), (DWORD)wscope.length(), &charsWritten, NULL);
		WriteConsole(_consoleHandle, L" : ", 3, &charsWritten, NULL);
	}
	std::wstring wmessage = UTF8ToUTF16(std::string(message, messageLength));
	WriteConsole(_consoleHandle, wmessage.c_str(), (DWORD)wmessage.length(), &charsWritten, NULL);
	WriteConsole(_consoleHandle, L"\n", 1, &charsWritten, NULL);

	// Restore the original console colour settings
	SetConsoleTextAttribute(_consoleHandle, consoleScreenBufferInfo.wAttributes);
}

//----------------------------------------------------------------------------------------------------------------------
std::wstring LogTargetConsole::UTF8ToUTF16(const std::string& stringUTF8)
{
	// Convert the encoding of the supplied string
	std::wstring stringUTF16;
	size_t sourceStringPos = 0;
	size_t sourceStringSize = stringUTF8.size();
	stringUTF16.reserve(sourceStringSize);
	while (sourceStringPos < sourceStringSize)
	{
		// Determine the number of code units required for the next character
		static const unsigned int codeUnitCountLookup[] = {1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 3, 4};
		unsigned int codeUnitCount = codeUnitCountLookup[(unsigned char)stringUTF8[sourceStringPos] >> 4];

		// Ensure that the requested number of code units are left in the source string
		if ((sourceStringPos + codeUnitCount) > sourceStringSize)
		{
			break;
		}

		// Convert the encoding of this character
		switch (codeUnitCount)
		{
		case 1:
		{
			stringUTF16.push_back((wchar_t)stringUTF8[sourceStringPos]);
			break;
		}
		case 2:
		{
			unsigned int unicodeCodePoint = (((unsigned int)stringUTF8[sourceStringPos] & 0x1F) << 6) | ((unsigned int)stringUTF8[sourceStringPos + 1] & 0x3F);
			stringUTF16.push_back((wchar_t)unicodeCodePoint);
			break;
		}
		case 3:
		{
			unsigned int unicodeCodePoint = (((unsigned int)stringUTF8[sourceStringPos] & 0x0F) << 12) | (((unsigned int)stringUTF8[sourceStringPos + 1] & 0x3F) << 6) | ((unsigned int)stringUTF8[sourceStringPos + 2] & 0x3F);
			stringUTF16.push_back((wchar_t)unicodeCodePoint);
			break;
		}
		case 4:
		{
			unsigned int unicodeCodePoint = (((unsigned int)stringUTF8[sourceStringPos] & 0x07) << 18) | (((unsigned int)stringUTF8[sourceStringPos + 1] & 0x3F) << 12) | (((unsigned int)stringUTF8[sourceStringPos + 2] & 0x3F) << 6) | ((unsigned int)stringUTF8[sourceStringPos + 3] & 0x3F);
			wchar_t convertedCodeUnit1 = 0xD800 | (((unicodeCodePoint - 0x10000) >> 10) & 0x03FF);
			wchar_t convertedCodeUnit2 = 0xDC00 | ((unicodeCodePoint - 0x10000) & 0x03FF);
			stringUTF16.push_back(convertedCodeUnit1);
			stringUTF16.push_back(convertedCodeUnit2);
			break;
		}
		}

		// Advance past the converted code units
		sourceStringPos += codeUnitCount;
	}

	// Return the converted string to the caller
	return stringUTF16;
}

}} // namespace cobalt::logging
