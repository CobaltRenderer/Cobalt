// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include "FrameBufferOutput.h"
#include "Macros.h"
#include "Result.h"
#include "TextureBuffer2D.h"
#include "WindowInfoBase.h"
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Cobalt_FrameBuffer_Internal* Cobalt_FrameBuffer;

// Enumerations
typedef enum
{
	Cobalt_AttachmentType_Color,
	Cobalt_AttachmentType_Depth,
	Cobalt_AttachmentType_Stencil
} Cobalt_AttachmentType;

typedef enum
{
	Cobalt_WindowDepthStencilMode_None,
	Cobalt_WindowDepthStencilMode_DepthUNorm16,
	Cobalt_WindowDepthStencilMode_DepthUNorm24,
	Cobalt_WindowDepthStencilMode_DepthUNorm24StencilUInt8,
	Cobalt_WindowDepthStencilMode_DepthFloat32,
	Cobalt_WindowDepthStencilMode_DepthFloat32StencilUInt8
} Cobalt_WindowDepthStencilMode;

typedef enum
{
	Cobalt_WindowColorSpaceMode_Default
} Cobalt_WindowColorSpaceMode;

typedef enum
{
	Cobalt_WindowBindingFlags_None = 0,
	Cobalt_WindowBindingFlags_LimitSwapToVSync = 0x00000001,
	Cobalt_WindowBindingFlags_AllowTearing = 0x00000002
} Cobalt_WindowBindingFlags;

// Binding methods
COBALT_FUNCTION_EXPORT Cobalt_Result Cobalt_FrameBuffer_BindTexture(Cobalt_FrameBuffer frameBuffer, Cobalt_TextureBuffer2D texture, Cobalt_AttachmentType type, size_t index);
COBALT_FUNCTION_EXPORT void Cobalt_FrameBuffer_UnbindTexture(Cobalt_FrameBuffer frameBuffer, Cobalt_AttachmentType type, size_t index);
COBALT_FUNCTION_EXPORT Cobalt_Result Cobalt_FrameBuffer_BindMultiSamplingResolveTexture(Cobalt_FrameBuffer frameBuffer, Cobalt_TextureBuffer2D texture, Cobalt_AttachmentType type, size_t index);
COBALT_FUNCTION_EXPORT void Cobalt_FrameBuffer_UnbindMultiSamplingResolveTexture(Cobalt_FrameBuffer frameBuffer, Cobalt_AttachmentType type, size_t index);
COBALT_FUNCTION_EXPORT Cobalt_Result Cobalt_FrameBuffer_BindWindow(Cobalt_FrameBuffer frameBuffer, const Cobalt_WindowInfoBase* windowInfo, Cobalt_WindowDepthStencilMode depthStencilMode, Cobalt_WindowColorSpaceMode colorSpaceMode, Cobalt_WindowBindingFlags bindingFlags);
COBALT_FUNCTION_EXPORT Cobalt_Result Cobalt_FrameBuffer_NotifyWindowResized(Cobalt_FrameBuffer frameBuffer, const uint32_t windowSizeInPixels[2]);

// Viewport methods
COBALT_FUNCTION_EXPORT void Cobalt_FrameBuffer_DefineViewportRegion(Cobalt_FrameBuffer frameBuffer, const uint32_t startPos[2], const uint32_t size[2]);
COBALT_FUNCTION_EXPORT void Cobalt_FrameBuffer_DefineScissorRegion(Cobalt_FrameBuffer frameBuffer, const uint32_t startPos[2], const uint32_t size[2]);
COBALT_FUNCTION_EXPORT void Cobalt_FrameBuffer_RemoveScissorRegion(Cobalt_FrameBuffer frameBuffer);

// Output capture methods
COBALT_FUNCTION_EXPORT void Cobalt_FrameBuffer_AddOutputCaptureTarget(Cobalt_FrameBuffer frameBuffer, Cobalt_FrameBufferOutput captureTarget, Cobalt_AttachmentType type, size_t index);
COBALT_FUNCTION_EXPORT void Cobalt_FrameBuffer_RemoveOutputCaptureTarget(Cobalt_FrameBuffer frameBuffer, Cobalt_FrameBufferOutput captureTarget);

COBALT_FUNCTION_EXPORT void Cobalt_FrameBuffer_Delete(Cobalt_FrameBuffer frameBuffer);

#ifdef __cplusplus
}
#endif
