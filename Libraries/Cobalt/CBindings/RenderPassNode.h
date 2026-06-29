// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include "DefaultState.h"
#include "FrameBuffer.h"
#include "Macros.h"
#include "ProgramNode.h"
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Cobalt_RenderPassNode_Internal* Cobalt_RenderPassNode;

// Enumerations
typedef enum
{
	Cobalt_AttachmentLoadBehavior_LoadExistingData,
	Cobalt_AttachmentLoadBehavior_UndefinedInitialData,
} Cobalt_AttachmentLoadBehavior;

typedef enum
{
	Cobalt_AttachmentStoreBehavior_StoreFinalData,
	Cobalt_AttachmentStoreBehavior_UndefinedFinalData,
} Cobalt_AttachmentStoreBehavior;

// Child node methods
COBALT_FUNCTION_EXPORT void Cobalt_RenderPassNode_AddChildNode(Cobalt_RenderPassNode renderPassNode, Cobalt_ProgramNode childNode, Cobalt_DefaultState defaultState);
COBALT_FUNCTION_EXPORT void Cobalt_RenderPassNode_AddChildNodes(Cobalt_RenderPassNode renderPassNode, const Cobalt_ProgramNode* childNodes, size_t childNodeCount, const Cobalt_DefaultState* defaultState);
COBALT_FUNCTION_EXPORT void Cobalt_RenderPassNode_RemoveChildNode(Cobalt_RenderPassNode renderPassNode, Cobalt_ProgramNode childNode);
COBALT_FUNCTION_EXPORT void Cobalt_RenderPassNode_RemoveChildNodes(Cobalt_RenderPassNode renderPassNode, const Cobalt_ProgramNode* childNodes, size_t childNodeCount);
COBALT_FUNCTION_EXPORT void Cobalt_RenderPassNode_RemoveAllChildNodes(Cobalt_RenderPassNode renderPassNode);
COBALT_FUNCTION_EXPORT void Cobalt_RenderPassNode_SetChildNodes(Cobalt_RenderPassNode renderPassNode, const Cobalt_ProgramNode* childNodes, size_t childNodeCount, const Cobalt_DefaultState* defaultState);

// Framebuffer methods
COBALT_FUNCTION_EXPORT void Cobalt_RenderPassNode_BindFrameBuffer(Cobalt_RenderPassNode renderPassNode, Cobalt_FrameBuffer frameBuffer);
COBALT_FUNCTION_EXPORT void Cobalt_RenderPassNode_SetAttachmentLoadStoreBehavior(Cobalt_RenderPassNode renderPassNode, Cobalt_AttachmentType type, size_t index, Cobalt_AttachmentLoadBehavior loadBehavior, Cobalt_AttachmentStoreBehavior storeBehavior);
COBALT_FUNCTION_EXPORT void Cobalt_RenderPassNode_SetAttachmentClearDataF(Cobalt_RenderPassNode renderPassNode, Cobalt_AttachmentType type, size_t index, const float data[4]);
COBALT_FUNCTION_EXPORT void Cobalt_RenderPassNode_SetAttachmentClearDataI(Cobalt_RenderPassNode renderPassNode, Cobalt_AttachmentType type, size_t index, const int32_t data[4]);
COBALT_FUNCTION_EXPORT void Cobalt_RenderPassNode_SetAttachmentClearDataU(Cobalt_RenderPassNode renderPassNode, Cobalt_AttachmentType type, size_t index, const uint32_t data[4]);

COBALT_FUNCTION_EXPORT void Cobalt_RenderPassNode_RemoveAttachmentClearData(Cobalt_RenderPassNode renderPassNode, Cobalt_AttachmentType type, size_t index);
COBALT_FUNCTION_EXPORT void Cobalt_RenderPassNode_EnableAttachmentMultiSamplingResolution(Cobalt_RenderPassNode renderPassNode, Cobalt_AttachmentType type, size_t index, size_t resolveAttachmentIndex);
COBALT_FUNCTION_EXPORT void Cobalt_RenderPassNode_DisableAttachmentMultiSamplingResolution(Cobalt_RenderPassNode renderPassNode, Cobalt_AttachmentType type, size_t index);

// Enabled state methods
COBALT_FUNCTION_EXPORT void Cobalt_RenderPassNode_SetIsEnabled(Cobalt_RenderPassNode renderPassNode, char state);

// Debug methods
COBALT_FUNCTION_EXPORT void Cobalt_RenderPassNode_SetDebugName(Cobalt_RenderPassNode renderPassNode, const char* name, size_t nameLength);

COBALT_FUNCTION_EXPORT void Cobalt_RenderPassNode_Delete(Cobalt_RenderPassNode renderPassNode);

#ifdef __cplusplus
}
#endif
