// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#if defined(COBALT_CBINDINGS_STATIC)
#define COBALT_FUNCTION_EXPORT
#elif defined(_WIN32)
#if defined(BUILDING_COBALT_CBINDINGS)
#define COBALT_FUNCTION_EXPORT __declspec(dllexport)
#else
#define COBALT_FUNCTION_EXPORT __declspec(dllimport)
#endif
#else
#define COBALT_FUNCTION_EXPORT __attribute__((visibility("default")))
#endif
