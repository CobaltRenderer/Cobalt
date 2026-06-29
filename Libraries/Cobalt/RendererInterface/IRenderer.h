// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include "Deleter.h"
#include "IDataArray.h"
#include "IDataArrayOutput.h"
#include "IFrameBuffer.h"
#include "IFrameBufferOutput.h"
#include "IIndexBuffer.h"
#include "IProgramNode.h"
#include "IRenderPassNode.h"
#include "IRenderableNode.h"
#include "IShaderProgram.h"
#include "IStateBuffer.h"
#include "IStateBufferLayout.h"
#include "IStateGroupNode.h"
#include "ITexelArray.h"
#include "ITexelArrayOutput.h"
#include "ITextureBuffer1D.h"
#include "ITextureBuffer1DArray.h"
#include "ITextureBuffer2D.h"
#include "ITextureBuffer2DArray.h"
#include "ITextureBuffer3D.h"
#include "ITextureBufferCube.h"
#include "ITextureBufferCubeArray.h"
#include "ITextureSampler1D.h"
#include "ITextureSampler1DArray.h"
#include "ITextureSampler2D.h"
#include "ITextureSampler2DArray.h"
#include "ITextureSampler3D.h"
#include "ITextureSamplerCube.h"
#include "ITextureSamplerCubeArray.h"
#include "ITransferBatch.h"
#include "IVertexBuffer.h"
#include <memory>
namespace cobalt { namespace graphics {

class IRenderer
{
public:
	// Enumerations
	enum class Options
	{
		EnableDebugLogging,
		EnableRenderMarkers,
	};
	enum class InitializationFlags : uint64_t
	{
		None = 0,
	};

	// Structures
	struct WindowSystemInfoBase
	{
		enum class WindowSystemType
		{
			// Headless
			Headless = 0x10001,
			// Windows
			Win32 = 0x20001,
			// Linux
			Xlib = 0x30001,
			XCB = 0x30002,
			Wayland = 0x30003,
			// MacOS
			AppKit = 0x40001,
		};

		size_t structureSizeInBytes = 0;
		WindowSystemType windowSystemType = {};

	protected:
		WindowSystemInfoBase() = default;
	};

	// Typedefs
	typedef std::unique_ptr<IRenderer, Deleter<IRenderer>> unique_ptr;

public:
	// Initialization methods
	virtual SuccessToken Initialize(const WindowSystemInfoBase& windowSystemInfo, InitializationFlags flags = InitializationFlags::None) = 0;
	virtual void Delete() = 0;

	// Geometry buffer methods
	inline IVertexBuffer::unique_ptr CreateVertexBuffer();
	inline IIndexBuffer::unique_ptr CreateIndexBuffer();

	// Image buffer methods
	inline ITextureBuffer1D::unique_ptr CreateTextureBuffer1D();
	inline ITextureBuffer2D::unique_ptr CreateTextureBuffer2D();
	inline ITextureBuffer3D::unique_ptr CreateTextureBuffer3D();
	inline ITextureBufferCube::unique_ptr CreateTextureBufferCube();
	inline ITextureBuffer1DArray::unique_ptr CreateTextureBuffer1DArray();
	inline ITextureBuffer2DArray::unique_ptr CreateTextureBuffer2DArray();
	inline ITextureBufferCubeArray::unique_ptr CreateTextureBufferCubeArray();

	// Image sampler methods
	inline ITextureSampler1D::unique_ptr CreateTextureSampler1D();
	inline ITextureSampler2D::unique_ptr CreateTextureSampler2D();
	inline ITextureSampler3D::unique_ptr CreateTextureSampler3D();
	inline ITextureSamplerCube::unique_ptr CreateTextureSamplerCube();
	inline ITextureSampler1DArray::unique_ptr CreateTextureSampler1DArray();
	inline ITextureSampler2DArray::unique_ptr CreateTextureSampler2DArray();
	inline ITextureSamplerCubeArray::unique_ptr CreateTextureSamplerCubeArray();

	// Data array methods
	inline IDataArray::unique_ptr CreateDataArray();
	inline IDataArrayOutput::unique_ptr CreateDataArrayOutput();
	inline ITexelArray::unique_ptr CreateTexelArray();
	inline ITexelArrayOutput::unique_ptr CreateTexelArrayOutput();

	// Batch methods
	inline ITransferBatch::unique_ptr CreateTransferBatch(ITransferBatch::StartTiming startTiming, ITransferBatch::EndTiming endTiming);

	// Frame buffer methods
	inline IFrameBuffer::unique_ptr CreateFrameBuffer();
	inline IFrameBufferOutput::unique_ptr CreateFrameBufferOutput();

	// State buffer methods
	inline IStateBuffer::unique_ptr CreateStateBuffer();
	inline IStateBufferLayout::unique_ptr CreateStateBufferLayout();

	// Render tree node methods
	inline IRenderPassNode::unique_ptr CreateRenderPassNode();
	inline IProgramNode::unique_ptr CreateProgramNode();
	inline IStateGroupNode::unique_ptr CreateStateGroupNode();
	inline IRenderableNode::unique_ptr CreateRenderableNode();
	inline IDefaultState::unique_ptr CreateDefaultState();

	// Program methods
	inline IShaderProgram::unique_ptr CreateShaderProgram();

	// Scene content methods
	virtual void SetRenderPasses(IRenderPassNode* const* childNodes, size_t childNodeCount, const int32_t* childNodeSortOrder = nullptr) = 0;
	virtual void SetRenderPasses(IRenderPassNode::unique_ptr const* childNodes, size_t childNodeCount, const int32_t* childNodeSortOrder = nullptr) = 0;
	virtual void RemoveAllRenderPasses() = 0;

	// Render methods
	virtual void StartNewFrame() = 0;
	virtual void WaitForDrawComplete() const = 0;
	virtual void WaitForOutputCaptureComplete() const = 0;
	virtual void WaitForDeferredDeletionComplete() const = 0;

protected:
	// Constructors
	~IRenderer() = default;

	// Geometry buffer methods
	virtual IVertexBuffer* CreateVertexBufferInternal() = 0;
	virtual IIndexBuffer* CreateIndexBufferInternal() = 0;

	// Image buffer methods
	virtual ITextureBuffer1D* CreateTextureBuffer1DInternal() = 0;
	virtual ITextureBuffer2D* CreateTextureBuffer2DInternal() = 0;
	virtual ITextureBuffer3D* CreateTextureBuffer3DInternal() = 0;
	virtual ITextureBufferCube* CreateTextureBufferCubeInternal() = 0;
	virtual ITextureBuffer1DArray* CreateTextureBuffer1DArrayInternal() = 0;
	virtual ITextureBuffer2DArray* CreateTextureBuffer2DArrayInternal() = 0;
	virtual ITextureBufferCubeArray* CreateTextureBufferCubeArrayInternal() = 0;

	// Image sampler methods
	virtual ITextureSampler1D* CreateTextureSampler1DInternal() = 0;
	virtual ITextureSampler2D* CreateTextureSampler2DInternal() = 0;
	virtual ITextureSampler3D* CreateTextureSampler3DInternal() = 0;
	virtual ITextureSamplerCube* CreateTextureSamplerCubeInternal() = 0;
	virtual ITextureSampler1DArray* CreateTextureSampler1DArrayInternal() = 0;
	virtual ITextureSampler2DArray* CreateTextureSampler2DArrayInternal() = 0;
	virtual ITextureSamplerCubeArray* CreateTextureSamplerCubeArrayInternal() = 0;

	// Data array methods
	virtual IDataArray* CreateDataArrayInternal() = 0;
	virtual IDataArrayOutput* CreateDataArrayOutputInternal() = 0;
	virtual ITexelArray* CreateTexelArrayInternal() = 0;
	virtual ITexelArrayOutput* CreateTexelArrayOutputInternal() = 0;

	// Batch methods
	virtual ITransferBatch* CreateTransferBatchInternal(ITransferBatch::StartTiming startTiming, ITransferBatch::EndTiming endTiming) = 0;

	// Frame buffer methods
	virtual IFrameBuffer* CreateFrameBufferInternal() = 0;
	virtual IFrameBufferOutput* CreateFrameBufferOutputInternal() = 0;

	// State buffer methods
	virtual IStateBuffer* CreateStateBufferInternal() = 0;
	virtual IStateBufferLayout* CreateStateBufferLayoutInternal() = 0;

	// Render tree node methods
	virtual IRenderPassNode* CreateRenderPassNodeInternal() = 0;
	virtual IProgramNode* CreateProgramNodeInternal() = 0;
	virtual IStateGroupNode* CreateStateGroupNodeInternal() = 0;
	virtual IRenderableNode* CreateRenderableNodeInternal() = 0;
	virtual IDefaultState* CreateDefaultStateInternal() = 0;

	// Program methods
	virtual IShaderProgram* CreateShaderProgramInternal() = 0;
};

}} // namespace cobalt::graphics
#include "IRenderer.inl"
