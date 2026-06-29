// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#include <sstream>
#include <string>
namespace cobalt { namespace debug {

class CrashHandler
{
public:
	// Report generation methods
	inline static std::wstring GenerateExceptionReport(PEXCEPTION_POINTERS pExceptionInfo);

private:
	// Structures
	struct ContainingModuleInfo
	{
		bool containsModuleInfo = false;
		std::wstring moduleName;
		ULONG_PTR loadedBaseAddress = 0;
		bool containsAddressInfo = false;
		DWORD section = 0;
		ULONG_PTR offsetFromSection = 0;
		bool containsHeaderInfo = false;
		DWORD imageTimestamp = 0;
		DWORD sizeOfImage = 0;
	};

private:
	// Report generation methods
	inline static std::wstring GetExceptionString(DWORD dwCode);
	inline static int __cdecl AppendCrashReport(std::wstringstream& crashReport, const wchar_t* format, ...);
	inline static bool GetContainingModuleInfo(void* targetAddress, ContainingModuleInfo& moduleInfo);
	inline static void WriteStackDetails(std::wstringstream& crashReport, PCONTEXT pContext);
};

}} // namespace cobalt::debug
#include "CrashHandler.inl"
#endif
