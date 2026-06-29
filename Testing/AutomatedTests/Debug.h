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
#endif

inline bool IsDebugBuildWithDebuggerAttached()
{
#ifdef _WIN32
#ifdef _DEBUG
	return IsDebuggerPresent() == TRUE;
#else
	return false;
#endif
#else
	return false;
#endif
}
