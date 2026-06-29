// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include "VulkanFrameBufferOutput.h"
#include "VulkanHeaders.h"
#include <Cobalt/Debug/Debug.pkg>
#include <Cobalt/LoggingInterface/LoggingInterface.pkg>
#include <Cobalt/RendererInterface/PlatformBindings.pkg>
#include <Cobalt/RendererInterface/RendererInterface.pkg>
#include <atomic>
#include <mutex>
#include <variant>
#include <vector>
namespace cobalt::graphics {
class VulkanRenderer;
class VulkanTextureBuffer2D;

class VulkanFrameBuffer : public IFrameBuffer
{
public:
	// Enumerations
	enum class AttachmentFormat
	{
		Int,
		UInt,
		Float,
	};

public:
	// Constructors
	VulkanFrameBuffer(cobalt::logging::ILogger* log, VulkanRenderer* renderer);
	~VulkanFrameBuffer();

	// Initialization methods
	void Delete() override;

	// Binding methods
	SuccessToken BindTexture(ITextureBuffer2D* texture, AttachmentType type, size_t index) override;
	void UnbindTexture(AttachmentType type, size_t index) override;
	SuccessToken BindMultiSamplingResolveTexture(ITextureBuffer2D* texture, AttachmentType type, size_t index) override;
	void UnbindMultiSamplingResolveTexture(AttachmentType type, size_t index) override;
	SuccessToken BindWindow(const WindowInfoBase& windowInfo, WindowDepthStencilMode depthStencilMode, WindowColorSpaceMode colorSpaceMode, WindowBindingFlags bindingFlags) override;
	SuccessToken NotifyWindowResized(const V2UInt32& windowSizeInPixels) override;
	void PrepareFrameBuffer(VkCommandBuffer commandBuffer);
	VkFramebuffer GetFramebuffer() const;
	VkRenderPass GetTemplateRenderPass() const;
	bool IsBoundToWindow() const;
	AttachmentFormat GetColorAttachmentFormat(size_t index) const;
	int GetRenderPassAttachmentIndex(AttachmentType type, size_t index) const;
	int GetRenderPassResolveAttachmentIndex(AttachmentType type, size_t index) const;
	int GetRenderPassColorAttachmentIndex(int attachmentIndex) const;
	size_t GetRenderPassAttachmentCount() const;
	size_t GetRenderPassColorAttachmentReferenceCount() const;
	int GetFrameBufferObjectLastUpdateToken() const;
	const VkAttachmentDescription* GetRenderPassAttachments() const;
	const VkAttachmentReference* GetRenderPassColorAttachmentReferences() const;
	const VkAttachmentReference* GetRenderPassDepthStencilAttachmentReference() const;

	// Multisampling operation methods
	ITextureBuffer::SampleCount GetSampleCount() const;
	VkSampleCountFlagBits GetSampleCountNative() const;
	bool HasResolveTargets() const;
	void CompleteRenderPassResolveOperationWithCopy(VkCommandBuffer commandBuffer, AttachmentType sourceType, size_t sourceIndex, AttachmentType resolveType, size_t resolveIndex) const;

	// Viewport methods
	void DefineViewportRegion(const V2UInt32& startPos, const V2UInt32& size) override;
	void DefineScissorRegion(const V2UInt32& startPos, const V2UInt32& size) override;
	void RemoveScissorRegion() override;
	const VkViewport* GetViewportDefinition() const;
	const VkRect2D* GetScissorDefinition() const;
	V2UInt32 GetFrameBufferSizeInPixels() const;
	int GetViewportLastUpdateToken() const;

	// Output capture methods
	bool HasCaptureTargets() const;
	void AddOutputCaptureTarget(IFrameBufferOutput* captureTarget, AttachmentType type, size_t index) override;
	void RemoveOutputCaptureTarget(IFrameBufferOutput* captureTarget) override;
	void CaptureFrameBufferOutput(VkCommandBuffer commandBuffer);
	void CompleteCaptureFrameBufferOutput();

	// Update methods
	void AcquireNextImage();
	void PresentToWindow();
	VkSemaphore GetPrepareSemaphore();
	VkSemaphore GetPresentSemaphore();

	// Build state methods
	void MigrateBuildStateToDrawState();

private:
	// Structures
	struct BoundTextureInfo
	{
		AttachmentType type;
		size_t index;
		VulkanTextureBuffer2D* texture;
		int attachmentNo;
		int colorAttachmentIndex;
	};
	struct CaptureTargetInfo
	{
		AttachmentType type;
		size_t index;
		VulkanFrameBufferOutput* captureTarget;
		bool completionPending = false;
		VkBuffer stagingBuffer;
		VmaAllocation stagingAllocation;
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
	struct MutableState
	{
		bool framebufferInvalid = false;
		bool viewportChanged = false;
		bool boundToWindow = false;
		bool headlessWindow = false;
		bool hasValidWindowType = false;
		bool scissorRegionDefined = false;
		std::variant<std::monostate,
		             WindowInfoHeadless
#ifdef COBALT_RENDERER_WIN32_SUPPORT
		             ,
		             WindowInfoWin32
#endif
#ifdef COBALT_RENDERER_XLIB_SUPPORT
		             ,
		             WindowInfoXlib
#endif
#ifdef COBALT_RENDERER_XCB_SUPPORT
		             ,
		             WindowInfoXCB
#endif
#ifdef COBALT_RENDERER_WAYLAND_SUPPORT
		             ,
		             WindowInfoWayland
#endif
#ifdef COBALT_RENDERER_APPKIT_SUPPORT
		             ,
		             WindowInfoAppKit
#endif
#ifdef COBALT_RENDERER_METALLAYER_SUPPORT
		             ,
		             WindowInfoMetalLayer
#endif
		             >
		  windowInfo;
		WindowDepthStencilMode windowDepthStencilMode = {};
		WindowColorSpaceMode windowColorSpaceMode = {};
		WindowBindingFlags windowBindingFlags = WindowBindingFlags::None;
		IFrameBuffer::WindowInfoBase::WindowType windowType = {};
		V2UInt32 windowSizeInPixels = {};
		V2UInt32 viewportRegionStartPos = {};
		V2UInt32 viewportRegionSize = {};
		V2UInt32 scissorRegionStartPos = {};
		V2UInt32 scissorRegionSize = {};
		std::vector<BoundTextureInfo> boundTextures;
		std::vector<BoundTextureInfo> resolveTextures;
		std::vector<CaptureTargetInfo> captureTargets;
	};

private:
	// Viewport methods
	void UpdateViewport();

	// Framebuffer update methods
	bool CreateWindowObjects(VkCommandBuffer commandBuffer);
	bool CreateTextureObjects(VkCommandBuffer commandBuffer);
	bool CreateTemplateRenderPass(VkCommandBuffer commandBuffer);
	bool CreateSurface(WindowColorSpaceMode windowColorSpaceMode);
	void DeleteWindowObjects();
	void DeleteTextureObjects();
	static void GetWindowColorFormat(WindowColorSpaceMode mode, VkFormat& format, VkColorSpaceKHR& colorSpace);
	static bool IsDepthStencilFormatSupported(VkPhysicalDevice physicalDevice, VkFormat format);
	static void GetWindowDepthStencilFormat(WindowDepthStencilMode mode, VkPhysicalDevice physicalDevice, VkFormat& format, bool& hasScencilComponent);

	// Output capture methods
	bool CaptureAttachmentTarget(VkCommandBuffer commandBuffer, CaptureTargetInfo& captureTargetInfo, VkImage sourceImage, VkFormat format, VkImageLayout initialLayout, uint32_t width, uint32_t height, ITextureBuffer::ImageFormat imageFormat, ITextureBuffer::DataFormat dataFormat, size_t elementCount, size_t elementSizeInBytes, size_t pixelOffsetInBytes, size_t pixelStrideInBytes);
	bool CompleteCaptureFrameBufferOutput(const CaptureTargetInfo& captureTargetInfo);
	static bool GetFormatFromNativeFormat(VkFormat nativeFormat, ITextureBuffer::ImageFormat& imageFormat, ITextureBuffer::DataFormat& dataFormat, size_t& elementCount, size_t& elementSizeInBytes, size_t& pixelOffsetInBytes, size_t& pixelStrideInBytes, bool stencilComponent);

	// Build state methods
	void FlagBuildStateModified();

private:
	cobalt::logging::ILogger* _log;
	VulkanRenderer* _renderer;

	mutable std::mutex _accessMutex;
	std::atomic_flag _stateModified = ATOMIC_FLAG_INIT;
	MutableState _buildState = {};
	MutableState _drawState = {};

	bool _framebufferCreated;
	bool _firstFrameForWindowBufferPending;
	bool _enableVerticalSync;
	bool _tearingSupported;

	// Surface and swapchain support
	VkSemaphore _prepareSemaphore = VK_NULL_HANDLE;
	VkSemaphore _presentSemaphore = VK_NULL_HANDLE;
	VkQueue _presentQueue = VK_NULL_HANDLE;
	VkSurfaceKHR _surface = VK_NULL_HANDLE;
	VkSwapchainKHR _swapchain = VK_NULL_HANDLE;
	VmaAllocation _headlessColorAllocation = VK_NULL_HANDLE;
	VkImage _headlessColorImage = VK_NULL_HANDLE;
	VkViewport _viewport = {};
	VkRect2D _scissorRect = {};
	VkSurfaceCapabilitiesKHR _surfaceCapabilities = {};
	std::vector<VkPresentModeKHR> _surfacePresentModes;
	std::vector<VkImage> _swapchainImages;
	std::vector<VkImageView> _swapchainImageViews;
	std::vector<VkFramebuffer> _swapchainFramebuffers;
	std::vector<VkSemaphore> _swapchainPresentSemaphores;
	VmaAllocation _swapchainDepthAllocation = VK_NULL_HANDLE;
	VkImage _swapchainDepthImage = VK_NULL_HANDLE;
	VkImageView _swapchainDepthImageView = VK_NULL_HANDLE;
	uint32_t _swapchainImageIndex = 0;
	VkSemaphore _imageAvailableSemaphore = VK_NULL_HANDLE;
	VkSurfaceFormatKHR _bestSurfaceFormat = {};
	VkFormat _depthStencilTextureFormat = VK_FORMAT_UNDEFINED;
	VkExtent2D _surfaceExtent = {};
	VkFramebuffer _textureFramebuffer = VK_NULL_HANDLE;
	VkRenderPass _templateRenderPass = VK_NULL_HANDLE;
	VkPresentInfoKHR _presentInfo = {};
	V2UInt32 _framebufferSize = {};
	ITextureBuffer::SampleCount _sampleCount = {};
	VkSampleCountFlagBits _sampleCountNative = {};
	std::vector<VkAttachmentDescription> _renderPassAttachments;
	std::vector<VkAttachmentReference> _renderPassColorAttachmentReferences;
	std::vector<VkAttachmentReference> _renderPassDepthStencilAttachmentReferences;
	int _viewportLastUpdateToken = 1;
	int _frameBufferObjectLastUpdateToken = 1;
};

} // namespace cobalt::graphics
