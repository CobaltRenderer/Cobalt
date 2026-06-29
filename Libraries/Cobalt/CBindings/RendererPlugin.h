// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include "GraphicsDevice.h"
#include "GraphicsDeviceEnumerator.h"
#include "Macros.h"
#include "Result.h"
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Cobalt_Library_Internal* Cobalt_Library;
typedef struct Cobalt_RendererPlugin_Internal* Cobalt_RendererPlugin;

// Enumerations
typedef enum
{
	Cobalt_ApiFamily_OpenGL,
	Cobalt_ApiFamily_OpenGLES,
	Cobalt_ApiFamily_Direct3D,
	Cobalt_ApiFamily_Vulkan,
	Cobalt_ApiFamily_Metal,
} Cobalt_ApiFamily;

typedef enum
{
	Cobalt_LogSeverity_Critical = 0x01, // Fatal error or application crash
	Cobalt_LogSeverity_Error = 0x02,    // Recoverable error
	Cobalt_LogSeverity_Warning = 0x04,  // Noncritical problem
	Cobalt_LogSeverity_Info = 0x08,     // Informational message
	Cobalt_LogSeverity_Debug = 0x10,    // Debugging info. Logged to file but not displayed by default.
	Cobalt_LogSeverity_Trace = 0x20,    // Trace info. Not recorded by default.
} Cobalt_LogSeverity;

typedef enum
{
	Cobalt_LogSeverityFilter_None = 0,
	Cobalt_LogSeverityFilter_CriticalOrHigher = (uint32_t)Cobalt_LogSeverity_Critical,
	Cobalt_LogSeverityFilter_ErrorOrHigher = (uint32_t)Cobalt_LogSeverity_Critical | (uint32_t)Cobalt_LogSeverity_Error,
	Cobalt_LogSeverityFilter_WarningOrHigher = (uint32_t)Cobalt_LogSeverity_Critical | (uint32_t)Cobalt_LogSeverity_Error | (uint32_t)Cobalt_LogSeverity_Warning,
	Cobalt_LogSeverityFilter_InfoOrHigher = (uint32_t)Cobalt_LogSeverity_Critical | (uint32_t)Cobalt_LogSeverity_Error | (uint32_t)Cobalt_LogSeverity_Warning | (uint32_t)Cobalt_LogSeverity_Info,
	Cobalt_LogSeverityFilter_DebugOrHigher = (uint32_t)Cobalt_LogSeverity_Critical | (uint32_t)Cobalt_LogSeverity_Error | (uint32_t)Cobalt_LogSeverity_Warning | (uint32_t)Cobalt_LogSeverity_Info | (uint32_t)Cobalt_LogSeverity_Debug,
	Cobalt_LogSeverityFilter_TraceOrHigher = (uint32_t)Cobalt_LogSeverity_Critical | (uint32_t)Cobalt_LogSeverity_Error | (uint32_t)Cobalt_LogSeverity_Warning | (uint32_t)Cobalt_LogSeverity_Info | (uint32_t)Cobalt_LogSeverity_Debug | (uint32_t)Cobalt_LogSeverity_Trace,
	Cobalt_LogSeverityFilter_Critical = (uint32_t)Cobalt_LogSeverity_Critical,
	Cobalt_LogSeverityFilter_Error = (uint32_t)Cobalt_LogSeverity_Error,
	Cobalt_LogSeverityFilter_Warning = (uint32_t)Cobalt_LogSeverity_Warning,
	Cobalt_LogSeverityFilter_Info = (uint32_t)Cobalt_LogSeverity_Info,
	Cobalt_LogSeverityFilter_Trace = (uint32_t)Cobalt_LogSeverity_Trace,
	Cobalt_LogSeverityFilter_Debug = (uint32_t)Cobalt_LogSeverity_Debug,
	Cobalt_LogSeverityFilter_All = Cobalt_LogSeverityFilter_TraceOrHigher,
} Cobalt_LogSeverityFilter;

// Structures
typedef struct Cobalt_Version
{
	uint32_t major;
	uint32_t minor;
} Cobalt_Version;

typedef void (*Cobalt_LogCallback)(Cobalt_LogSeverity severity, const char* scope, size_t scopeLength, const char* message, size_t messageLength);

COBALT_FUNCTION_EXPORT Cobalt_Result Cobalt_Initialize(Cobalt_LogCallback logCallback, Cobalt_LogSeverityFilter logFilter, Cobalt_Library* library);
COBALT_FUNCTION_EXPORT void Cobalt_Terminate(Cobalt_Library library);

COBALT_FUNCTION_EXPORT Cobalt_Result Cobalt_GetRendererPlugin(Cobalt_Library library, void* moduleHandle, unsigned int index, Cobalt_RendererPlugin* plugin);
COBALT_FUNCTION_EXPORT Cobalt_Result Cobalt_GetRendererPluginStatic(Cobalt_Library library, void* getRendererPluginFunction, Cobalt_RendererPlugin* plugin);

COBALT_FUNCTION_EXPORT Cobalt_ApiFamily Cobalt_RendererPlugin_GetApiFamily(Cobalt_RendererPlugin plugin);
COBALT_FUNCTION_EXPORT void Cobalt_RendererPlugin_GetTargetApiVersion(Cobalt_RendererPlugin plugin, Cobalt_Version* version);
COBALT_FUNCTION_EXPORT void Cobalt_RendererPlugin_GetName(Cobalt_RendererPlugin plugin, char* name, size_t* nameLength);
COBALT_FUNCTION_EXPORT void Cobalt_RendererPlugin_GetDisplayName(Cobalt_RendererPlugin plugin, char* name, size_t* nameLength);

COBALT_FUNCTION_EXPORT void Cobalt_RendererPlugin_CreateGraphicsDeviceEnumerator(Cobalt_RendererPlugin plugin, Cobalt_GraphicsDeviceEnumerator* enumerator);

COBALT_FUNCTION_EXPORT void Cobalt_RendererPlugin_Delete(Cobalt_RendererPlugin plugin);

#ifdef __cplusplus
}
#endif
