// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "CrashHandler.h"
#include <DbgHelp.h>
#include <algorithm>
#include <cstring>
namespace cobalt::debug {
// clang-format off
#ifdef __clang_analyzer__
inline std::wstring CrashHandler::GenerateExceptionReport(PEXCEPTION_POINTERS pExceptionInfo)
{
	return L"";
}
#else

//----------------------------------------------------------------------------------------
#ifdef _WIN64
#define FORMAT_POINTER L"%016I64X"
#else
#define FORMAT_POINTER L"%08X"
#endif

//----------------------------------------------------------------------------------------
// Report generation methods
//----------------------------------------------------------------------------------------
inline std::wstring CrashHandler::GetExceptionString(DWORD dwCode)
{
#define GetExceptionString_EXCEPTION(x) \
	case EXCEPTION_##x:                 \
		return L#x;

	switch (dwCode)
	{
		GetExceptionString_EXCEPTION(ACCESS_VIOLATION)
		GetExceptionString_EXCEPTION(DATATYPE_MISALIGNMENT)
		GetExceptionString_EXCEPTION(BREAKPOINT)
		GetExceptionString_EXCEPTION(SINGLE_STEP)
		GetExceptionString_EXCEPTION(ARRAY_BOUNDS_EXCEEDED)
		GetExceptionString_EXCEPTION(FLT_DENORMAL_OPERAND)
		GetExceptionString_EXCEPTION(FLT_DIVIDE_BY_ZERO)
		GetExceptionString_EXCEPTION(FLT_INEXACT_RESULT)
		GetExceptionString_EXCEPTION(FLT_INVALID_OPERATION)
		GetExceptionString_EXCEPTION(FLT_OVERFLOW)
		GetExceptionString_EXCEPTION(FLT_STACK_CHECK)
		GetExceptionString_EXCEPTION(FLT_UNDERFLOW)
		GetExceptionString_EXCEPTION(INT_DIVIDE_BY_ZERO)
		GetExceptionString_EXCEPTION(INT_OVERFLOW)
		GetExceptionString_EXCEPTION(PRIV_INSTRUCTION)
		GetExceptionString_EXCEPTION(IN_PAGE_ERROR)
		GetExceptionString_EXCEPTION(ILLEGAL_INSTRUCTION)
		GetExceptionString_EXCEPTION(NONCONTINUABLE_EXCEPTION)
		GetExceptionString_EXCEPTION(STACK_OVERFLOW)
		GetExceptionString_EXCEPTION(INVALID_DISPOSITION)
		GetExceptionString_EXCEPTION(GUARD_PAGE)
		GetExceptionString_EXCEPTION(INVALID_HANDLE)
	}

	// If not one of the "known" exceptions, try to get the string
	// from NTDLL.DLL's message table.
	wchar_t bufferUTF16[512] = {};
	FormatMessageW(FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_FROM_HMODULE, GetModuleHandleW(L"ntdll.dll"), dwCode, 0, bufferUTF16, sizeof(bufferUTF16) / sizeof(bufferUTF16[0]), 0);

#undef GetExceptionString_EXCEPTION
	return bufferUTF16;
}

//----------------------------------------------------------------------------------------
inline int __cdecl CrashHandler::AppendCrashReport(std::wstringstream& crashReport, const wchar_t* format, ...)
{
	wchar_t szBuff[1024];
	int retValue;
	va_list argptr;

	va_start(argptr, format);
	retValue = vswprintf(szBuff, 1024, format, argptr);
	va_end(argptr);

	crashReport << szBuff;

	return retValue;
}

//----------------------------------------------------------------------------------------
inline bool CrashHandler::GetContainingModuleInfo(void* targetAddress, ContainingModuleInfo& moduleInfo)
{
	moduleInfo.containsModuleInfo = false;
	moduleInfo.containsHeaderInfo = false;
	moduleInfo.containsAddressInfo = false;

	MEMORY_BASIC_INFORMATION mbi;
	if (!VirtualQuery(targetAddress, &mbi, sizeof(mbi)))
	{
		return false;
	}

	void* hMod = mbi.AllocationBase;

	wchar_t buffer[MAX_PATH + 1] = L"";
	if (GetModuleFileNameW((HMODULE)hMod, buffer, MAX_PATH) != 0)
	{
		moduleInfo.containsModuleInfo = true;
		moduleInfo.moduleName = buffer;
		moduleInfo.loadedBaseAddress = (ULONG_PTR)hMod;
	}

	// Point to the DOS header in memory
	PIMAGE_DOS_HEADER pDosHdr = (PIMAGE_DOS_HEADER)hMod;
	if ((pDosHdr == 0) || (pDosHdr->e_magic != 0x5A4D)) //MZ
	{
		return false;
	}

	// From the DOS header, find the NT (PE) header
	PIMAGE_NT_HEADERS pNtHdr = reinterpret_cast<PIMAGE_NT_HEADERS>((unsigned char*)hMod + pDosHdr->e_lfanew);
	if (pNtHdr->Signature != 0x00004550) //PE\0\0
	{
		return false;
	}
	PIMAGE_SECTION_HEADER pSection = IMAGE_FIRST_SECTION(pNtHdr);
	ULONG_PTR rva = (ULONG_PTR)((unsigned char*)targetAddress - (unsigned char*)hMod); // RVA is offset from module load address

	// Find the image timestamp and size fields from the headers. This is enough to retrieve the matching binary from
	// the symbol server.
	if (pNtHdr->FileHeader.SizeOfOptionalHeader >= 8)
	{
		moduleInfo.containsHeaderInfo = true;
		moduleInfo.imageTimestamp = pNtHdr->FileHeader.TimeDateStamp;
		moduleInfo.sizeOfImage = pNtHdr->OptionalHeader.SizeOfImage;
	}

	// Iterate through the section table, looking for the one that encompasses
	// the linear address.
	for (unsigned int i = 0; i < pNtHdr->FileHeader.NumberOfSections; ++i, ++pSection)
	{
		ULONG_PTR sectionStart = pSection->VirtualAddress;
		ULONG_PTR sectionEnd = sectionStart + std::max(pSection->SizeOfRawData, pSection->Misc.VirtualSize);

		// Is the address in this section???
		if ((rva >= sectionStart) && (rva <= sectionEnd))
		{
			// Yes, address is in the section.  Calculate section and offset,
			// and store in the "section" & "offset" params, which were
			// passed by reference.
			moduleInfo.containsAddressInfo = true;
			moduleInfo.section = i + 1;
			moduleInfo.offsetFromSection = rva - sectionStart;
			break;
		}
	}

	return true;
}

//----------------------------------------------------------------------------------------
inline std::wstring CrashHandler::GenerateExceptionReport(PEXCEPTION_POINTERS pExceptionInfo)
{
	// Start out with a banner
	std::wstringstream errorString;
	AppendCrashReport(errorString, L"\n//=====================================================\n");

	PEXCEPTION_RECORD pExceptionRecord = pExceptionInfo->ExceptionRecord;

	// First print information about the type of fault
	AppendCrashReport(errorString, L"Exception code: %08X %s\n", pExceptionRecord->ExceptionCode, GetExceptionString(pExceptionRecord->ExceptionCode).c_str());
	if (pExceptionRecord->ExceptionInformation[0] == 0)
	{
		AppendCrashReport(errorString, L"Error Reading Memory: " FORMAT_POINTER L"\n", pExceptionRecord->ExceptionInformation[1]);
	}
	else if (pExceptionRecord->ExceptionInformation[0] == 1)
	{
		AppendCrashReport(errorString, L"Error Writing Memory: " FORMAT_POINTER L"\n", pExceptionRecord->ExceptionInformation[1]);
	}

	// Now print information about where the fault occurred
	ContainingModuleInfo moduleInfo;
	GetContainingModuleInfo(pExceptionRecord->ExceptionAddress, moduleInfo);
	DWORD section = (moduleInfo.containsAddressInfo ? moduleInfo.section : 0);
	ULONG_PTR offsetFromSection = (moduleInfo.containsAddressInfo ? moduleInfo.offsetFromSection : 0);
	ULONG_PTR offsetFromBase = (moduleInfo.containsModuleInfo ? (ULONG_PTR)pExceptionRecord->ExceptionAddress - moduleInfo.loadedBaseAddress : 0);
	AppendCrashReport(errorString, L"Fault address: " FORMAT_POINTER L" " FORMAT_POINTER L"|%04X:" FORMAT_POINTER, (ULONG_PTR)pExceptionRecord->ExceptionAddress, offsetFromBase, section, offsetFromSection);

	if (moduleInfo.containsModuleInfo)
	{
		AppendCrashReport(errorString, L" %s", moduleInfo.moduleName.c_str());
		if (moduleInfo.containsHeaderInfo)
		{
			AppendCrashReport(errorString, L"|%08X|%08X", moduleInfo.imageTimestamp, moduleInfo.sizeOfImage);
		}
	}
	AppendCrashReport(errorString, L"\n");

	PCONTEXT pCtx = pExceptionInfo->ContextRecord;

	// Show the registers
#ifdef _M_AMD64
	AppendCrashReport(errorString, L"\nRegisters:\n");

	AppendCrashReport(errorString, L"RIP:" FORMAT_POINTER L"\n", pCtx->Rip);

	AppendCrashReport(errorString, L"RAX:" FORMAT_POINTER L" RCX:" FORMAT_POINTER L" RDX:" FORMAT_POINTER L" RBX:" FORMAT_POINTER L"\n", pCtx->Rax, pCtx->Rcx, pCtx->Rdx, pCtx->Rbx);
	AppendCrashReport(errorString, L"RSP:" FORMAT_POINTER L" RBP:" FORMAT_POINTER L" RSI:" FORMAT_POINTER L" RDI:" FORMAT_POINTER L"\n", pCtx->Rsp, pCtx->Rbp, pCtx->Rsi, pCtx->Rdi);
	AppendCrashReport(errorString, L"R8: " FORMAT_POINTER L" R9: " FORMAT_POINTER L" R10:" FORMAT_POINTER L" R11:" FORMAT_POINTER L"\n", pCtx->R8, pCtx->R9, pCtx->R10, pCtx->R11);
	AppendCrashReport(errorString, L"R12:" FORMAT_POINTER L" R13:" FORMAT_POINTER L" R14:" FORMAT_POINTER L" R15:" FORMAT_POINTER L"\n", pCtx->R12, pCtx->R13, pCtx->R14, pCtx->R15);

	AppendCrashReport(errorString, L"CS:%04X SS:%04X DS:%04X ES:%04X FS:%04X GS:%04X\n", pCtx->SegCs, pCtx->SegSs, pCtx->SegDs, pCtx->SegEs, pCtx->SegFs, pCtx->SegGs);

	AppendCrashReport(errorString, L"Flags:%08X\n", pCtx->EFlags);
#elif defined _M_ARM64
	AppendCrashReport(errorString, L"\nRegisters:\n");

	AppendCrashReport(errorString, L"PC:" FORMAT_POINTER L" LR:" FORMAT_POINTER L"\n", pCtx->Pc, pCtx->Lr);
	AppendCrashReport(errorString, L"SP:" FORMAT_POINTER L" FP:" FORMAT_POINTER L"\n", pCtx->Sp, pCtx->Fp);
	AppendCrashReport(errorString, L"X0: " FORMAT_POINTER L" X1: " FORMAT_POINTER L" X2: " FORMAT_POINTER L" X3: " FORMAT_POINTER L"\n", pCtx->X0, pCtx->X1, pCtx->X2, pCtx->X3);
	AppendCrashReport(errorString, L"X4: " FORMAT_POINTER L" X5: " FORMAT_POINTER L" X6: " FORMAT_POINTER L" X7: " FORMAT_POINTER L"\n", pCtx->X4, pCtx->X5, pCtx->X6, pCtx->X7);
	AppendCrashReport(errorString, L"X8: " FORMAT_POINTER L" X9: " FORMAT_POINTER L" X10:" FORMAT_POINTER L" X11:" FORMAT_POINTER L"\n", pCtx->X8, pCtx->X9, pCtx->X10, pCtx->X11);
	AppendCrashReport(errorString, L"X12:" FORMAT_POINTER L" X13:" FORMAT_POINTER L" X14:" FORMAT_POINTER L" X15:" FORMAT_POINTER L"\n", pCtx->X12, pCtx->X13, pCtx->X14, pCtx->X15);
	AppendCrashReport(errorString, L"X16:" FORMAT_POINTER L" X17:" FORMAT_POINTER L" X18:" FORMAT_POINTER L" X19:" FORMAT_POINTER L"\n", pCtx->X16, pCtx->X17, pCtx->X18, pCtx->X19);
	AppendCrashReport(errorString, L"X20:" FORMAT_POINTER L" X21:" FORMAT_POINTER L" X22:" FORMAT_POINTER L" X23:" FORMAT_POINTER L"\n", pCtx->X20, pCtx->X21, pCtx->X22, pCtx->X23);
	AppendCrashReport(errorString, L"X24:" FORMAT_POINTER L" X25:" FORMAT_POINTER L" X26:" FORMAT_POINTER L" X27:" FORMAT_POINTER L"\n", pCtx->X24, pCtx->X25, pCtx->X26, pCtx->X27);
	AppendCrashReport(errorString, L"X28:" FORMAT_POINTER L"\n", pCtx->X28);

	AppendCrashReport(errorString, L"CPSR:%08X\n", pCtx->Cpsr);
#elif defined _M_IX86
	AppendCrashReport(errorString, L"\nRegisters:\n");

	AppendCrashReport(errorString, L"EAX:" FORMAT_POINTER L"\nEBX:" FORMAT_POINTER L"\nECX:" FORMAT_POINTER L"\nEDX:" FORMAT_POINTER L"\nESI:" FORMAT_POINTER L"\nEDI:" FORMAT_POINTER L"\n", pCtx->Eax, pCtx->Ebx, pCtx->Ecx, pCtx->Edx, pCtx->Esi, pCtx->Edi);

	AppendCrashReport(errorString, L"CS:EIP:%04X:%08X\n", pCtx->SegCs, pCtx->Eip);
	AppendCrashReport(errorString, L"SS:ESP:%04X:%08X  EBP:%08X\n", pCtx->SegSs, pCtx->Esp, pCtx->Ebp);
	AppendCrashReport(errorString, L"DS:%04X  ES:%04X  FS:%04X  GS:%04X\n", pCtx->SegDs, pCtx->SegEs, pCtx->SegFs, pCtx->SegGs);
	AppendCrashReport(errorString, L"Flags:%08X\n", pCtx->EFlags);
#endif

	SymSetOptions(SYMOPT_DEFERRED_LOADS);

	// Initialize DbgHelp
	if (!SymInitialize(GetCurrentProcess(), 0, TRUE))
	{
		return errorString.str();
	}

	CONTEXT trashableContext = *pCtx;
	WriteStackDetails(errorString, &trashableContext);

	SymCleanup(GetCurrentProcess());

	AppendCrashReport(errorString, L"\n");
	return errorString.str();
}

//----------------------------------------------------------------------------------------
inline void CrashHandler::WriteStackDetails(std::wstringstream& crashReport, PCONTEXT pContext)
{
	AppendCrashReport(crashReport, L"\nCall stack:\n");

#if defined(_M_AMD64) || defined(_M_ARM64)
	AppendCrashReport(crashReport, L"FV Addr-EIP          RetAddr_          FramePtr          StackPtr          Function                                  SourceFile\n");
#else
	AppendCrashReport(crashReport, L"FV Addr-EIP  RetAddr_  FramePtr  StackPtr  Function                     SourceFile\n");
#endif

	DWORD dwMachineType = 0;
	// Could use SymSetOptions here to add the SYMOPT_DEFERRED_LOADS flag

#if defined(_M_AMD64) || defined(_M_ARM64)
	STACKFRAME64 sf;
#else
	STACKFRAME sf;
#endif
	std::memset(&sf, 0, sizeof(sf));

	// Initialize the STACKFRAME structure for the first call.  This is only
	// necessary for Intel CPUs, and isn't mentioned in the documentation.
#ifdef _M_AMD64
	sf.AddrPC.Offset = pContext->Rip;
	sf.AddrFrame.Offset = pContext->Rbp;
	sf.AddrStack.Offset = pContext->Rsp;
#elif defined _M_ARM64
	sf.AddrPC.Offset = pContext->Pc;
	sf.AddrFrame.Offset = pContext->Fp;
	sf.AddrStack.Offset = pContext->Sp;
#elif defined _M_IX86
	sf.AddrPC.Offset = pContext->Eip;
	sf.AddrFrame.Offset = pContext->Ebp;
	sf.AddrStack.Offset = pContext->Esp;
#endif
	sf.AddrPC.Mode = AddrModeFlat;
	sf.AddrStack.Mode = AddrModeFlat;
	sf.AddrFrame.Mode = AddrModeFlat;

#ifdef _M_AMD64
	dwMachineType = IMAGE_FILE_MACHINE_AMD64;
#elif defined _M_ARM64
	dwMachineType = IMAGE_FILE_MACHINE_ARM64;
#elif defined _M_IX86
	dwMachineType = IMAGE_FILE_MACHINE_I386;
#endif

	auto processHandle = GetCurrentProcess();
	if (dwMachineType == 0)
	{
		AppendCrashReport(crashReport, L"Stack walking isn't supported for this target architecture.\n");
		return;
	}
	while (true)
	{
		// Get the next stack frame
#if defined(_M_AMD64) || defined(_M_ARM64)
		if (!StackWalk64(dwMachineType, processHandle, GetCurrentThread(), &sf, pContext, 0, SymFunctionTableAccess64, SymGetModuleBase64, 0))
#else
		if (!StackWalk(dwMachineType, processHandle, GetCurrentThread(), &sf, pContext, 0, SymFunctionTableAccess, SymGetModuleBase, 0))
#endif
		{
			break;
		}

		// Basic sanity check to make sure the frame is OK. Bail if not.
		if (sf.AddrFrame.Offset == 0)
		{
			break;
		}

		AppendCrashReport(crashReport, L"%c%c " FORMAT_POINTER L"  " FORMAT_POINTER L"  " FORMAT_POINTER L"  " FORMAT_POINTER L"  ", sf.Far ? L'F' : L'.', sf.Virtual ? L'V' : L'.', sf.AddrPC.Offset, sf.AddrReturn.Offset, sf.AddrFrame.Offset, sf.AddrStack.Offset);

		// Get the name of the function for this stack frame entry
		constexpr DWORD maxSymbolNameLength = 1024;
		struct SymbolInfoBuffer
		{
			SYMBOL_INFO symbolInfo;
			CHAR symbolName[1024];
		};
		SymbolInfoBuffer symbolBuffer = {};
		PSYMBOL_INFO pSymbol = &symbolBuffer.symbolInfo;
		pSymbol->SizeOfStruct = sizeof(SYMBOL_INFO);
		pSymbol->MaxNameLen = maxSymbolNameLength;

		// Print out the logical address
		ContainingModuleInfo moduleInfo;
		if (sf.AddrPC.Offset != 0)
		{
			GetContainingModuleInfo((void*)sf.AddrPC.Offset, moduleInfo);
		}
		DWORD section = (moduleInfo.containsAddressInfo ? moduleInfo.section : 0);
		DWORD64 offsetFromSection = (moduleInfo.containsAddressInfo ? moduleInfo.offsetFromSection : 0);
		DWORD64 offsetFromBase = (moduleInfo.containsModuleInfo ? (DWORD64)sf.AddrPC.Offset - moduleInfo.loadedBaseAddress : 0);

		// printout the logical address - this is more useful than the symbol
		// and offset if there is no debug in the module ( i.e. release builds )
		AppendCrashReport(crashReport, FORMAT_POINTER L"|%04X:" FORMAT_POINTER L" ", offsetFromBase, section, offsetFromSection);

		// if we can determine a symbol, lets print that too.
		DWORD64 symDisplacement = 0; // Displacement of the input address,
		                             // relative to the start of the symbol
		if (SymFromAddr(processHandle, sf.AddrPC.Offset, &symDisplacement, pSymbol))
		{
			AppendCrashReport(crashReport, L"- %hs", pSymbol->Name);
			if (symDisplacement > 0)
			{
				AppendCrashReport(crashReport, L"+%I64X", symDisplacement);
			}
		}

		// Get the source line for this stack frame entry
		IMAGEHLP_LINEW64 lineInfo = {};
		lineInfo.SizeOfStruct = sizeof(IMAGEHLP_LINEW64);
		DWORD dwLineDisplacement;
		if (SymGetLineFromAddrW64(processHandle, sf.AddrPC.Offset, &dwLineDisplacement, &lineInfo))
		{
			AppendCrashReport(crashReport, L"  %s line %u", lineInfo.FileName, lineInfo.LineNumber);
		}

		// And finally write the module name and header info required to retrieve the image from a symbol server
		// via SymFindFileInPath.
		if (moduleInfo.containsModuleInfo)
		{
			AppendCrashReport(crashReport, L"  [ %s", moduleInfo.moduleName.c_str());
			if (moduleInfo.containsHeaderInfo)
			{
				AppendCrashReport(crashReport, L"|%08X|%08X", moduleInfo.imageTimestamp, moduleInfo.sizeOfImage);
			}
			AppendCrashReport(crashReport, L" ]");
		}
		AppendCrashReport(crashReport, L"\n");
	}
}

//----------------------------------------------------------------------------------------
#undef FORMAT_POINTER

#endif
// clang-format on
} // namespace cobalt::debug
