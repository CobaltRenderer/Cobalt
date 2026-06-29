// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include "DataArray.h"
#include "DataArrayOutput.h"
#include "DefaultState.h"
#include "FrameBuffer.h"
#include "FrameBufferOutput.h"
#include "IndexAttribute.h"
#include "IndexBuffer.h"
#include "Macros.h"
#include "ProgramNode.h"
#include "RenderPassNode.h"
#include "RenderableNode.h"
#include "Result.h"
#include "ShaderProgram.h"
#include "StateBuffer.h"
#include "StateBufferLayout.h"
#include "StateContainer.h"
#include "StateGroupNode.h"
#include "TexelArray.h"
#include "TexelArrayOutput.h"
#include "TextureBuffer1D.h"
#include "TextureBuffer1DArray.h"
#include "TextureBuffer2D.h"
#include "TextureBuffer2DArray.h"
#include "TextureBuffer3D.h"
#include "TextureBufferCube.h"
#include "TextureBufferCubeArray.h"
#include "TextureSampler1D.h"
#include "TextureSampler1DArray.h"
#include "TextureSampler2D.h"
#include "TextureSampler2DArray.h"
#include "TextureSampler3D.h"
#include "TextureSamplerCube.h"
#include "TextureSamplerCubeArray.h"
#include "TransferBatch.h"
#include "VertexAttribute.h"
#include "VertexBuffer.h"
#include "WindowSystemInfoBase.h"
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Cobalt_Renderer_Internal* Cobalt_Renderer;

// Enumerations
typedef enum
{
	Cobalt_RendererOption_EnableDebugLogging,
	Cobalt_RendererOption_EnableRenderMarkers
} Cobalt_RendererOption;

typedef enum
{
	Cobalt_RendererInitializationFlags_None = 0
} Cobalt_RendererInitializationFlags;

// Initialization methods
COBALT_FUNCTION_EXPORT Cobalt_Result Cobalt_Renderer_Initialize(Cobalt_Renderer renderer, const Cobalt_WindowSystemInfoBase* windowSystemInfo, Cobalt_RendererInitializationFlags flags);

// Geometry buffer methods
COBALT_FUNCTION_EXPORT void Cobalt_Renderer_CreateVertexBuffer(Cobalt_Renderer renderer, Cobalt_VertexBuffer* vertexBuffer);
COBALT_FUNCTION_EXPORT void Cobalt_Renderer_CreateIndexBuffer(Cobalt_Renderer renderer, Cobalt_IndexBuffer* indexBuffer);
COBALT_FUNCTION_EXPORT void Cobalt_Renderer_CreateVertexAttribute(Cobalt_Renderer renderer, Cobalt_VertexAttribute* vertexAttribute, Cobalt_VertexAttributeType type, size_t elementCount, size_t vertexCount, Cobalt_VertexPerformanceHint performanceHintCpu, Cobalt_VertexPerformanceHint performanceHintGpu, Cobalt_VertexDataPersistenceFlags dataPersistenceFlags);
COBALT_FUNCTION_EXPORT void Cobalt_Renderer_CreateIndexAttribute(Cobalt_Renderer renderer, Cobalt_IndexAttribute* indexAttribute, Cobalt_IndexAttributeType type, size_t indexCount, Cobalt_IndexPerformanceHint performanceHintCpu, Cobalt_IndexPerformanceHint performanceHintGpu, Cobalt_IndexDataPersistenceFlags dataPersistenceFlags);

// Image buffer methods
COBALT_FUNCTION_EXPORT void Cobalt_Renderer_CreateTextureBuffer1D(Cobalt_Renderer renderer, Cobalt_TextureBuffer1D* texture);
COBALT_FUNCTION_EXPORT void Cobalt_Renderer_CreateTextureBuffer2D(Cobalt_Renderer renderer, Cobalt_TextureBuffer2D* texture);
COBALT_FUNCTION_EXPORT void Cobalt_Renderer_CreateTextureBuffer3D(Cobalt_Renderer renderer, Cobalt_TextureBuffer3D* texture);
COBALT_FUNCTION_EXPORT void Cobalt_Renderer_CreateTextureBufferCube(Cobalt_Renderer renderer, Cobalt_TextureBufferCube* texture);
COBALT_FUNCTION_EXPORT void Cobalt_Renderer_CreateTextureBuffer1DArray(Cobalt_Renderer renderer, Cobalt_TextureBuffer1DArray* texture);
COBALT_FUNCTION_EXPORT void Cobalt_Renderer_CreateTextureBuffer2DArray(Cobalt_Renderer renderer, Cobalt_TextureBuffer2DArray* texture);
COBALT_FUNCTION_EXPORT void Cobalt_Renderer_CreateTextureBufferCubeArray(Cobalt_Renderer renderer, Cobalt_TextureBufferCubeArray* texture);

// Image sampler methods
COBALT_FUNCTION_EXPORT void Cobalt_Renderer_CreateTextureSampler1D(Cobalt_Renderer renderer, Cobalt_TextureSampler1D* sampler);
COBALT_FUNCTION_EXPORT void Cobalt_Renderer_CreateTextureSampler2D(Cobalt_Renderer renderer, Cobalt_TextureSampler2D* sampler);
COBALT_FUNCTION_EXPORT void Cobalt_Renderer_CreateTextureSampler3D(Cobalt_Renderer renderer, Cobalt_TextureSampler3D* sampler);
COBALT_FUNCTION_EXPORT void Cobalt_Renderer_CreateTextureSamplerCube(Cobalt_Renderer renderer, Cobalt_TextureSamplerCube* sampler);
COBALT_FUNCTION_EXPORT void Cobalt_Renderer_CreateTextureSampler1DArray(Cobalt_Renderer renderer, Cobalt_TextureSampler1DArray* sampler);
COBALT_FUNCTION_EXPORT void Cobalt_Renderer_CreateTextureSampler2DArray(Cobalt_Renderer renderer, Cobalt_TextureSampler2DArray* sampler);
COBALT_FUNCTION_EXPORT void Cobalt_Renderer_CreateTextureSamplerCubeArray(Cobalt_Renderer renderer, Cobalt_TextureSamplerCubeArray* sampler);

// Data array methods
COBALT_FUNCTION_EXPORT void Cobalt_Renderer_CreateDataArray(Cobalt_Renderer renderer, Cobalt_DataArray* array);
COBALT_FUNCTION_EXPORT void Cobalt_Renderer_CreateDataArrayOutput(Cobalt_Renderer renderer, Cobalt_DataArrayOutput* output);
COBALT_FUNCTION_EXPORT void Cobalt_Renderer_CreateTexelArray(Cobalt_Renderer renderer, Cobalt_TexelArray* array);
COBALT_FUNCTION_EXPORT void Cobalt_Renderer_CreateTexelArrayOutput(Cobalt_Renderer renderer, Cobalt_TexelArrayOutput* output);

// Batch methods
COBALT_FUNCTION_EXPORT void Cobalt_Renderer_CreateTransferBatch(Cobalt_Renderer renderer, Cobalt_TransferBatch* batch, Cobalt_StartTiming startTiming, Cobalt_EndTiming endTiming);

// Frame buffer methods
COBALT_FUNCTION_EXPORT void Cobalt_Renderer_CreateFrameBuffer(Cobalt_Renderer renderer, Cobalt_FrameBuffer* frameBuffer);
COBALT_FUNCTION_EXPORT void Cobalt_Renderer_CreateFrameBufferOutput(Cobalt_Renderer renderer, Cobalt_FrameBufferOutput* output);

// State buffer methods
COBALT_FUNCTION_EXPORT void Cobalt_Renderer_CreateStateBuffer(Cobalt_Renderer renderer, Cobalt_StateBuffer* buffer);
COBALT_FUNCTION_EXPORT void Cobalt_Renderer_CreateStateBufferLayout(Cobalt_Renderer renderer, Cobalt_StateBufferLayout* layout);

// Render tree node methods
COBALT_FUNCTION_EXPORT void Cobalt_Renderer_CreateRenderPassNode(Cobalt_Renderer renderer, Cobalt_RenderPassNode* renderPassNode);
COBALT_FUNCTION_EXPORT void Cobalt_Renderer_CreateProgramNode(Cobalt_Renderer renderer, Cobalt_ProgramNode* programNode);
COBALT_FUNCTION_EXPORT void Cobalt_Renderer_CreateStateGroupNode(Cobalt_Renderer renderer, Cobalt_StateGroupNode* stateGroupNode);
COBALT_FUNCTION_EXPORT void Cobalt_Renderer_CreateRenderableNode(Cobalt_Renderer renderer, Cobalt_RenderableNode* renderableNode);
COBALT_FUNCTION_EXPORT void Cobalt_Renderer_CreateDefaultState(Cobalt_Renderer renderer, Cobalt_DefaultState* defaultState);

// Program methods
COBALT_FUNCTION_EXPORT void Cobalt_Renderer_CreateShaderProgram(Cobalt_Renderer renderer, Cobalt_ShaderProgram* shaderProgram);

// Scene content methods
COBALT_FUNCTION_EXPORT void Cobalt_Renderer_SetRenderPasses(Cobalt_Renderer renderer, const Cobalt_RenderPassNode* childNodes, size_t childNodeCount, const int32_t* childNodeSortOrder);
COBALT_FUNCTION_EXPORT void Cobalt_Renderer_RemoveAllRenderPasses(Cobalt_Renderer renderer);

// Render methods
COBALT_FUNCTION_EXPORT void Cobalt_Renderer_StartNewFrame(Cobalt_Renderer renderer);
COBALT_FUNCTION_EXPORT void Cobalt_Renderer_WaitForDrawComplete(Cobalt_Renderer renderer);
COBALT_FUNCTION_EXPORT void Cobalt_Renderer_WaitForOutputCaptureComplete(Cobalt_Renderer renderer);
COBALT_FUNCTION_EXPORT void Cobalt_Renderer_WaitForDeferredDeletionComplete(Cobalt_Renderer renderer);

COBALT_FUNCTION_EXPORT void Cobalt_Renderer_Delete(Cobalt_Renderer renderer);

#ifdef __cplusplus
}
#endif
