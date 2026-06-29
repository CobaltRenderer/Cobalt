// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include "DescriptorHandle.h"
#include "Direct3DHeaders.h"
#include "Direct3DTextureBuffer2D.h"
#include <Cobalt/LoggingInterface/LoggingInterface.pkg>
#include <Cobalt/RendererInterface/PlatformBindings.pkg>
#include <Cobalt/RendererInterface/RendererInterface.pkg>
#include <atomic>
#include <dxgi1_5.h>
#include <mutex>
#include <variant>
#include <vector>
namespace cobalt::graphics {
class Direct3DRenderer;
class Direct3DTextureBuffer2D;
class Direct3DFrameBufferOutput;

class Direct3DFrameBuffer : public IFrameBuffer
{
public:
	// Constructors
	Direct3DFrameBuffer(cobalt::logging::ILogger* log, Direct3DRenderer* renderer);
	~Direct3DFrameBuffer();

	// Initialization methods
	void Delete() override;

	// Binding methods
	SuccessToken BindTexture(ITextureBuffer2D* texture, AttachmentType type, size_t index) override;
	void UnbindTexture(AttachmentType type, size_t index) override;
	SuccessToken BindMultiSamplingResolveTexture(ITextureBuffer2D* texture, AttachmentType type, size_t index) override;
	void UnbindMultiSamplingResolveTexture(AttachmentType type, size_t index) override;
	SuccessToken BindWindow(const WindowInfoBase& windowInfo, WindowDepthStencilMode depthStencilMode, WindowColorSpaceMode colorSpaceMode, WindowBindingFlags bindingFlags) override;
	SuccessToken NotifyWindowResized(const V2UInt32& windowSizeInPixels) override;
	void BindFrameBuffer(ID3D12GraphicsCommandList* commandList, ID3D12CommandQueue* commandQueue, IDXGIFactory4* dxgiFactory, bool performResourceStateTransition = true);
	void UnbindFrameBuffer(ID3D12GraphicsCommandList* commandList);
	void GetDataFormats(DXGI_FORMAT (&renderTargetFormats)[8], UINT& populatedRenderTargetFormatEntries, DXGI_FORMAT& depthStencilFormat) const;
	bool IsBoundToWindow() const;
	int GetFrameBufferObjectLastUpdateToken() const;
	ITextureBuffer::SampleCount GetSampleCount(UINT& nativeSampleCount, UINT& sampleQuality) const;

	// Viewport methods
	void DefineViewportRegion(const V2UInt32& startPos, const V2UInt32& size) override;
	void DefineScissorRegion(const V2UInt32& startPos, const V2UInt32& size) override;
	void RemoveScissorRegion() override;

	// Output capture methods
	bool HasCaptureTargets() const;
	void AddOutputCaptureTarget(IFrameBufferOutput* captureTarget, AttachmentType type, size_t index) override;
	void RemoveOutputCaptureTarget(IFrameBufferOutput* captureTarget) override;
	void CaptureFrameBufferOutput(ID3D12GraphicsCommandList* commandList);
	void CompleteCaptureFrameBufferOutput();

	// Framebuffer update methods
	bool PresentToWindow();
	void ClearRenderView(ID3D12GraphicsCommandList* commandList, size_t index, const V4Float32& val);
	void ClearDepthStencilView(ID3D12GraphicsCommandList* commandList, float depthVal, uint32_t stencilVal);
	void ResolveMultiSamplingAttachmentToTexture(ID3D12GraphicsCommandList* commandList, AttachmentType type, size_t index, size_t resolveIndex);

	// Build state methods
	void MigrateBuildStateToDrawState();

private:
	// Constants
	static const uint32_t FrameCount = 2;

	// Structures
	struct BoundTextureInfo
	{
		AttachmentType type;
		size_t index;
		Direct3DTextureBuffer2D* texture;
	};
	struct CaptureTargetInfo
	{
		AttachmentType type;
		size_t index;
		Direct3DFrameBufferOutput* captureTarget;
		bool completionPending = false;
		ID3D12Resource* stagingTextureBuffer = nullptr;
		ITextureBuffer::ImageFormat imageFormat;
		ITextureBuffer::DataFormat dataFormat;
		size_t elementCount;
		size_t elementSizeInBytes;
		size_t pixelOffsetInBytes;
		size_t pixelStrideInBytes;
		size_t rowStrideInBytes;
		size_t bufferSizeInBytes;
		V2UInt32 croppedImageSize;
	};
	struct CaptureStagingTextureInfo
	{
		Microsoft::WRL::ComPtr<ID3D12Resource> buffer;
		D3D12MA::Allocation* allocation = nullptr;
		size_t bufferSizeInBytes = 0;
	};
	struct MutableState
	{
		bool framebufferInvalid = false;
		bool viewportInvalid = false;
		bool boundToWindow = false;
		bool headlessWindow = false;
		bool scissorRegionDefined = false;
		std::variant<std::monostate, WindowInfoHeadless, WindowInfoWin32> windowInfo;
		WindowDepthStencilMode windowDepthStencilMode = {};
		WindowColorSpaceMode windowColorSpaceMode = {};
		WindowBindingFlags windowBindingFlags = WindowBindingFlags::None;
		V2UInt32 viewportRegionStartPos = {};
		V2UInt32 viewportRegionSize = {};
		V2UInt32 scissorRegionStartPos = {};
		V2UInt32 scissorRegionSize = {};
		std::vector<BoundTextureInfo> boundTextures;
		std::vector<BoundTextureInfo> resolveTextures;
		std::vector<CaptureTargetInfo> captureTargets;
	};

private:
	// Output capture methods
	bool CaptureFrameBufferOutput(ID3D12GraphicsCommandList* commandList, CaptureTargetInfo& captureTargetInfo, ID3D12Resource* sourceTextureBuffer, ID3D12Resource* stagingTextureBuffer, D3D12_RESOURCE_STATES lastResourceState, UINT sourceSubresource, const D3D12_PLACED_SUBRESOURCE_FOOTPRINT& bufferFootprint, const V2UInt32& croppedImageSize, ITextureBuffer::ImageFormat imageFormat, ITextureBuffer::DataFormat dataFormat, size_t elementCount, size_t elementSizeInBytes, size_t pixelOffsetInBytes, size_t pixelStrideInBytes);
	bool CompleteCaptureFrameBufferOutput(const CaptureTargetInfo& captureTargetInfo);
	void DeleteCaptureStagingTexture(CaptureStagingTextureInfo& entry);
	void DeleteCaptureStagingTextures();
	static bool GetFormatFromNativeFormat(DXGI_FORMAT nativeFormat, ITextureBuffer::ImageFormat& imageFormat, ITextureBuffer::DataFormat& dataFormat, size_t& elementCount, size_t& elementSizeInBytes, size_t& pixelOffsetInBytes, size_t& pixelStrideInBytes, bool stencilComponent);

	// Framebuffer update methods
	void UpdateViewport();
	bool CreateNativeObjectsForWindowTarget(ID3D12Device* device, ID3D12CommandQueue* commandQueue, IDXGIFactory4* dxgiFactory, bool resizeExistingSwapChain);
	bool CreateNativeObjectsForTextureTarget();
	bool WaitForWindowTargetRelease();
	void DeleteNativeObjects();
	static void GetWindowColorFormat(WindowColorSpaceMode mode, DXGI_FORMAT& textureFormat);
	static void GetWindowDepthStencilFormat(WindowDepthStencilMode mode, DXGI_FORMAT& textureFormat, bool& hasScencilComponent);

	// Build state methods
	void FlagBuildStateModified();

private:
	cobalt::logging::ILogger* _log;
	Direct3DRenderer* _renderer;
	mutable std::mutex _accessMutex;
	Microsoft::WRL::ComPtr<IDXGISwapChain3> _swapChain;
	Microsoft::WRL::ComPtr<ID3D12Resource> _depthStencilTexture;
	std::vector<std::unique_ptr<DescriptorHandle>> _renderTargetViews;
	std::vector<CD3DX12_CPU_DESCRIPTOR_HANDLE> _renderTargetViewsFlatArray;
	std::unique_ptr<DescriptorHandle> _depthStencilView;
	std::vector<CaptureStagingTextureInfo> _stagingTexturesForCapture;
	std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> _swapChainBuffers;
	std::vector<DXGI_FORMAT> _renderTargetFormats;
	UINT _currentWindowBackBufferIndex = {};
	DXGI_FORMAT _colorBufferFormat = {};
	DXGI_FORMAT _depthStencilFormat = {};
	D3D12_CLEAR_FLAGS _depthStencilClearFlags = {};
	bool _framebufferCreated = false;
	bool _checkedTearingFeaturePresent = false;
	bool _tearingFeaturePresent = false;
	mutable bool _warnedAboutRenderTargetLimitExceeded = false;
	std::atomic_flag _stateModified = ATOMIC_FLAG_INIT;
	MutableState _drawState;
	MutableState _buildState;
	D3D12_VIEWPORT _viewport = {};
	D3D12_RECT _scissorRect = {};
	V2UInt32 _framebufferSize = {};
	ITextureBuffer::SampleCount _sampleCount = {};
	UINT _nativeSampleCount = {};
	UINT _sampleQuality = {};
	int _frameBufferObjectLastUpdateToken = 1;
};

} // namespace cobalt::graphics
