// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once

#ifdef _WIN32
#ifdef IMAGEDIFF_DLL
#define IMAGEDIFF_EXPORT __declspec(dllexport)
#else
#define IMAGEDIFF_EXPORT __declspec(dllimport)
#endif
#else
#define IMAGEDIFF_EXPORT
#endif
