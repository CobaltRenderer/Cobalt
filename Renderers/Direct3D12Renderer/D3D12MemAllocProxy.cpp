// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
// RJS - Since this file is just a proxy for an external file, we exclude it from analysis.
#ifndef __clang_analyzer__
#include "Direct3DHeaders.h"
// This needs to be after Direct3DHeaders.h so we have our defines for Windows.h set properly
#include "D3D12MemAlloc.h"
#include <Cobalt/Debug/Debug.pkg>
WARNINGS_PUSH_OFF
#ifdef _MSC_VER
#pragma warning(disable : 6386)
#pragma warning(disable : 6387)
#pragma warning(disable : 26110)
#pragma warning(disable : 26819)
#endif
#include "D3D12MemAlloc.cpp" // NOLINT(bugprone-suspicious-include)
WARNINGS_POP
#endif
