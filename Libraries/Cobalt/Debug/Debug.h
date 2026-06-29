// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include <string>
namespace cobalt { namespace debug {

// Optional setting to enable assertions even under release builds. Useful for pre-release test builds to detect code
// errors. You should define this through build settings rather than enabling it directly here.
//#define ENABLE_RELEASE_ASSERT

// Internal functions
namespace internal {
inline void AssertInternal(const char* fileName, size_t lineNumber, const char* expressionAsString, bool expression);
}

// Debug functions
inline constexpr bool IsDebug();
inline void Break();
inline void BreakOnce();

// Assertion macros
// If Debug mode, check an expression is true. If it's not, crash and attach the
// debugger. No cost in optimised.
#ifndef ASSERT
#if defined(_DEBUG) || defined(ENABLE_RELEASE_ASSERT)
// NOLINTNEXTLINE(readability-simplify-boolean-expr)
#define ASSERT(EXP) cobalt::debug::internal::AssertInternal(__FILE__, static_cast<size_t>(__LINE__), #EXP, static_cast<bool>(!!(EXP)))
#else
#define ASSERT(EXP) \
	do              \
	{               \
	} while (0)
#endif
#endif

// This section of code should never be reached. Triggers an assertion if execution reaches this point.
#ifndef UNREACHABLE
#define UNREACHABLE() ASSERT(0)
#endif

// Same as UNREACHABLE, but suitable for use in a constexpr function.
#ifndef UNREACHABLE_CONSTEXPR
#define UNREACHABLE_CONSTEXPR() assert(0)
#endif

}} // namespace cobalt::debug
#include "Debug.inl"
