// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include "Direct3DHeaders.h"
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
	void BindFrameBuffer(ID3D11Device1* device, ID3D11DeviceContext1* context, IDXGIFactory2* dxgiFactory);
	bool IsBoundToWindow() const;
	ITextureBuffer::SampleCount GetSampleCount() const;
	ID3D11DepthStencilView* GetDepthStencilView() const;
	const std::vector<ID3D11RenderTargetView*>& GetRenderTargetViews() const;

	// Viewport methods
	void DefineViewportRegion(const V2UInt32& startPos, const V2UInt32& size) override;
	void DefineScissorRegion(const V2UInt32& startPos, const V2UInt32& size) override;
	void RemoveScissorRegion() override;

	// Output capture methods
	void AddOutputCaptureTarget(IFrameBufferOutput* captureTarget, AttachmentType type, size_t index) override;
	void RemoveOutputCaptureTarget(IFrameBufferOutput* captureTarget) override;
	void CaptureFrameBufferOutput(ID3D11Device1* device, ID3D11DeviceContext1* context);

	// Framebuffer update methods
	bool PresentToWindow();
	void ClearRenderView(ID3D11DeviceContext1* context, size_t index, const V4Float32& val);
	void ClearDepthStencilView(ID3D11DeviceContext1* context, float depthVal, uint32_t stencilVal);
	void ResolveMultiSamplingAttachmentToTexture(ID3D11DeviceContext1* context, AttachmentType type, size_t index, size_t resolveIndex);

	// Build state methods
	void MigrateBuildStateToDrawState();

private:
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
	bool CaptureFrameBufferOutput(ID3D11DeviceContext1* context, const CaptureTargetInfo& captureTargetInfo, ID3D11Texture2D* sourceTextureBuffer, ID3D11Texture2D* stagingTextureBuffer, ITextureBuffer::ImageFormat imageFormat, ITextureBuffer::DataFormat dataFormat, size_t elementCount, size_t elementSizeInBytes, size_t pixelOffsetInBytes, size_t pixelStrideInBytes);
	static bool GetFormatFromNativeFormat(DXGI_FORMAT nativeFormat, ITextureBuffer::ImageFormat& imageFormat, ITextureBuffer::DataFormat& dataFormat, size_t& elementCount, size_t& elementSizeInBytes, size_t& pixelOffsetInBytes, size_t& pixelStrideInBytes, bool stencilComponent);

	// Framebuffer update methods
	void UpdateViewport();
	bool CreateNativeObjectsForWindowTarget(ID3D11Device1* device, IDXGIFactory2* dxgiFactory, bool resizeExistingSwapChain);
	bool CreateNativeObjectsForTextureTarget(ID3D11Device1* device);
	void DeleteNativeObjects();
	static void GetWindowColorFormat(WindowColorSpaceMode mode, DXGI_FORMAT& textureFormat);
	static void GetWindowDepthStencilFormat(WindowDepthStencilMode mode, DXGI_FORMAT& textureFormat, bool& hasScencilComponent);

	// Build state methods
	void FlagBuildStateModified();

private:
	cobalt::logging::ILogger* _log;
	Direct3DRenderer* _renderer;
	mutable std::mutex _accessMutex;
	Microsoft::WRL::ComPtr<IDXGISwapChain1> _swapChain;
	Microsoft::WRL::ComPtr<ID3D11Texture2D> _depthStencilTexture;
	Microsoft::WRL::ComPtr<ID3D11Texture2D> _backBuffer;
	std::vector<Microsoft::WRL::ComPtr<ID3D11RenderTargetView>> _renderTargetViews;
	std::vector<ID3D11RenderTargetView*> _renderTargetViewsFlatArray;
	Microsoft::WRL::ComPtr<ID3D11DepthStencilView> _depthStencilView;
	std::vector<Microsoft::WRL::ComPtr<ID3D11Texture2D>> _stagingTexturesForCapture;
	bool _framebufferCreated = false;
	bool _checkedFeatureAvailability = false;
	bool _flipDiscardModePresent = false;
	bool _tearingFeaturePresent = false;
	bool _warnedAboutRenderTargetLimitExceeded = false;
	std::atomic_flag _stateModified = ATOMIC_FLAG_INIT;
	D3D11_CLEAR_FLAG _depthStencilClearFlags = {};
	MutableState _drawState;
	MutableState _buildState;
	D3D11_VIEWPORT _viewport = {};
	D3D11_RECT _scissorRect = {};
	ITextureBuffer::SampleCount _sampleCount = {};
};

} // namespace cobalt::graphics
