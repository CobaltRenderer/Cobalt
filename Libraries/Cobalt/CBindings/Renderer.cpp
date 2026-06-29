// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "Renderer.h"
#include "PlatformBindings.pkg"
#include "WindowSystemInfoBase.h"
#include <Cobalt/RendererInterface/PlatformBindings.pkg>
#include <Cobalt/RendererInterface/RendererInterface.pkg>

using namespace cobalt::graphics;

//----------------------------------------------------------------------------------------
// Initialization methods
//----------------------------------------------------------------------------------------
Cobalt_Result Cobalt_Renderer_Initialize(Cobalt_Renderer renderer, const Cobalt_WindowSystemInfoBase* windowSystemInfo, Cobalt_RendererInitializationFlags flags)
{
	auto _this = reinterpret_cast<IRenderer*>(renderer);
	auto _initializationFlags = (IRenderer::InitializationFlags)flags;

	// Note that we can't just reinterpret cast the structure, as it's not guaranteed to be layout compatible due to
	// tail padding. While in reality it probably will be on every platform ever, we can't guarantee that from the
	// standard alone, even if the structure sizes match. We could however calculate the offset of the first member of
	// the derived class, and if those match, we could know the meaningful data is laid out the same way, possibly
	// apart from tail padding. That still won't help us though, as we have a structure size embedded that would need to
	// match, and even if that did match, we'd still be violating the strict aliasing rule, meaning we'd be in undefined
	// territory anyway. We need to unpack the C structure here and pack a new C++ one to be safe, which is fine,
	// because this operation is already heavyweight and infrequently called, and the tiny bit of work we do here
	// repacking things, while not optimal, isn't an issue in practice.
	bool result = false;
	switch (windowSystemInfo->type)
	{
	case Cobalt_WindowSystemType_Headless:
	{
		cobalt::graphics::WindowSystemInfoHeadless _windowSystemInfo;
		result = _this->Initialize(_windowSystemInfo, _initializationFlags);
		break;
	}
#ifdef COBALT_RENDERER_WIN32_SUPPORT
	case Cobalt_WindowSystemType_Win32:
	{
		cobalt::graphics::WindowSystemInfoWin32 _windowSystemInfo;
		result = _this->Initialize(_windowSystemInfo, _initializationFlags);
		break;
	}
#endif
#ifdef COBALT_RENDERER_XLIB_SUPPORT
	case Cobalt_WindowSystemType_Xlib:
	{
		auto windowSystemInfoResolved = reinterpret_cast<const Cobalt_WindowSystemInfoXlib*>(windowSystemInfo);
		cobalt::graphics::WindowSystemInfoXlib _windowSystemInfo(reinterpret_cast<Display*>(windowSystemInfoResolved->display));
		result = _this->Initialize(_windowSystemInfo, _initializationFlags);
		break;
	}
#endif
#ifdef COBALT_RENDERER_XCB_SUPPORT
	case Cobalt_WindowSystemType_XCB:
	{
		auto windowSystemInfoResolved = reinterpret_cast<const Cobalt_WindowSystemInfoXCB*>(windowSystemInfo);
		cobalt::graphics::WindowSystemInfoXCB _windowSystemInfo(reinterpret_cast<xcb_connection_t*>(windowSystemInfoResolved->connection));
		result = _this->Initialize(_windowSystemInfo, _initializationFlags);
		break;
	}
#endif
#ifdef COBALT_RENDERER_WAYLAND_SUPPORT
	case Cobalt_WindowSystemType_Wayland:
	{
		auto windowSystemInfoResolved = reinterpret_cast<const Cobalt_WindowSystemInfoWayland*>(windowSystemInfo);
		cobalt::graphics::WindowSystemInfoWayland _windowSystemInfo(reinterpret_cast<wl_display*>(windowSystemInfoResolved->display));
		result = _this->Initialize(_windowSystemInfo, _initializationFlags);
		break;
	}
#endif
#ifdef COBALT_RENDERER_APPKIT_SUPPORT
	case Cobalt_WindowSystemType_AppKit:
	{
		cobalt::graphics::WindowSystemInfoAppKit _windowSystemInfo;
		result = _this->Initialize(_windowSystemInfo, _initializationFlags);
		break;
	}
#endif
	}
	return result ? COBALT_SUCCESS : COBALT_FAILURE;
}

//----------------------------------------------------------------------------------------
// Geometry buffer methods
//----------------------------------------------------------------------------------------
void Cobalt_Renderer_CreateVertexBuffer(Cobalt_Renderer renderer, Cobalt_VertexBuffer* vertexBuffer)
{
	auto _this = reinterpret_cast<IRenderer*>(renderer);

	auto _vertexBuffer = _this->CreateVertexBuffer();
	*vertexBuffer = reinterpret_cast<Cobalt_VertexBuffer>(_vertexBuffer.release());
}

//----------------------------------------------------------------------------------------
void Cobalt_Renderer_CreateIndexBuffer(Cobalt_Renderer renderer, Cobalt_IndexBuffer* indexBuffer)
{
	auto _this = reinterpret_cast<IRenderer*>(renderer);

	auto _indexBuffer = _this->CreateIndexBuffer();
	*indexBuffer = reinterpret_cast<Cobalt_IndexBuffer>(_indexBuffer.release());
}

//----------------------------------------------------------------------------------------
void Cobalt_Renderer_CreateVertexAttribute(Cobalt_Renderer renderer, Cobalt_VertexAttribute* vertexAttribute, Cobalt_VertexAttributeType type, size_t elementCount, size_t vertexCount, Cobalt_VertexPerformanceHint performanceHintCpu, Cobalt_VertexPerformanceHint performanceHintGpu, Cobalt_VertexDataPersistenceFlags dataPersistenceFlags)
{
	auto _vertexAttribute = new RawVertexAttribute(
	  (IVertexAttribute::DataType)type,
	  elementCount,
	  vertexCount,
	  (IVertexAttribute::PerformanceHint)performanceHintCpu,
	  (IVertexAttribute::PerformanceHint)performanceHintGpu,
	  (IVertexAttribute::DataPersistenceFlags)dataPersistenceFlags);
	*vertexAttribute = reinterpret_cast<Cobalt_VertexAttribute>(_vertexAttribute);
}

//----------------------------------------------------------------------------------------
void Cobalt_Renderer_CreateIndexAttribute(Cobalt_Renderer renderer, Cobalt_IndexAttribute* indexAttribute, Cobalt_IndexAttributeType type, size_t indexCount, Cobalt_IndexPerformanceHint performanceHintCpu, Cobalt_IndexPerformanceHint performanceHintGpu, Cobalt_IndexDataPersistenceFlags dataPersistenceFlags)
{
	auto _indexAttribute = new RawIndexAttribute(
	  (IIndexAttribute::DataType)type,
	  indexCount,
	  (IIndexAttribute::PerformanceHint)performanceHintCpu,
	  (IIndexAttribute::PerformanceHint)performanceHintGpu,
	  (IIndexAttribute::DataPersistenceFlags)dataPersistenceFlags);
	*indexAttribute = reinterpret_cast<Cobalt_IndexAttribute>(_indexAttribute);
}

//----------------------------------------------------------------------------------------
// Image buffer methods
//----------------------------------------------------------------------------------------
void Cobalt_Renderer_CreateTextureBuffer1D(Cobalt_Renderer renderer, Cobalt_TextureBuffer1D* texture)
{
	auto _this = reinterpret_cast<IRenderer*>(renderer);

	auto _textureBuffer = _this->CreateTextureBuffer1D();
	*texture = reinterpret_cast<Cobalt_TextureBuffer1D>(_textureBuffer.release());
}

//----------------------------------------------------------------------------------------
void Cobalt_Renderer_CreateTextureBuffer2D(Cobalt_Renderer renderer, Cobalt_TextureBuffer2D* texture)
{
	auto _this = reinterpret_cast<IRenderer*>(renderer);

	auto _textureBuffer = _this->CreateTextureBuffer2D();
	*texture = reinterpret_cast<Cobalt_TextureBuffer2D>(_textureBuffer.release());
}

//----------------------------------------------------------------------------------------
void Cobalt_Renderer_CreateTextureBuffer3D(Cobalt_Renderer renderer, Cobalt_TextureBuffer3D* texture)
{
	auto _this = reinterpret_cast<IRenderer*>(renderer);

	auto _textureBuffer = _this->CreateTextureBuffer3D();
	*texture = reinterpret_cast<Cobalt_TextureBuffer3D>(_textureBuffer.release());
}

//----------------------------------------------------------------------------------------
void Cobalt_Renderer_CreateTextureBufferCube(Cobalt_Renderer renderer, Cobalt_TextureBufferCube* texture)
{
	auto _this = reinterpret_cast<IRenderer*>(renderer);

	auto _textureBuffer = _this->CreateTextureBufferCube();
	*texture = reinterpret_cast<Cobalt_TextureBufferCube>(_textureBuffer.release());
}

//----------------------------------------------------------------------------------------
void Cobalt_Renderer_CreateTextureBuffer1DArray(Cobalt_Renderer renderer, Cobalt_TextureBuffer1DArray* texture)
{
	auto _this = reinterpret_cast<IRenderer*>(renderer);

	auto _textureBuffer = _this->CreateTextureBuffer1DArray();
	*texture = reinterpret_cast<Cobalt_TextureBuffer1DArray>(_textureBuffer.release());
}

//----------------------------------------------------------------------------------------
void Cobalt_Renderer_CreateTextureBuffer2DArray(Cobalt_Renderer renderer, Cobalt_TextureBuffer2DArray* texture)
{
	auto _this = reinterpret_cast<IRenderer*>(renderer);

	auto _textureBuffer = _this->CreateTextureBuffer2DArray();
	*texture = reinterpret_cast<Cobalt_TextureBuffer2DArray>(_textureBuffer.release());
}

//----------------------------------------------------------------------------------------
void Cobalt_Renderer_CreateTextureBufferCubeArray(Cobalt_Renderer renderer, Cobalt_TextureBufferCubeArray* texture)
{
	auto _this = reinterpret_cast<IRenderer*>(renderer);

	auto _textureBuffer = _this->CreateTextureBufferCubeArray();
	*texture = reinterpret_cast<Cobalt_TextureBufferCubeArray>(_textureBuffer.release());
}

//----------------------------------------------------------------------------------------
// Image sampler methods
//----------------------------------------------------------------------------------------
void Cobalt_Renderer_CreateTextureSampler1D(Cobalt_Renderer renderer, Cobalt_TextureSampler1D* sampler)
{
	auto _this = reinterpret_cast<IRenderer*>(renderer);

	auto _textureSampler = _this->CreateTextureSampler1D();
	*sampler = reinterpret_cast<Cobalt_TextureSampler1D>(_textureSampler.release());
}

//----------------------------------------------------------------------------------------
void Cobalt_Renderer_CreateTextureSampler2D(Cobalt_Renderer renderer, Cobalt_TextureSampler2D* sampler)
{
	auto _this = reinterpret_cast<IRenderer*>(renderer);

	auto _textureSampler = _this->CreateTextureSampler2D();
	*sampler = reinterpret_cast<Cobalt_TextureSampler2D>(_textureSampler.release());
}

//----------------------------------------------------------------------------------------
void Cobalt_Renderer_CreateTextureSampler3D(Cobalt_Renderer renderer, Cobalt_TextureSampler3D* sampler)
{
	auto _this = reinterpret_cast<IRenderer*>(renderer);

	auto _textureSampler = _this->CreateTextureSampler3D();
	*sampler = reinterpret_cast<Cobalt_TextureSampler3D>(_textureSampler.release());
}

//----------------------------------------------------------------------------------------
void Cobalt_Renderer_CreateTextureSamplerCube(Cobalt_Renderer renderer, Cobalt_TextureSamplerCube* sampler)
{
	auto _this = reinterpret_cast<IRenderer*>(renderer);

	auto _textureSampler = _this->CreateTextureSamplerCube();
	*sampler = reinterpret_cast<Cobalt_TextureSamplerCube>(_textureSampler.release());
}

//----------------------------------------------------------------------------------------
void Cobalt_Renderer_CreateTextureSampler1DArray(Cobalt_Renderer renderer, Cobalt_TextureSampler1DArray* sampler)
{
	auto _this = reinterpret_cast<IRenderer*>(renderer);

	auto _textureSampler = _this->CreateTextureSampler1DArray();
	*sampler = reinterpret_cast<Cobalt_TextureSampler1DArray>(_textureSampler.release());
}

//----------------------------------------------------------------------------------------
void Cobalt_Renderer_CreateTextureSampler2DArray(Cobalt_Renderer renderer, Cobalt_TextureSampler2DArray* sampler)
{
	auto _this = reinterpret_cast<IRenderer*>(renderer);

	auto _textureSampler = _this->CreateTextureSampler2DArray();
	*sampler = reinterpret_cast<Cobalt_TextureSampler2DArray>(_textureSampler.release());
}

//----------------------------------------------------------------------------------------
void Cobalt_Renderer_CreateTextureSamplerCubeArray(Cobalt_Renderer renderer, Cobalt_TextureSamplerCubeArray* sampler)
{
	auto _this = reinterpret_cast<IRenderer*>(renderer);

	auto _textureSampler = _this->CreateTextureSamplerCubeArray();
	*sampler = reinterpret_cast<Cobalt_TextureSamplerCubeArray>(_textureSampler.release());
}

//----------------------------------------------------------------------------------------
// Data array methods
//----------------------------------------------------------------------------------------
void Cobalt_Renderer_CreateDataArray(Cobalt_Renderer renderer, Cobalt_DataArray* array)
{
	auto _this = reinterpret_cast<IRenderer*>(renderer);

	auto _array = _this->CreateDataArray();
	*array = reinterpret_cast<Cobalt_DataArray>(_array.release());
}

//----------------------------------------------------------------------------------------
void Cobalt_Renderer_CreateDataArrayOutput(Cobalt_Renderer renderer, Cobalt_DataArrayOutput* output)
{
	auto _this = reinterpret_cast<IRenderer*>(renderer);

	auto _output = _this->CreateDataArrayOutput();
	*output = reinterpret_cast<Cobalt_DataArrayOutput>(_output.release());
}

//----------------------------------------------------------------------------------------
void Cobalt_Renderer_CreateTexelArray(Cobalt_Renderer renderer, Cobalt_TexelArray* array)
{
	auto _this = reinterpret_cast<IRenderer*>(renderer);

	auto _array = _this->CreateTexelArray();
	*array = reinterpret_cast<Cobalt_TexelArray>(_array.release());
}

//----------------------------------------------------------------------------------------
void Cobalt_Renderer_CreateTexelArrayOutput(Cobalt_Renderer renderer, Cobalt_TexelArrayOutput* output)
{
	auto _this = reinterpret_cast<IRenderer*>(renderer);

	auto _output = _this->CreateTexelArrayOutput();
	*output = reinterpret_cast<Cobalt_TexelArrayOutput>(_output.release());
}

//----------------------------------------------------------------------------------------
// Batch method
//----------------------------------------------------------------------------------------
void Cobalt_Renderer_CreateTransferBatch(Cobalt_Renderer renderer, Cobalt_TransferBatch* batch, Cobalt_StartTiming startTiming, Cobalt_EndTiming endTiming)
{
	auto _this = reinterpret_cast<IRenderer*>(renderer);

	auto _batch = _this->CreateTransferBatch((ITransferBatch::StartTiming)startTiming, (ITransferBatch::EndTiming)endTiming);
	*batch = reinterpret_cast<Cobalt_TransferBatch>(_batch.release());
}

//----------------------------------------------------------------------------------------
// Frame buffer methods
//----------------------------------------------------------------------------------------
void Cobalt_Renderer_CreateFrameBuffer(Cobalt_Renderer renderer, Cobalt_FrameBuffer* frameBuffer)
{
	auto _this = reinterpret_cast<IRenderer*>(renderer);

	auto _frameBuffer = _this->CreateFrameBuffer();
	*frameBuffer = reinterpret_cast<Cobalt_FrameBuffer>(_frameBuffer.release());
}

//----------------------------------------------------------------------------------------
void Cobalt_Renderer_CreateFrameBufferOutput(Cobalt_Renderer renderer, Cobalt_FrameBufferOutput* output)
{
	auto _this = reinterpret_cast<IRenderer*>(renderer);

	auto _output = _this->CreateFrameBufferOutput();
	*output = reinterpret_cast<Cobalt_FrameBufferOutput>(_output.release());
}

//----------------------------------------------------------------------------------------
// State buffer methods
//----------------------------------------------------------------------------------------
void Cobalt_Renderer_CreateStateBuffer(Cobalt_Renderer renderer, Cobalt_StateBuffer* buffer)
{
	auto _this = reinterpret_cast<IRenderer*>(renderer);

	auto _buffer = _this->CreateStateBuffer();
	*buffer = reinterpret_cast<Cobalt_StateBuffer>(_buffer.release());
}

//----------------------------------------------------------------------------------------
void Cobalt_Renderer_CreateStateBufferLayout(Cobalt_Renderer renderer, Cobalt_StateBufferLayout* layout)
{
	auto _this = reinterpret_cast<IRenderer*>(renderer);

	auto _layout = _this->CreateStateBufferLayout();
	*layout = reinterpret_cast<Cobalt_StateBufferLayout>(_layout.release());
}

//----------------------------------------------------------------------------------------
// Render tree node methods
//----------------------------------------------------------------------------------------
void Cobalt_Renderer_CreateRenderPassNode(Cobalt_Renderer renderer, Cobalt_RenderPassNode* renderPassNode)
{
	auto _this = reinterpret_cast<IRenderer*>(renderer);

	auto _renderPass = _this->CreateRenderPassNode();
	*renderPassNode = reinterpret_cast<Cobalt_RenderPassNode>(_renderPass.release());
}

//----------------------------------------------------------------------------------------
void Cobalt_Renderer_CreateProgramNode(Cobalt_Renderer renderer, Cobalt_ProgramNode* programNode)
{
	auto _this = reinterpret_cast<IRenderer*>(renderer);

	auto _programNode = _this->CreateProgramNode();
	*programNode = reinterpret_cast<Cobalt_ProgramNode>(_programNode.release());
}

//----------------------------------------------------------------------------------------
void Cobalt_Renderer_CreateStateGroupNode(Cobalt_Renderer renderer, Cobalt_StateGroupNode* stateGroupNode)
{
	auto _this = reinterpret_cast<IRenderer*>(renderer);

	auto _stateGroupNode = _this->CreateStateGroupNode();
	*stateGroupNode = reinterpret_cast<Cobalt_StateGroupNode>(_stateGroupNode.release());
}

//----------------------------------------------------------------------------------------
void Cobalt_Renderer_CreateRenderableNode(Cobalt_Renderer renderer, Cobalt_RenderableNode* renderableNode)
{
	auto _this = reinterpret_cast<IRenderer*>(renderer);

	auto _renderableNode = _this->CreateRenderableNode();
	*renderableNode = reinterpret_cast<Cobalt_RenderableNode>(_renderableNode.release());
}

//----------------------------------------------------------------------------------------
void Cobalt_Renderer_CreateDefaultState(Cobalt_Renderer renderer, Cobalt_DefaultState* defaultState)
{
	auto _this = reinterpret_cast<IRenderer*>(renderer);

	auto _defaultState = _this->CreateDefaultState();
	*defaultState = reinterpret_cast<Cobalt_DefaultState>(_defaultState.release());
}

//----------------------------------------------------------------------------------------
// Program methods
//----------------------------------------------------------------------------------------
void Cobalt_Renderer_CreateShaderProgram(Cobalt_Renderer renderer, Cobalt_ShaderProgram* shaderProgram)
{
	auto _this = reinterpret_cast<IRenderer*>(renderer);

	auto _shaderProgram = _this->CreateShaderProgram();
	*shaderProgram = reinterpret_cast<Cobalt_ShaderProgram>(_shaderProgram.release());
}

//----------------------------------------------------------------------------------------
// Scene content methods
//----------------------------------------------------------------------------------------
void Cobalt_Renderer_SetRenderPasses(Cobalt_Renderer renderer, const Cobalt_RenderPassNode* childNodes, size_t childNodeCount, const int32_t* childNodeSortOrder)
{
	auto _this = reinterpret_cast<IRenderer*>(renderer);
	auto _childNodes = reinterpret_cast<IRenderPassNode* const*>(childNodes);

	_this->SetRenderPasses(_childNodes, childNodeCount, childNodeSortOrder);
}

//----------------------------------------------------------------------------------------
void Cobalt_Renderer_RemoveAllRenderPasses(Cobalt_Renderer renderer)
{
	auto _this = reinterpret_cast<IRenderer*>(renderer);

	_this->RemoveAllRenderPasses();
}

//----------------------------------------------------------------------------------------
// Render methods
//----------------------------------------------------------------------------------------
void Cobalt_Renderer_StartNewFrame(Cobalt_Renderer renderer)
{
	auto _this = reinterpret_cast<IRenderer*>(renderer);

	_this->StartNewFrame();
}

//----------------------------------------------------------------------------------------
void Cobalt_Renderer_WaitForDrawComplete(Cobalt_Renderer renderer)
{
	auto _this = reinterpret_cast<IRenderer*>(renderer);

	_this->WaitForDrawComplete();
}

//----------------------------------------------------------------------------------------
void Cobalt_Renderer_WaitForOutputCaptureComplete(Cobalt_Renderer renderer)
{
	auto _this = reinterpret_cast<IRenderer*>(renderer);

	_this->WaitForOutputCaptureComplete();
}

//----------------------------------------------------------------------------------------
void Cobalt_Renderer_WaitForDeferredDeletionComplete(Cobalt_Renderer renderer)
{
	auto _this = reinterpret_cast<IRenderer*>(renderer);

	_this->WaitForDeferredDeletionComplete();
}

//----------------------------------------------------------------------------------------
void Cobalt_Renderer_Delete(Cobalt_Renderer renderer)
{
	auto _this = reinterpret_cast<IRenderer*>(renderer);

	_this->Delete();
}
