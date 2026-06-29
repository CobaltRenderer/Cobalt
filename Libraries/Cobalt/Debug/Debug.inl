// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include <atomic>
#ifdef _WIN32
#include <intrin.h>
#else
#include <csignal>
#endif
#include "Warnings.h"
namespace cobalt { namespace debug {

namespace internal {
//----------------------------------------------------------------------------------------
void AssertInternal(const char* fileName, size_t lineNumber, const char* expressionAsString, bool expression)
{
	if (!expression)
	{
		cobalt::debug::Break();
	}
}
} // namespace internal

//----------------------------------------------------------------------------------------
constexpr bool IsDebug()
{
#ifdef _DEBUG
	return true;
#else
	return false;
#endif
}

//----------------------------------------------------------------------------------------
void Break()
{
#ifdef _WIN32
	__debugbreak();
#else
	(void)raise(SIGTRAP);
#endif
}

//----------------------------------------------------------------------------------------
void BreakOnce()
{
	// Attempt to break only the first time this function is called. Note that this will most likely only be effective
	// at tracking whether a break has occurred within a given calling module. This is acceptable for the intended use
	// of this method, which is to explicitly trigger a break when critical errors are reported or logged in central
	// areas of code, where there is likely to be a cascade of future errors being recorded at the same location, and
	// breaking on every one of them after the first may hinder debugging. In other cases, the Break function should be
	// used.
	WARNINGS_PUSH_OFF
	static std::atomic_flag hasBroken = ATOMIC_FLAG_INIT;
	WARNINGS_POP
	if (!hasBroken.test_and_set())
	{
		Break();
	}
}

}} // namespace cobalt::debug
