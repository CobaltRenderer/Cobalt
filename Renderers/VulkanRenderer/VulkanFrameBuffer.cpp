// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "VulkanFrameBuffer.h"
#include "VulkanFrameBufferOutput.h"
#include "VulkanHeaders.h"
#include "VulkanMemoryManager.h"
#include "VulkanRenderer.h"
#include "VulkanTextureBuffer2D.h"
#include <Cobalt/LoggingInterface/LoggingInterface.pkg>
#include <Cobalt/RendererInterface/PlatformBindings.pkg>
#include <Cobalt/RendererInterface/RendererInterface.pkg>
#include <Internal/RendererSupport/KnownDynamicCast.h>
#include <array>
#include <cmath>
#include <limits>
#include <vector>
#ifdef _WIN32
#include <libloaderapi.h>
#endif
#ifdef __APPLE__
#import <QuartzCore/CAMetalLayer.h>
#endif
namespace cobalt::graphics {

//----------------------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------------------
VulkanFrameBuffer::VulkanFrameBuffer(cobalt::logging::ILogger* log, VulkanRenderer* renderer)
: _log(log)
{
	_log = log;
	_renderer = renderer;

	// Build state
	_buildState.viewportRegionStartPos = {};
	_buildState.viewportRegionSize = {};
	_buildState.framebufferInvalid = true;
	_buildState.boundToWindow = false;
	_buildState.headlessWindow = false;
	_buildState.viewportChanged = false;
	_buildState.scissorRegionDefined = false;

	_framebufferCreated = false;
	_firstFrameForWindowBufferPending = false;
	_enableVerticalSync = false;
	_tearingSupported = false;
	_viewportLastUpdateToken = 1;
	_frameBufferObjectLastUpdateToken = 1;

	_prepareSemaphore = VK_NULL_HANDLE;
	_presentSemaphore = VK_NULL_HANDLE;
}

//----------------------------------------------------------------------------------------
VulkanFrameBuffer::~VulkanFrameBuffer()
{
	if (_framebufferCreated && _drawState.boundToWindow)
	{
		DeleteWindowObjects();
	}
	else if (_framebufferCreated)
	{
		DeleteTextureObjects();
	}
	if (_prepareSemaphore != VK_NULL_HANDLE)
	{
		vkDestroySemaphore(_renderer->GetDevice(), _prepareSemaphore, nullptr);
	}
	for (auto& entry : _swapchainPresentSemaphores)
	{
		vkDestroySemaphore(_renderer->GetDevice(), entry, nullptr);
	}
}

//----------------------------------------------------------------------------------------
// Initialization methods
//----------------------------------------------------------------------------------------
void VulkanFrameBuffer::Delete()
{
	_renderer->DeleteObject(this);
}

//----------------------------------------------------------------------------------------
// Binding methods
//----------------------------------------------------------------------------------------
SuccessToken VulkanFrameBuffer::BindTexture(ITextureBuffer2D* texture, AttachmentType type, size_t index)
{
	// Ensure the specified texture is able to be bound
	auto* textureResolved = KnownDynamicCast<VulkanTextureBuffer2D*>(texture);
	if (((uint32_t)textureResolved->GetUsageFlags() & (uint32_t)ITextureBuffer::UsageFlags::FrameBufferOutput) == 0)
	{
		_log->Error("Attempted to bind texture to framebuffer when the usage flags for the texture don't allow framebuffer output.");
		return false;
	}

	// Add this texture to the list of bound textures for this framebuffer
	std::unique_lock<std::mutex> lock(_accessMutex);
	BoundTextureInfo textureInfo{};
	textureInfo.type = type;
	textureInfo.index = index;
	textureInfo.texture = textureResolved;
	bool foundEntry = false;
	for (auto& entry : _buildState.boundTextures)
	{
		if ((entry.type == type) && (entry.index == index))
		{
			entry = textureInfo;
			foundEntry = true;
			break;
		}
	}
	if (!foundEntry)
	{
		_buildState.boundTextures.push_back(textureInfo);
	}

	// Update the framebuffer state, and mark it as modified.
	_buildState.boundToWindow = false;
	_buildState.headlessWindow = false;
	_buildState.hasValidWindowType = false;
	_buildState.windowInfo = std::monostate();
	_buildState.framebufferInvalid = true;
	lock.unlock();
	FlagBuildStateModified();
	return true;
}

//----------------------------------------------------------------------------------------
void VulkanFrameBuffer::UnbindTexture(AttachmentType type, size_t index)
{
	// Attempt to locate the target bound texture entry
	std::unique_lock<std::mutex> lock(_accessMutex);
	bool foundEntry = false;
	auto boundTexturesIterator = _buildState.boundTextures.begin();
	while (boundTexturesIterator != _buildState.boundTextures.end())
	{
		if ((boundTexturesIterator->type == type) && (boundTexturesIterator->index == index))
		{
			foundEntry = true;
			break;
		}
		++boundTexturesIterator;
	}
	if (!foundEntry)
	{
		return;
	}

	// Remove this texture from the list of bound textures for this framebuffer
	_buildState.boundTextures.erase(boundTexturesIterator);

	// Update the framebuffer state, and mark it as modified.
	_buildState.framebufferInvalid = true;
	lock.unlock();
	FlagBuildStateModified();
}

//----------------------------------------------------------------------------------------
SuccessToken VulkanFrameBuffer::BindMultiSamplingResolveTexture(ITextureBuffer2D* texture, AttachmentType type, size_t index)
{
	// Ensure the specified texture is able to be bound
	auto* textureResolved = KnownDynamicCast<VulkanTextureBuffer2D*>(texture);
	if (((uint32_t)textureResolved->GetUsageFlags() & (uint32_t)ITextureBuffer::UsageFlags::MultiSampleResolve) == 0)
	{
		_log->Error("Attempted to bind resolve texture to framebuffer when the usage flags for the texture don't allow multisample texture resolution.");
		return false;
	}
	if (type != IFrameBuffer::AttachmentType::Color)
	{
		//##TODO## The VK_KHR_depth_stencil_resolve extension adds support for resolution of depth/stencil targets. It
		// has around 50% support on Windows at this time, and hardware support is provided across NVidia, AMD, and Intel
		// hardware. We should consider adding support for this in the future, as it's a very useful feature.
		_log->Error("Attempted to bind resolve texture with type {0}, but only resolution of color targets are supported at this time.", type);
		return false;
	}
	if (textureResolved->GetSampleCount() != ITextureBuffer::SampleCount::SampleCount1)
	{
		_log->Error("Attempted to bind resolve texture with sample count {0}, but resolve textures must have a sample count of {1}.", textureResolved->GetSampleCount(), ITextureBuffer::SampleCount::SampleCount1);
		return false;
	}

	// Add this texture to the list of bound resolve textures for this framebuffer
	std::unique_lock<std::mutex> lock(_accessMutex);
	BoundTextureInfo textureInfo{};
	textureInfo.type = type;
	textureInfo.index = index;
	textureInfo.texture = textureResolved;
	bool foundEntry = false;
	for (auto& entry : _buildState.resolveTextures)
	{
		if ((entry.type == type) && (entry.index == index))
		{
			entry = textureInfo;
			foundEntry = true;
			break;
		}
	}
	if (!foundEntry)
	{
		_buildState.resolveTextures.push_back(textureInfo);
	}

	// Update the framebuffer state, and mark it as modified.
	_buildState.framebufferInvalid = true;
	lock.unlock();
	FlagBuildStateModified();
	return true;
}

//----------------------------------------------------------------------------------------
void VulkanFrameBuffer::UnbindMultiSamplingResolveTexture(AttachmentType type, size_t index)
{
	// Attempt to locate the target resolve texture entry
	std::unique_lock<std::mutex> lock(_accessMutex);
	bool foundEntry = false;
	auto resolveTexturesIterator = _buildState.resolveTextures.begin();
	while (resolveTexturesIterator != _buildState.resolveTextures.end())
	{
		if ((resolveTexturesIterator->type == type) && (resolveTexturesIterator->index == index))
		{
			foundEntry = true;
			break;
		}
		++resolveTexturesIterator;
	}
	if (!foundEntry)
	{
		return;
	}

	// Remove this texture from the list of resolve textures for this framebuffer
	_buildState.resolveTextures.erase(resolveTexturesIterator);

	// Update the framebuffer state, and mark it as modified.
	_buildState.framebufferInvalid = true;
	lock.unlock();
	FlagBuildStateModified();
}

//----------------------------------------------------------------------------------------
SuccessToken VulkanFrameBuffer::BindWindow(const WindowInfoBase& windowInfo, WindowDepthStencilMode depthStencilMode, WindowColorSpaceMode colorSpaceMode, WindowBindingFlags bindingFlags)
{
	// Update the framebuffer state, and mark it as modified.
	std::unique_lock<std::mutex> lock(_accessMutex);
	if ((windowInfo.windowType == IFrameBuffer::WindowInfoBase::WindowType::Headless) && (windowInfo.structureSizeInBytes == sizeof(WindowInfoHeadless)))
	{
		auto windowInfoResolved = reinterpret_cast<const WindowInfoHeadless*>(&windowInfo);
		_buildState.windowInfo = *windowInfoResolved;
		_buildState.windowType = windowInfo.windowType;
		_buildState.windowSizeInPixels = windowInfoResolved->windowSizeInPixels;
		_buildState.headlessWindow = true;
	}
	else
#ifdef COBALT_RENDERER_WIN32_SUPPORT
	  if ((windowInfo.windowType == IFrameBuffer::WindowInfoBase::WindowType::Win32) && (windowInfo.structureSizeInBytes == sizeof(WindowInfoWin32)))
	{
		auto windowInfoResolved = reinterpret_cast<const WindowInfoWin32*>(&windowInfo);
		_buildState.windowInfo = *windowInfoResolved;
		_buildState.windowType = windowInfo.windowType;
		_buildState.windowSizeInPixels = windowInfoResolved->windowSizeInPixels;
		_buildState.headlessWindow = false;
	}
	else
#endif
#ifdef COBALT_RENDERER_XLIB_SUPPORT
	  if ((windowInfo.windowType == IFrameBuffer::WindowInfoBase::WindowType::Xlib) && (windowInfo.structureSizeInBytes == sizeof(WindowInfoXlib)))
	{
		auto windowInfoResolved = reinterpret_cast<const WindowInfoXlib*>(&windowInfo);
		_buildState.windowInfo = *windowInfoResolved;
		_buildState.windowType = windowInfo.windowType;
		_buildState.windowSizeInPixels = windowInfoResolved->windowSizeInPixels;
		_buildState.headlessWindow = false;
	}
	else
#endif
#ifdef COBALT_RENDERER_XCB_SUPPORT
	  if ((windowInfo.windowType == IFrameBuffer::WindowInfoBase::WindowType::XCB) && (windowInfo.structureSizeInBytes == sizeof(WindowInfoXCB)))
	{
		auto windowInfoResolved = reinterpret_cast<const WindowInfoXCB*>(&windowInfo);
		_buildState.windowInfo = *windowInfoResolved;
		_buildState.windowType = windowInfo.windowType;
		_buildState.windowSizeInPixels = windowInfoResolved->windowSizeInPixels;
		_buildState.headlessWindow = false;
	}
	else
#endif
#ifdef COBALT_RENDERER_WAYLAND_SUPPORT
	  if ((windowInfo.windowType == IFrameBuffer::WindowInfoBase::WindowType::Wayland) && (windowInfo.structureSizeInBytes == sizeof(WindowInfoWayland)))
	{
		auto windowInfoResolved = reinterpret_cast<const WindowInfoWayland*>(&windowInfo);
		_buildState.windowInfo = *windowInfoResolved;
		_buildState.windowType = windowInfo.windowType;
		_buildState.windowSizeInPixels = windowInfoResolved->windowSizeInPixels;
		_buildState.headlessWindow = false;
	}
	else
#endif
#ifdef COBALT_RENDERER_APPKIT_SUPPORT
	  if ((windowInfo.windowType == IFrameBuffer::WindowInfoBase::WindowType::AppKit) && (windowInfo.structureSizeInBytes == sizeof(WindowInfoAppKit)))
	{
		auto windowInfoResolved = reinterpret_cast<const WindowInfoAppKit*>(&windowInfo);
		_buildState.windowInfo = *windowInfoResolved;
		_buildState.windowType = windowInfo.windowType;
		_buildState.windowSizeInPixels = windowInfoResolved->windowSizeInPixels;
		_buildState.headlessWindow = false;

		// Allocate a new CAMetalLayer and associate it with the view if required. Note that we can reuse an existing
		// layer if one is present, as long as it's not being used. We can't safely detach it if it's in use anyway
		// though, so we're pushing that one back on the caller. Note that we do NOT need to keep a reference to the
		// layer itself to keep it alive, or handle cleanup. We can safely detatch from the layer, and reattach later
		// if we choose, and the layer remains owned and alive on the view. If the caller wants to clean up the layer
		// after we detach, we leave that to them. This avoids the very messy problem of trying to remove this
		// deterministically, when we can only detach/destroy the layer on the UI thread.
		auto view = (__bridge NSView*)windowInfoResolved->view;
		if ((view.wantsLayer != YES) || (view.layer == nil) || ([view.layer isKindOfClass:[CAMetalLayer class]] != YES))
		{
			// Create and bind a new CAMetalLayer object. Note that as per documentation, we need to set wantsLayer
			// after we call setLayer, not the other way around, as we want a layer-hosting view here. See the
			// following for more info:
			// https://developer.apple.com/documentation/appkit/nsview/wantslayer
			CAMetalLayer* metalLayer = [CAMetalLayer layer];
			NSSize viewSizeInFractionalPixels = [view convertSizeToBacking:view.bounds.size];
			CGSize viewSizeInPixels = CGSizeMake(std::floor(viewSizeInFractionalPixels.width), std::floor(viewSizeInFractionalPixels.height));
			metalLayer.drawableSize = viewSizeInPixels;
			metalLayer.contentsScale = view.window.backingScaleFactor;
			[view setLayer:metalLayer];
			view.wantsLayer = YES;
		}
	}
	else
#endif
	{
		_log->Error("Could not bind to window. Unsupported window binding structure supplied with type \"{0}\" and size \"{1}\".", windowInfo.windowType, windowInfo.structureSizeInBytes);
		return false;
	}
	_buildState.windowDepthStencilMode = depthStencilMode;
	_buildState.windowColorSpaceMode = colorSpaceMode;
	_buildState.windowBindingFlags = bindingFlags;
	_buildState.boundTextures.clear();
	_buildState.boundToWindow = true;
	_buildState.framebufferInvalid = true;
	_buildState.viewportChanged = true;
	lock.unlock();
	FlagBuildStateModified();
	return true;
}

//----------------------------------------------------------------------------------------
SuccessToken VulkanFrameBuffer::NotifyWindowResized(const V2UInt32& windowSizeInPixels)
{
	// Ensure we're currently bound to a window
	std::unique_lock<std::mutex> lock(_accessMutex);
	if (!_buildState.boundToWindow)
	{
		_log->Warning("NotifyWindowResized called when the framebuffer is not currently bound to a window.");
		return false;
	}

	// Update the framebuffer state, and mark it as modified.
	switch (_buildState.windowType)
	{
	case IFrameBuffer::WindowInfoBase::WindowType::Headless:
		if (auto* windowInfo = std::get_if<WindowInfoHeadless>(&_buildState.windowInfo))
		{
			windowInfo->windowSizeInPixels = windowSizeInPixels;
			_buildState.windowSizeInPixels = windowSizeInPixels;
		}
		break;
#ifdef COBALT_RENDERER_WIN32_SUPPORT
	case IFrameBuffer::WindowInfoBase::WindowType::Win32:
		if (auto* windowInfo = std::get_if<WindowInfoWin32>(&_buildState.windowInfo))
		{
			windowInfo->windowSizeInPixels = windowSizeInPixels;
			_buildState.windowSizeInPixels = windowSizeInPixels;
		}
		break;
#endif
#ifdef COBALT_RENDERER_XLIB_SUPPORT
	case IFrameBuffer::WindowInfoBase::WindowType::Xlib:
		if (auto* windowInfo = std::get_if<WindowInfoXlib>(&_buildState.windowInfo))
		{
			windowInfo->windowSizeInPixels = windowSizeInPixels;
			_buildState.windowSizeInPixels = windowSizeInPixels;
		}
		break;
#endif
#ifdef COBALT_RENDERER_XCB_SUPPORT
	case IFrameBuffer::WindowInfoBase::WindowType::XCB:
		if (auto* windowInfo = std::get_if<WindowInfoXCB>(&_buildState.windowInfo))
		{
			windowInfo->windowSizeInPixels = windowSizeInPixels;
			_buildState.windowSizeInPixels = windowSizeInPixels;
		}
		break;
#endif
#ifdef COBALT_RENDERER_WAYLAND_SUPPORT
	case IFrameBuffer::WindowInfoBase::WindowType::Wayland:
		if (auto* windowInfo = std::get_if<WindowInfoWayland>(&_buildState.windowInfo))
		{
			windowInfo->windowSizeInPixels = windowSizeInPixels;
			_buildState.windowSizeInPixels = windowSizeInPixels;
		}
		break;
#endif
#ifdef COBALT_RENDERER_APPKIT_SUPPORT
	case IFrameBuffer::WindowInfoBase::WindowType::AppKit:
		if (auto* windowInfo = std::get_if<WindowInfoAppKit>(&_buildState.windowInfo))
		{
			windowInfo->windowSizeInPixels = windowSizeInPixels;
			_buildState.windowSizeInPixels = windowSizeInPixels;
		}
		break;
#endif
#ifdef COBALT_RENDERER_METALLAYER_SUPPORT
	case IFrameBuffer::WindowInfoBase::WindowType::MetalLayer:
		if (auto* windowInfo = std::get_if<WindowInfoMetalLayer>(&_buildState.windowInfo))
		{
			windowInfo->windowSizeInPixels = windowSizeInPixels;
			_buildState.windowSizeInPixels = windowSizeInPixels;
		}
		break;
#endif
	}

	_buildState.viewportChanged = true;
	lock.unlock();
	FlagBuildStateModified();
	return true;
}

//----------------------------------------------------------------------------------------
void VulkanFrameBuffer::PrepareFrameBuffer(VkCommandBuffer commandBuffer)
{
	// Delete objects if they are outdated
	if ((_drawState.framebufferInvalid || _drawState.viewportChanged) && _framebufferCreated)
	{
		if (_drawState.boundToWindow)
		{
			if (!_drawState.headlessWindow)
			{
				// Since we might be re-creating the swapchain, wait for the device to be idle. This is heavyweight, but
				// there could be a pending vkAcquireNextImageKHR call, and we need to ensure it's complete before we delete
				// the swapchain. We also need to re-create the present semaphore to get it back into an unsignalled state.
				// We first let all pending work complete, then we delete the prepare semaphore.
				if (_prepareSemaphore != VK_NULL_HANDLE)
				{
					vkDeviceWaitIdle(_renderer->GetDevice());
					vkDestroySemaphore(_renderer->GetDevice(), _prepareSemaphore, nullptr);
					_prepareSemaphore = VK_NULL_HANDLE;
				}
			}

			// Delete all remaining window objects now that we're synchronized with vkAcquireNextImageKHR
			DeleteWindowObjects();
		}
		else
		{
			DeleteTextureObjects();
		}
	}

	// Create new framebuffer objects if needed
	if (_drawState.framebufferInvalid || _drawState.viewportChanged)
	{
		if (_drawState.viewportChanged)
		{
			UpdateViewport();
		}
		if (_drawState.boundToWindow)
		{
			CreateWindowObjects(commandBuffer);

			// Create our prepare semaphore if required
			if (!_drawState.headlessWindow && (_prepareSemaphore == VK_NULL_HANDLE))
			{
				VkSemaphoreCreateInfo semaphoreInfo = {};
				semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
				semaphoreInfo.flags = 0;
				VkResult createPrepareSemaphoreResult = vkCreateSemaphore(_renderer->GetDevice(), &semaphoreInfo, nullptr, &_prepareSemaphore);
				if (createPrepareSemaphoreResult != VK_SUCCESS)
				{
					_log->Error("Could not create prepare semaphore for framebuffer with error code {0}", createPrepareSemaphoreResult);
				}
			}

			// Create our present semaphores if required
			if (!_drawState.headlessWindow && _swapchainPresentSemaphores.empty())
			{
				_swapchainPresentSemaphores.resize(_swapchainImages.size());
				VkSemaphoreCreateInfo semaphoreInfo = {};
				semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
				semaphoreInfo.flags = 0;
				for (auto& entry : _swapchainPresentSemaphores)
				{
					VkResult createPresentSemaphoreResult = vkCreateSemaphore(_renderer->GetDevice(), &semaphoreInfo, nullptr, &entry);
					if (createPresentSemaphoreResult != VK_SUCCESS)
					{
						_log->Error("Could not create present semaphore for framebuffer with error code {0}", createPresentSemaphoreResult);
					}
				}
			}

			if (!_drawState.headlessWindow)
			{
				AcquireNextImage();
			}
		}
		else
		{
			CreateTextureObjects(commandBuffer);
		}

		// Increment the framebuffer object last update token
		if (++_frameBufferObjectLastUpdateToken > 100000)
		{
			_frameBufferObjectLastUpdateToken = 1;
		}
	}

	// Flag that our framebuffer objects have now been created
	_drawState.framebufferInvalid = false;
	_drawState.viewportChanged = false;
	_framebufferCreated = true;
}

//----------------------------------------------------------------------------------------
VkFramebuffer VulkanFrameBuffer::GetFramebuffer() const
{
	if (_drawState.boundToWindow)
	{
		return _swapchainFramebuffers[_swapchainImageIndex];
	}
	return _textureFramebuffer;
}

//----------------------------------------------------------------------------------------
VkRenderPass VulkanFrameBuffer::GetTemplateRenderPass() const
{
	return _templateRenderPass;
}

//----------------------------------------------------------------------------------------
bool VulkanFrameBuffer::IsBoundToWindow() const
{
	return _drawState.boundToWindow && !_drawState.headlessWindow;
}

//----------------------------------------------------------------------------------------
VulkanFrameBuffer::AttachmentFormat VulkanFrameBuffer::GetColorAttachmentFormat(size_t index) const
{
	// If we're bound to a window, only floating point and normalized targets are supported, so return the float type
	// immediately to the caller.
	if (_drawState.boundToWindow)
	{
		return AttachmentFormat::Float;
	}

	// Attempt to locate the target bound texture entry
	std::unique_lock<std::mutex> lock(_accessMutex);
	bool foundEntry = false;
	auto boundTexturesIterator = _drawState.boundTextures.begin();
	while (boundTexturesIterator != _drawState.boundTextures.end())
	{
		if ((boundTexturesIterator->type == IFrameBuffer::AttachmentType::Color) && (boundTexturesIterator->index == index))
		{
			foundEntry = true;
			break;
		}
		++boundTexturesIterator;
	}
	if (!foundEntry)
	{
		return AttachmentFormat::Float;
	}
	const auto& entry = *boundTexturesIterator;

	// Return the basic format of the target texture
	auto dataFormat = entry.texture->AllocatedDataFormat();
	switch (dataFormat)
	{
	case ITextureBuffer::DataFormat::Int8:
	case ITextureBuffer::DataFormat::Int16:
	case ITextureBuffer::DataFormat::Int32:
		return AttachmentFormat::Int;
	case ITextureBuffer::DataFormat::UInt8:
	case ITextureBuffer::DataFormat::UInt16:
	case ITextureBuffer::DataFormat::UInt32:
		return AttachmentFormat::UInt;
	case ITextureBuffer::DataFormat::Norm8:
	case ITextureBuffer::DataFormat::Norm16:
	case ITextureBuffer::DataFormat::UNorm8:
	case ITextureBuffer::DataFormat::UNorm16:
	case ITextureBuffer::DataFormat::Float16:
	case ITextureBuffer::DataFormat::Float32:
		return AttachmentFormat::Float;
	}
	return AttachmentFormat::Float;
}

//----------------------------------------------------------------------------------------
int VulkanFrameBuffer::GetRenderPassAttachmentIndex(AttachmentType type, size_t index) const
{
	if (_drawState.boundToWindow)
	{
		if (type == IFrameBuffer::AttachmentType::Color)
		{
			return 0;
		}
		if (_drawState.windowDepthStencilMode != WindowDepthStencilMode::None)
		{
			return 1;
		}
	}
	else
	{
		for (const auto& texture : _drawState.boundTextures)
		{
			if ((texture.type == type) && (texture.index == index))
			{
				return texture.attachmentNo;
			}
		}
	}
	return -1;
}

//----------------------------------------------------------------------------------------
int VulkanFrameBuffer::GetRenderPassResolveAttachmentIndex(AttachmentType type, size_t index) const
{
	if (!_drawState.boundToWindow)
	{
		for (const auto& texture : _drawState.resolveTextures)
		{
			if ((texture.type == type) && (texture.index == index))
			{
				return texture.attachmentNo;
			}
		}
	}
	return -1;
}

//----------------------------------------------------------------------------------------
int VulkanFrameBuffer::GetRenderPassColorAttachmentIndex(int attachmentIndex) const
{
	if (!_drawState.boundToWindow)
	{
		for (const auto& texture : _drawState.boundTextures)
		{
			if ((texture.type == AttachmentType::Color) && (texture.attachmentNo == attachmentIndex))
			{
				return texture.colorAttachmentIndex;
			}
		}
	}
	return -1;
}

//----------------------------------------------------------------------------------------
size_t VulkanFrameBuffer::GetRenderPassAttachmentCount() const
{
	return _renderPassAttachments.size();
}

//----------------------------------------------------------------------------------------
size_t VulkanFrameBuffer::GetRenderPassColorAttachmentReferenceCount() const
{
	return _renderPassColorAttachmentReferences.size();
}

//----------------------------------------------------------------------------------------
int VulkanFrameBuffer::GetFrameBufferObjectLastUpdateToken() const
{
	return _frameBufferObjectLastUpdateToken;
}

//----------------------------------------------------------------------------------------
const VkAttachmentDescription* VulkanFrameBuffer::GetRenderPassAttachments() const
{
	return (!_renderPassAttachments.empty() ? _renderPassAttachments.data() : nullptr);
}

//----------------------------------------------------------------------------------------
const VkAttachmentReference* VulkanFrameBuffer::GetRenderPassColorAttachmentReferences() const
{
	return (!_renderPassColorAttachmentReferences.empty() ? _renderPassColorAttachmentReferences.data() : nullptr);
}

//----------------------------------------------------------------------------------------
const VkAttachmentReference* VulkanFrameBuffer::GetRenderPassDepthStencilAttachmentReference() const
{
	return (!_renderPassDepthStencilAttachmentReferences.empty() ? _renderPassDepthStencilAttachmentReferences.data() : nullptr);
}

//----------------------------------------------------------------------------------------
// Multisampling operation methods
//----------------------------------------------------------------------------------------
ITextureBuffer::SampleCount VulkanFrameBuffer::GetSampleCount() const
{
	return _sampleCount;
}

//----------------------------------------------------------------------------------------
VkSampleCountFlagBits VulkanFrameBuffer::GetSampleCountNative() const
{
	return _sampleCountNative;
}

//----------------------------------------------------------------------------------------
bool VulkanFrameBuffer::HasResolveTargets() const
{
	return !_drawState.resolveTextures.empty();
}

//----------------------------------------------------------------------------------------
void VulkanFrameBuffer::CompleteRenderPassResolveOperationWithCopy(VkCommandBuffer commandBuffer, AttachmentType sourceType, size_t sourceIndex, AttachmentType resolveType, size_t resolveIndex) const
{
	// Find the source texture
	const BoundTextureInfo* sourceTextureInfo = nullptr;
	for (const auto& entry : _drawState.boundTextures)
	{
		if ((entry.type == sourceType) && (entry.index == sourceIndex))
		{
			sourceTextureInfo = &entry;
			break;
		}
	}
	if (sourceTextureInfo == nullptr)
	{
		return;
	}
	auto sourceTexture = sourceTextureInfo->texture;

	// Find the target resolve texture
	const BoundTextureInfo* resolveTextureInfo = nullptr;
	for (const auto& entry : _drawState.resolveTextures)
	{
		if ((entry.type == resolveType) && (entry.index == resolveIndex))
		{
			resolveTextureInfo = &entry;
			break;
		}
	}
	if (resolveTextureInfo == nullptr)
	{
		return;
	}
	auto resolveTexture = resolveTextureInfo->texture;

	// Perform the copy operation
	VulkanMemoryManager* memoryManager = _renderer->GetMemoryManager();
	auto sourceImage = sourceTexture->GetImage();
	auto resolveImage = resolveTexture->GetImage();
	auto sourceFormat = sourceTexture->GetNativeFormat();
	auto resolveFormat = resolveTexture->GetNativeFormat();
	auto sourceLayout = sourceTexture->GetDefaultImageLayout();
	auto resolveLayout = resolveTexture->GetDefaultImageLayout();
	auto sourceDimensions = sourceTexture->MipmapLevelDimensions(0);
	memoryManager->RecordTransitionImageLayout(commandBuffer, sourceImage, sourceFormat, 1, 1, sourceLayout, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
	// The copy overwrites the full resolve target, so discard its previous contents rather than requiring the image to
	// have already been transitioned into its default layout.
	memoryManager->RecordTransitionImageLayout(commandBuffer, resolveImage, resolveFormat, 1, 1, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
	memoryManager->RecordCopyImage(commandBuffer, sourceImage, resolveImage, sourceDimensions.X(), sourceDimensions.Y(), VK_IMAGE_ASPECT_COLOR_BIT);
	memoryManager->RecordTransitionImageLayout(commandBuffer, sourceImage, sourceFormat, 1, 1, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, sourceLayout);
	memoryManager->RecordTransitionImageLayout(commandBuffer, resolveImage, resolveFormat, 1, 1, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, resolveLayout);
}

//----------------------------------------------------------------------------------------
// Viewport methods
//----------------------------------------------------------------------------------------
void VulkanFrameBuffer::DefineViewportRegion(const V2UInt32& startPos, const V2UInt32& size)
{
	// Update the framebuffer state, and mark it as modified.
	std::unique_lock<std::mutex> lock(_accessMutex);
	_buildState.viewportRegionStartPos = startPos;
	_buildState.viewportRegionSize = size;
	_buildState.viewportChanged = true;
	lock.unlock();
	FlagBuildStateModified();
}

//----------------------------------------------------------------------------------------
void VulkanFrameBuffer::DefineScissorRegion(const V2UInt32& startPos, const V2UInt32& size)
{
	// Update the framebuffer state, and mark it as modified.
	std::unique_lock<std::mutex> lock(_accessMutex);
	_buildState.scissorRegionDefined = true;
	_buildState.scissorRegionStartPos = startPos;
	_buildState.scissorRegionSize = size;
	_buildState.viewportChanged = true;
	lock.unlock();
	FlagBuildStateModified();
}

//----------------------------------------------------------------------------------------
void VulkanFrameBuffer::RemoveScissorRegion()
{
	// Update the framebuffer state, and mark it as modified.
	std::unique_lock<std::mutex> lock(_accessMutex);
	_buildState.scissorRegionDefined = false;
	_buildState.viewportChanged = true;
	lock.unlock();
	FlagBuildStateModified();
}

//----------------------------------------------------------------------------------------
const VkViewport* VulkanFrameBuffer::GetViewportDefinition() const
{
	return &_viewport;
}

//----------------------------------------------------------------------------------------
const VkRect2D* VulkanFrameBuffer::GetScissorDefinition() const
{
	return &_scissorRect;
}

//----------------------------------------------------------------------------------------
V2UInt32 VulkanFrameBuffer::GetFrameBufferSizeInPixels() const
{
	return _framebufferSize;
}

//----------------------------------------------------------------------------------------
int VulkanFrameBuffer::GetViewportLastUpdateToken() const
{
	return _viewportLastUpdateToken;
}

//----------------------------------------------------------------------------------------
void VulkanFrameBuffer::UpdateViewport()
{
	// Setup the viewport. Note that we use a negative value in the height field to flip the viewport, giving us an
	// origin in the bottom left. This requires the "VK_KHR_maintenance1" extension.
	_viewport.width = (float)_drawState.viewportRegionSize.X();
	_viewport.height = -(float)_drawState.viewportRegionSize.Y();
	_viewport.minDepth = 0.0f;
	_viewport.maxDepth = 1.0f;
	_viewport.x = (float)_drawState.viewportRegionStartPos.X();
	_viewport.y = (float)_drawState.viewportRegionStartPos.Y() + (float)_drawState.viewportRegionSize.Y();

	// Setup the scissor rectangle
	if (_drawState.scissorRegionDefined)
	{
		_scissorRect.offset.x = _drawState.scissorRegionStartPos.X();
		_scissorRect.offset.y = _drawState.scissorRegionStartPos.Y();
		_scissorRect.extent.width = _drawState.scissorRegionSize.X();
		_scissorRect.extent.height = _drawState.scissorRegionSize.Y();
	}
	else
	{
		_scissorRect.offset.x = _drawState.viewportRegionStartPos.X();
		_scissorRect.offset.y = _drawState.viewportRegionStartPos.Y();
		_scissorRect.extent.width = _drawState.viewportRegionSize.X();
		_scissorRect.extent.height = _drawState.viewportRegionSize.Y();
	}

	// Increment the viewport last update token
	if (++_viewportLastUpdateToken > 100000)
	{
		_viewportLastUpdateToken = 1;
	}
}

//----------------------------------------------------------------------------------------
// Output capture methods
//----------------------------------------------------------------------------------------
bool VulkanFrameBuffer::HasCaptureTargets() const
{
	return !_drawState.captureTargets.empty();
}

//----------------------------------------------------------------------------------------
void VulkanFrameBuffer::AddOutputCaptureTarget(IFrameBufferOutput* captureTarget, AttachmentType type, size_t index)
{
	std::unique_lock<std::mutex> lock(_accessMutex);
	CaptureTargetInfo captureTargetInfo{};
	captureTargetInfo.captureTarget = KnownDynamicCast<VulkanFrameBufferOutput*>(captureTarget);
	captureTargetInfo.type = type;
	captureTargetInfo.index = index;
	_buildState.captureTargets.push_back(captureTargetInfo);
	lock.unlock();
	FlagBuildStateModified();
}

//----------------------------------------------------------------------------------------
void VulkanFrameBuffer::RemoveOutputCaptureTarget(IFrameBufferOutput* captureTarget)
{
	// Find and remove output capture target
	std::unique_lock<std::mutex> lock(_accessMutex);
	auto* captureTargetResolved = KnownDynamicCast<VulkanFrameBufferOutput*>(captureTarget);
	for (auto captureTargetsIterator = _buildState.captureTargets.begin(); captureTargetsIterator != _buildState.captureTargets.end(); ++captureTargetsIterator)
	{
		if (captureTargetsIterator->captureTarget == captureTargetResolved)
		{
			_buildState.captureTargets.erase(captureTargetsIterator);
			lock.unlock();
			FlagBuildStateModified();
			return;
		}
	}
	FlagBuildStateModified();
}

//----------------------------------------------------------------------------------------
void VulkanFrameBuffer::CaptureFrameBufferOutput(VkCommandBuffer commandBuffer)
{
	// Store our framebuffer output in any capture targets that have been bound
	for (auto& captureTargetInfo : _drawState.captureTargets)
	{
		// Attempt to locate the bound texture for the target framebuffer output
		VkImage srcImage = VK_NULL_HANDLE;
		VkFormat format{};
		VkImageLayout initialLayout{};
		uint32_t width{};
		uint32_t height{};
		ITextureBuffer::ImageFormat imageFormat{};
		ITextureBuffer::DataFormat dataFormat{};
		size_t elementCount{};
		size_t elementSizeInBytes{};
		size_t pixelOffsetInBytes{};
		size_t pixelStrideInBytes{};
		if (_drawState.boundToWindow)
		{
			if (captureTargetInfo.index == 0)
			{
				if (captureTargetInfo.type == IFrameBuffer::AttachmentType::Color)
				{
					srcImage = _swapchainImages[_swapchainImageIndex];
					format = _bestSurfaceFormat.format;
					initialLayout = (_drawState.headlessWindow ? VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL : VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
				}
				else if ((captureTargetInfo.type == IFrameBuffer::AttachmentType::Depth) || (captureTargetInfo.type == IFrameBuffer::AttachmentType::Stencil))
				{
					srcImage = _swapchainDepthImage;
					format = _depthStencilTextureFormat;
					initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
				}
				width = _surfaceExtent.width;
				height = _surfaceExtent.height;

				// Retrieve image information for the source buffer
				if (!GetFormatFromNativeFormat(format, imageFormat, dataFormat, elementCount, elementSizeInBytes, pixelOffsetInBytes, pixelStrideInBytes, captureTargetInfo.type == IFrameBuffer::AttachmentType::Stencil))
				{
					_log->Error("Failed to identify a compatible data format for framebuffer capture with native format {0}", format);
					continue;
				}
			}
		}
		else
		{
			for (const auto& boundTexture : _drawState.boundTextures)
			{
				if ((boundTexture.type == captureTargetInfo.type) && (boundTexture.index == captureTargetInfo.index))
				{
					srcImage = boundTexture.texture->GetImage();
					format = boundTexture.texture->GetNativeFormat();
					initialLayout = boundTexture.texture->GetDefaultImageLayout();
					auto textureDimensions = boundTexture.texture->MipmapLevelDimensions(0);
					width = textureDimensions.X();
					height = textureDimensions.Y();
					boundTexture.texture->GetTextureFormatInfo(imageFormat, dataFormat, elementCount, elementSizeInBytes, pixelOffsetInBytes, pixelStrideInBytes, (boundTexture.type == IFrameBuffer::AttachmentType::Stencil));
					break;
				}
			}
		}

		// If we couldn't locate the target framebuffer output, log an error, and advance to the next capture entry.
		if (srcImage == VK_NULL_HANDLE)
		{
			_log->Error("Failed to locate target framebuffer output for target type {0} and index {1}", captureTargetInfo.type, captureTargetInfo.index);
			continue;
		}

		// Attempt to capture the framebuffer output
		if (!CaptureAttachmentTarget(commandBuffer, captureTargetInfo, srcImage, format, initialLayout, width, height, imageFormat, dataFormat, elementCount, elementSizeInBytes, pixelOffsetInBytes, pixelStrideInBytes))
		{
			_log->Warning("Failed to capture framebuffer output");
			continue;
		}
	}
}

//----------------------------------------------------------------------------------------
bool VulkanFrameBuffer::CaptureAttachmentTarget(VkCommandBuffer commandBuffer, CaptureTargetInfo& captureTargetInfo, VkImage sourceImage, VkFormat format, VkImageLayout initialLayout, uint32_t imageWidth, uint32_t imageHeight, ITextureBuffer::ImageFormat imageFormat, ITextureBuffer::DataFormat dataFormat, size_t elementCount, size_t elementSizeInBytes, size_t pixelOffsetInBytes, size_t pixelStrideInBytes)
{
	// Retrieve the requested capture settings from our framebuffer output object
	VulkanFrameBufferOutput* captureTarget = captureTargetInfo.captureTarget;
	V2UInt32 requestedImageOffset = captureTarget->GetRequestedImageOffset();
	V2UInt32 requestedImageSize = captureTarget->GetRequestedImageSize();

	// Calculate the cropped size of the image, taking into account the requested image offset and size, if any.
	V2UInt32 croppedImageSize = VulkanFrameBufferOutput::CalculateCroppedImageDimensions(V2UInt32(imageWidth, imageHeight), requestedImageOffset, requestedImageSize);

	// Create a staging buffer to receive the data
	VulkanMemoryManager* memoryManager = _renderer->GetMemoryManager();
	size_t rowStrideInBytes = pixelStrideInBytes * croppedImageSize.X();
	size_t bufferSizeInBytes = rowStrideInBytes * croppedImageSize.Y();
	VkBuffer stagingBuffer;
	VmaAllocation stagingAllocation;
	if (!memoryManager->CreateBuffer(bufferSizeInBytes, VK_BUFFER_USAGE_TRANSFER_DST_BIT, VMA_MEMORY_USAGE_GPU_TO_CPU, 0, stagingBuffer, stagingAllocation))
	{
		_log->Error("Failed to create staging texture for framebuffer capture");
		return false;
	}

	// Determine the aspect flags to use based on the type of buffer being read
	VkImageAspectFlagBits aspectFlag = VK_IMAGE_ASPECT_COLOR_BIT;
	switch (captureTargetInfo.type)
	{
	case IFrameBuffer::AttachmentType::Color:
		aspectFlag = VK_IMAGE_ASPECT_COLOR_BIT;
		break;
	case IFrameBuffer::AttachmentType::Depth:
		aspectFlag = VK_IMAGE_ASPECT_DEPTH_BIT;
		break;
	case IFrameBuffer::AttachmentType::Stencil:
		aspectFlag = VK_IMAGE_ASPECT_STENCIL_BIT;
		break;
	}

	// Transition the source image to a layout where it can be read
	memoryManager->RecordTransitionImageLayout(commandBuffer, sourceImage, format, 1, 1, initialLayout, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

	// Copy the source buffer to the staging buffer
	memoryManager->RecordCopyImageToBuffer(commandBuffer, sourceImage, stagingBuffer, croppedImageSize.X(), croppedImageSize.Y(), requestedImageOffset.X(), requestedImageOffset.Y(), aspectFlag);

	// Restore the original source image layout
	memoryManager->RecordTransitionImageLayout(commandBuffer, sourceImage, format, 1, 1, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, initialLayout);

	// Store info on this framebuffer capture process for later processing
	captureTargetInfo.completionPending = true;
	captureTargetInfo.stagingBuffer = stagingBuffer;
	captureTargetInfo.stagingAllocation = stagingAllocation;
	captureTargetInfo.imageFormat = imageFormat;
	captureTargetInfo.dataFormat = dataFormat;
	captureTargetInfo.elementCount = elementCount;
	captureTargetInfo.elementSizeInBytes = elementSizeInBytes;
	captureTargetInfo.pixelOffsetInBytes = pixelOffsetInBytes;
	captureTargetInfo.pixelStrideInBytes = pixelStrideInBytes;
	captureTargetInfo.rowStrideInBytes = rowStrideInBytes;
	captureTargetInfo.bufferSizeInBytes = bufferSizeInBytes;
	captureTargetInfo.croppedImageSize = croppedImageSize;
	return true;
}

//----------------------------------------------------------------------------------------
void VulkanFrameBuffer::CompleteCaptureFrameBufferOutput()
{
	// Complete any pending framebuffer capture processes
	for (CaptureTargetInfo& captureTargetInfo : _drawState.captureTargets)
	{
		if (captureTargetInfo.completionPending)
		{
			captureTargetInfo.completionPending = false;
			if (!CompleteCaptureFrameBufferOutput(captureTargetInfo))
			{
				_log->Error("Failed to complete capture framebuffer output for target type {0} and index {1}", captureTargetInfo.type, captureTargetInfo.index);
				continue;
			}
		}
	}
}

//----------------------------------------------------------------------------------------
bool VulkanFrameBuffer::CompleteCaptureFrameBufferOutput(const CaptureTargetInfo& captureTargetInfo)
{
	// Map our staging texture into memory so we can access its data
	VulkanMemoryManager* memoryManager = _renderer->GetMemoryManager();
	uint8_t* sourceData;
	memoryManager->MapBufferMemory(captureTargetInfo.stagingAllocation, sourceData);

	// Write the texture data into our capture target
	VulkanFrameBufferOutput* captureTarget = captureTargetInfo.captureTarget;
	captureTarget->StoreCaptureData(captureTargetInfo.croppedImageSize, sourceData, captureTargetInfo.bufferSizeInBytes, captureTargetInfo.imageFormat, captureTargetInfo.dataFormat, captureTargetInfo.elementCount, captureTargetInfo.elementSizeInBytes, captureTargetInfo.pixelOffsetInBytes, captureTargetInfo.pixelStrideInBytes, captureTargetInfo.rowStrideInBytes, (captureTargetInfo.type == IFrameBuffer::AttachmentType::Stencil));

	// Unmap and destroy our staging texture
	memoryManager->UnmapBufferMemory(captureTargetInfo.stagingAllocation);
	memoryManager->DestroyBuffer(captureTargetInfo.stagingBuffer, captureTargetInfo.stagingAllocation);

	// Record this captured framebuffer output with the renderer
	_renderer->AddCurrentFrameBufferOutput(captureTarget);

	// Now that we've captured a frame, detach the output capture target if requested.
	if (captureTarget->IsDetachingAfterCapture())
	{
		RemoveOutputCaptureTarget(captureTarget);
	}
	return true;
}

//----------------------------------------------------------------------------------------
bool VulkanFrameBuffer::GetFormatFromNativeFormat(VkFormat nativeFormat, ITextureBuffer::ImageFormat& imageFormat, ITextureBuffer::DataFormat& dataFormat, size_t& elementCount, size_t& elementSizeInBytes, size_t& pixelOffsetInBytes, size_t& pixelStrideInBytes, bool stencilComponent)
{
	switch (nativeFormat)
	{
	case VK_FORMAT_B8G8R8A8_UNORM:
		imageFormat = ITextureBuffer::ImageFormat::BGRA;
		dataFormat = ITextureBuffer::DataFormat::UInt8;
		elementCount = ITextureBuffer::ElementCountPerPixelFromFormat(imageFormat);
		elementSizeInBytes = ITextureBuffer::ByteSizePerElementFromFormat(dataFormat);
		pixelStrideInBytes = elementSizeInBytes * elementCount;
		pixelOffsetInBytes = 0;
		return true;
	case VK_FORMAT_R8G8B8A8_UNORM:
		imageFormat = ITextureBuffer::ImageFormat::RGBA;
		dataFormat = ITextureBuffer::DataFormat::UInt8;
		elementCount = ITextureBuffer::ElementCountPerPixelFromFormat(imageFormat);
		elementSizeInBytes = ITextureBuffer::ByteSizePerElementFromFormat(dataFormat);
		pixelStrideInBytes = elementSizeInBytes * elementCount;
		pixelOffsetInBytes = 0;
		return true;
	case VK_FORMAT_R32G32B32_SFLOAT:
		imageFormat = ITextureBuffer::ImageFormat::RGB;
		dataFormat = ITextureBuffer::DataFormat::Float32;
		elementCount = ITextureBuffer::ElementCountPerPixelFromFormat(imageFormat);
		elementSizeInBytes = ITextureBuffer::ByteSizePerElementFromFormat(dataFormat);
		pixelStrideInBytes = elementSizeInBytes * elementCount;
		pixelOffsetInBytes = 0;
		return true;
	case VK_FORMAT_R32G32B32A32_SFLOAT:
		imageFormat = ITextureBuffer::ImageFormat::RGBA;
		dataFormat = ITextureBuffer::DataFormat::Float32;
		elementCount = ITextureBuffer::ElementCountPerPixelFromFormat(imageFormat);
		elementSizeInBytes = ITextureBuffer::ByteSizePerElementFromFormat(dataFormat);
		pixelStrideInBytes = elementSizeInBytes * elementCount;
		pixelOffsetInBytes = 0;
		return true;
	case VK_FORMAT_D16_UNORM:
		imageFormat = ITextureBuffer::ImageFormat::Depth;
		dataFormat = ITextureBuffer::DataFormat::DepthUNorm16;
		elementCount = ITextureBuffer::ElementCountPerPixelFromFormat(imageFormat);
		elementSizeInBytes = ITextureBuffer::ByteSizePerElementFromFormat(dataFormat);
		pixelStrideInBytes = elementSizeInBytes * elementCount;
		pixelOffsetInBytes = 0;
		return true;
	case VK_FORMAT_D24_UNORM_S8_UINT:
		imageFormat = ITextureBuffer::ImageFormat::DepthAndStencil;
		dataFormat = ITextureBuffer::DataFormat::DepthUNorm24StencilUInt8;
		elementCount = 1;
		// See Chapter 39 "Copy Commands" in the Vulkan spec for info on format rules when accessing depth/stencil
		// components from a combined format:
		// https://docs.vulkan.org/spec/latest/chapters/copies.html
		// Copies between a buffer and the depth or stencil aspect of an image use separate aspect formats rather than the
		// base interleaved image format, so we apply overrides here to set the stride, offset and size correctly.
		if (!stencilComponent)
		{
			// For D24/S8, the depth aspect is treated as VK_FORMAT_X8_D24_UNORM_PACK32
			elementSizeInBytes = 3;
			pixelStrideInBytes = 4;
			pixelOffsetInBytes = 0;
		}
		else
		{
			// For D24/S8, the stencil aspect is treated as VK_FORMAT_S8_UINT.
			elementSizeInBytes = 1;
			pixelStrideInBytes = 1;
			pixelOffsetInBytes = 0;
		}
		return true;
	case VK_FORMAT_D32_SFLOAT:
		imageFormat = ITextureBuffer::ImageFormat::Depth;
		dataFormat = ITextureBuffer::DataFormat::Float32;
		elementCount = ITextureBuffer::ElementCountPerPixelFromFormat(imageFormat);
		elementSizeInBytes = ITextureBuffer::ByteSizePerElementFromFormat(dataFormat);
		pixelStrideInBytes = elementSizeInBytes * elementCount;
		pixelOffsetInBytes = 0;
		return true;
	case VK_FORMAT_D32_SFLOAT_S8_UINT:
		imageFormat = ITextureBuffer::ImageFormat::DepthAndStencil;
		dataFormat = ITextureBuffer::DataFormat::DepthFloat32StencilUInt8;
		elementCount = 1;
		// See Chapter 39 "Copy Commands" in the Vulkan spec for info on format rules when accessing depth/stencil
		// components from a combined format:
		// https://docs.vulkan.org/spec/latest/chapters/copies.html
		// Copies between a buffer and the depth or stencil aspect of an image use separate aspect formats rather than the
		// base interleaved image format, so we apply overrides here to set the stride, offset and size correctly.
		if (!stencilComponent)
		{
			// For D32/S8, the depth aspect is treated as VK_FORMAT_D32_SFLOAT.
			elementSizeInBytes = 4;
			pixelStrideInBytes = 4;
			pixelOffsetInBytes = 0;
		}
		else
		{
			// For D32/S8, the stencil aspect is treated as VK_FORMAT_S8_UINT.
			elementSizeInBytes = 1;
			pixelStrideInBytes = 1;
			pixelOffsetInBytes = 0;
		}
		return true;
	}
	return false;
}

//----------------------------------------------------------------------------------------
// Framebuffer update methods
//----------------------------------------------------------------------------------------
bool VulkanFrameBuffer::CreateWindowObjects(VkCommandBuffer commandBuffer)
{
	if (_drawState.headlessWindow)
	{
		// Headless window targets use the same framebuffer/render pass plumbing as real window targets, but they don't
		// have a native surface or swapchain. Create renderer-owned images with the same default window formats instead.
		VkColorSpaceKHR colorSpaceMode = {};
		GetWindowColorFormat(_drawState.windowColorSpaceMode, _bestSurfaceFormat.format, colorSpaceMode);
		_bestSurfaceFormat.colorSpace = colorSpaceMode;

		// Select the optional depth/stencil format up front so the image and render pass attachment descriptions stay
		// aligned with the normal swapchain-backed window path.
		_depthStencilTextureFormat = VK_FORMAT_UNDEFINED;
		bool depthStencilTextureHasStencilComponent = false;
		bool hasDepthTexture = (_drawState.windowDepthStencilMode != WindowDepthStencilMode::None);
		if (hasDepthTexture)
		{
			GetWindowDepthStencilFormat(_drawState.windowDepthStencilMode, _renderer->GetPhysicalDevice(), _depthStencilTextureFormat, depthStencilTextureHasStencilComponent);
		}

		_surfaceExtent.width = _drawState.windowSizeInPixels.X();
		_surfaceExtent.height = _drawState.windowSizeInPixels.Y();
		VulkanMemoryManager* memoryManager = _renderer->GetMemoryManager();

		// Create the color attachment as an ordinary 2D image. It needs transfer source usage because framebuffer
		// output capture reads window targets through the same copy path used for swapchain images.
		VkImageCreateInfo colorImageCreateInfo = {};
		colorImageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		colorImageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
		colorImageCreateInfo.extent.width = _surfaceExtent.width;
		colorImageCreateInfo.extent.height = _surfaceExtent.height;
		colorImageCreateInfo.extent.depth = 1;
		colorImageCreateInfo.mipLevels = 1;
		colorImageCreateInfo.arrayLayers = 1;
		colorImageCreateInfo.format = _bestSurfaceFormat.format;
		colorImageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		colorImageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		colorImageCreateInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
		colorImageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		colorImageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		colorImageCreateInfo.flags = 0;

		VmaAllocationCreateInfo colorAllocationInfo = {};
		colorAllocationInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
		VkResult createHeadlessColorBufferResult = vmaCreateImage(memoryManager->Allocator(), &colorImageCreateInfo, &colorAllocationInfo, &_headlessColorImage, &_headlessColorAllocation, nullptr);
		if (createHeadlessColorBufferResult != VK_SUCCESS)
		{
			_log->Error("Failed to create headless color buffer with error code {0}", createHeadlessColorBufferResult);
			return false;
		}
		// Headless images are not acquired from a presentation engine, so transition them directly into the layout used
		// by the render pass instead of starting from VK_IMAGE_LAYOUT_PRESENT_SRC_KHR.
		memoryManager->RecordTransitionImageLayout(commandBuffer, _headlessColorImage, _bestSurfaceFormat.format, 1, 1, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

		// Store the color image view in the swapchain image-view array below. The rest of the framebuffer code only
		// needs a current image/view pair, and does not need to know whether it came from a real swapchain.
		VkImageView headlessColorImageView = VK_NULL_HANDLE;
		VkImageViewCreateInfo colorViewInfo = {};
		colorViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		colorViewInfo.image = _headlessColorImage;
		colorViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		colorViewInfo.format = _bestSurfaceFormat.format;
		colorViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		colorViewInfo.subresourceRange.baseMipLevel = 0;
		colorViewInfo.subresourceRange.levelCount = 1;
		colorViewInfo.subresourceRange.baseArrayLayer = 0;
		colorViewInfo.subresourceRange.layerCount = 1;
		VkResult createHeadlessColorViewResult = vkCreateImageView(_renderer->GetDevice(), &colorViewInfo, nullptr, &headlessColorImageView);
		if (createHeadlessColorViewResult != VK_SUCCESS)
		{
			_log->Error("Could not create image view for headless color buffer with error code {0}", createHeadlessColorViewResult);
			return false;
		}

		if (hasDepthTexture)
		{
			// Create a matching renderer-owned depth/stencil image for window depth testing and framebuffer capture.
			VkImageCreateInfo imageCreateInfo = {};
			imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
			imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
			imageCreateInfo.extent.width = _surfaceExtent.width;
			imageCreateInfo.extent.height = _surfaceExtent.height;
			imageCreateInfo.extent.depth = 1;
			imageCreateInfo.mipLevels = 1;
			imageCreateInfo.arrayLayers = 1;
			imageCreateInfo.format = _depthStencilTextureFormat;
			imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
			imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			imageCreateInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
			imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
			imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
			imageCreateInfo.flags = 0;

			VmaAllocationCreateInfo allocInfo = {};
			allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
			VkResult createDepthBufferResult = vmaCreateImage(memoryManager->Allocator(), &imageCreateInfo, &allocInfo, &_swapchainDepthImage, &_swapchainDepthAllocation, nullptr);
			if (createDepthBufferResult != VK_SUCCESS)
			{
				_log->Error("Failed to create depth buffer with error code {0}", createDepthBufferResult);
				return false;
			}
			memoryManager->RecordTransitionImageLayout(commandBuffer, _swapchainDepthImage, _depthStencilTextureFormat, 1, 1, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

			VkImageViewCreateInfo viewInfo = {};
			viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			viewInfo.image = _swapchainDepthImage;
			viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			viewInfo.format = _depthStencilTextureFormat;
			viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | (depthStencilTextureHasStencilComponent ? VK_IMAGE_ASPECT_STENCIL_BIT : 0);
			viewInfo.subresourceRange.baseMipLevel = 0;
			viewInfo.subresourceRange.levelCount = 1;
			viewInfo.subresourceRange.baseArrayLayer = 0;
			viewInfo.subresourceRange.layerCount = 1;
			VkResult createDepthViewResult = vkCreateImageView(_renderer->GetDevice(), &viewInfo, nullptr, &_swapchainDepthImageView);
			if (createDepthViewResult != VK_SUCCESS)
			{
				_log->Error("Could not create image view for depth buffer with error code {0}", createDepthViewResult);
				return false;
			}
		}

		// Describe attachments exactly as the headless images are left after creation. Render pass nodes will override
		// load/store operations for each pass, but the template render pass still needs compatible formats and layouts.
		_renderPassAttachments.clear();
		_renderPassColorAttachmentReferences.clear();
		_renderPassDepthStencilAttachmentReferences.clear();

		VkAttachmentDescription colorAttachment = {};
		colorAttachment.format = _bestSurfaceFormat.format;
		colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		colorAttachment.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		_renderPassAttachments.push_back(colorAttachment);

		VkAttachmentReference colorAttachmentReference = {};
		colorAttachmentReference.attachment = 0;
		colorAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		_renderPassColorAttachmentReferences.push_back(colorAttachmentReference);

		if (hasDepthTexture)
		{
			VkAttachmentDescription depthAttachment = {};
			depthAttachment.format = _depthStencilTextureFormat;
			depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
			depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			depthAttachment.initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
			depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
			_renderPassAttachments.push_back(depthAttachment);

			VkAttachmentReference depthAttachmentReference = {};
			depthAttachmentReference.attachment = 1;
			depthAttachmentReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
			_renderPassDepthStencilAttachmentReferences.push_back(depthAttachmentReference);
		}

		_framebufferSize = _drawState.windowSizeInPixels;
		_sampleCount = ITextureBuffer::SampleCount::SampleCount1;
		_sampleCountNative = VulkanTextureBuffer2D::GetNativeSampleCountFromSampleCount(_sampleCount);

		// Create a single framebuffer and present it through the existing window-target member arrays. This keeps later
		// binding and capture code shared with swapchain-backed framebuffers while avoiding any presentation calls.
		if (!CreateTemplateRenderPass(commandBuffer))
		{
			return false;
		}

		_swapchainImages = {_headlessColorImage};
		_swapchainImageViews = {headlessColorImageView};
		_swapchainFramebuffers.resize(1);
		_swapchainImageIndex = 0;

		VkImageView viewAttachments[2];
		viewAttachments[0] = _swapchainImageViews[0];
		viewAttachments[1] = _swapchainDepthImageView;
		VkFramebufferCreateInfo frameBufferCreateInfo = {};
		frameBufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		frameBufferCreateInfo.renderPass = _templateRenderPass;
		frameBufferCreateInfo.attachmentCount = (hasDepthTexture ? 2 : 1);
		frameBufferCreateInfo.pAttachments = &viewAttachments[0];
		frameBufferCreateInfo.width = _surfaceExtent.width;
		frameBufferCreateInfo.height = _surfaceExtent.height;
		frameBufferCreateInfo.layers = 1;
		VkResult createHeadlessFramebufferResult = vkCreateFramebuffer(_renderer->GetDevice(), &frameBufferCreateInfo, nullptr, _swapchainFramebuffers.data());
		if (createHeadlessFramebufferResult != VK_SUCCESS)
		{
			_log->Error("Could not create vulkan framebuffer object with error code {0}", createHeadlessFramebufferResult);
			return false;
		}
		return true;
	}

	// Create and configure the window surface
	CreateSurface(_drawState.windowColorSpaceMode);

	// Select the presentation mode to use. Note that only VK_PRESENT_MODE_FIFO_KHR is required to be supported.
	VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR;
	bool presentModeMailboxSupported = false;
	bool presentModeImmediateSupported = false;
	for (auto& surfacePresentMode : _surfacePresentModes)
	{
		presentModeMailboxSupported |= (surfacePresentMode == VK_PRESENT_MODE_MAILBOX_KHR);
		presentModeImmediateSupported |= (surfacePresentMode == VK_PRESENT_MODE_IMMEDIATE_KHR);
	}
	if (presentModeImmediateSupported && ((_drawState.windowBindingFlags & WindowBindingFlags::LimitSwapToVSync) == WindowBindingFlags::None) && ((_drawState.windowBindingFlags & WindowBindingFlags::AllowTearing) != WindowBindingFlags::None))
	{
		// Immediate mode doesn't limit to vsync and allows tearing
		presentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
	}
	else if (presentModeMailboxSupported && ((_drawState.windowBindingFlags & WindowBindingFlags::LimitSwapToVSync) == WindowBindingFlags::None))
	{
		// Mailbox mode doesn't limit to vsync but doesn't allow tearing
		presentMode = VK_PRESENT_MODE_MAILBOX_KHR;
	}
	else
	{
		// Fifo mode limits to vsync without tearing
		presentMode = VK_PRESENT_MODE_FIFO_KHR;
	}

	// Select swap chain extent (resolution). Note that we can't rely on _surfaceCapabilities.currentExtent, as it can
	// be 0xFFFFFFFF x 0xFFFFFFFF, on Weyland in particular. See the following:
	// https://github.com/KhronosGroup/Vulkan-Samples/pull/1338
	// Instead, we rely on being notified of window size changes by the application, and use the supplied size. This is
	// also a hard requirement on OpenGL under Wayland and AppKit, so it's not just for this issue we rely on that.
	VkExtent2D extent;
	extent.width = _drawState.windowSizeInPixels.X();
	extent.height = _drawState.windowSizeInPixels.Y();
	_surfaceExtent = extent;

	// Select the image count for the swap chain, targeting an ideal count of 2. Note that _surfaceCapabilities.maxImageCount
	// can be 0 to indicate "no limit.
	uint32_t targetImageCount = (_surfaceCapabilities.maxImageCount == 1) ? 1u : 2u;
	uint32_t imageCount = std::max(targetImageCount, _surfaceCapabilities.minImageCount);

	// Build the swap chain create info structure
	VkSwapchainCreateInfoKHR swapchainInfo = {};
	swapchainInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapchainInfo.surface = _surface;
	swapchainInfo.minImageCount = imageCount;
	swapchainInfo.imageFormat = _bestSurfaceFormat.format;
	swapchainInfo.imageColorSpace = _bestSurfaceFormat.colorSpace;
	swapchainInfo.imageExtent = _surfaceExtent;
	swapchainInfo.imageArrayLayers = 1;
	swapchainInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
	uint32_t queueIndices[] = {_renderer->GetPresentQueueFamily(), _renderer->GetGraphicsQueueFamily()};
	if (queueIndices[0] != queueIndices[1])
	{
		// Different queues, need to share
		swapchainInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		swapchainInfo.pQueueFamilyIndices = &queueIndices[0];
		swapchainInfo.queueFamilyIndexCount = 2;
	}
	else
	{
		// Same queue, no need to share
		swapchainInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		swapchainInfo.pQueueFamilyIndices = &queueIndices[1];
		swapchainInfo.queueFamilyIndexCount = 1;
	}
	swapchainInfo.preTransform = _surfaceCapabilities.currentTransform;
	swapchainInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	swapchainInfo.presentMode = presentMode;
	swapchainInfo.clipped = VK_TRUE;
	swapchainInfo.oldSwapchain = VK_NULL_HANDLE;

	// Create the swapchain
	VkResult vkCreateSwapchainReturn = vkCreateSwapchainKHR(_renderer->GetDevice(), &swapchainInfo, nullptr, &_swapchain);
	if (vkCreateSwapchainReturn == VK_ERROR_SURFACE_LOST_KHR)
	{
		_log->Error("Could not create swapchain, surface was lost");
		return false;
	}
	if (vkCreateSwapchainReturn != VK_SUCCESS)
	{
		_log->Error("Could not create swapchain for frame buffer with error code {0}", vkCreateSwapchainReturn);
		return false;
	}

	// Select the depth/stencil format to use for the window framebuffer
	_depthStencilTextureFormat = VK_FORMAT_UNDEFINED;
	bool depthStencilTextureHasStencilComponent = false;
	bool hasDepthTexture = (_drawState.windowDepthStencilMode != WindowDepthStencilMode::None);
	if (hasDepthTexture)
	{
		GetWindowDepthStencilFormat(_drawState.windowDepthStencilMode, _renderer->GetPhysicalDevice(), _depthStencilTextureFormat, depthStencilTextureHasStencilComponent);
	}

	// Create a depth/stencil texture if required
	VulkanMemoryManager* memoryManager = _renderer->GetMemoryManager();
	if (hasDepthTexture)
	{
		// Populate the VkImageCreateInfo structure for the depth/stencil buffer
		VkImageCreateInfo imageCreateInfo = {};
		imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
		imageCreateInfo.extent.width = (uint32_t)_drawState.windowSizeInPixels.X();
		imageCreateInfo.extent.height = (uint32_t)_drawState.windowSizeInPixels.Y();
		imageCreateInfo.extent.depth = 1;
		imageCreateInfo.mipLevels = 1;
		imageCreateInfo.arrayLayers = 1;
		imageCreateInfo.format = _depthStencilTextureFormat;
		imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imageCreateInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
		imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		imageCreateInfo.flags = 0;

		// Create the depth/stencil buffer
		VmaAllocationCreateInfo allocInfo = {};
		allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
		VkResult createDepthBufferResult = vmaCreateImage(memoryManager->Allocator(), &imageCreateInfo, &allocInfo, &_swapchainDepthImage, &_swapchainDepthAllocation, nullptr);
		if (createDepthBufferResult != VK_SUCCESS)
		{
			_log->Error("Failed to create depth buffer with error code {0}", createDepthBufferResult);
			return false;
		}

		// Transition the depth/stencil buffer into its initial layout state
		memoryManager->RecordTransitionImageLayout(commandBuffer, _swapchainDepthImage, _depthStencilTextureFormat, 1, 1, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

		// Create an image view for the depth/stencil texture
		VkImageViewCreateInfo viewInfo = {};
		viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewInfo.image = _swapchainDepthImage;
		viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		viewInfo.format = _depthStencilTextureFormat;
		viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | (depthStencilTextureHasStencilComponent ? VK_IMAGE_ASPECT_STENCIL_BIT : 0);
		viewInfo.subresourceRange.baseMipLevel = 0;
		viewInfo.subresourceRange.levelCount = 1;
		viewInfo.subresourceRange.baseArrayLayer = 0;
		viewInfo.subresourceRange.layerCount = 1;
		VkResult createDepthViewResult = vkCreateImageView(_renderer->GetDevice(), &viewInfo, nullptr, &_swapchainDepthImageView);
		if (createDepthViewResult != VK_SUCCESS)
		{
			_log->Error("Could not create image view for depth buffer with error code {0}", createDepthViewResult);
			return false;
		}
	}

	// Clear the render pass attachment information in preparation for regenerating it below
	_renderPassAttachments.clear();
	_renderPassColorAttachmentReferences.clear();
	_renderPassDepthStencilAttachmentReferences.clear();

	// Create the color attachment
	VkAttachmentDescription colorAttachment = {};
	colorAttachment.format = _bestSurfaceFormat.format;
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	_renderPassAttachments.push_back(colorAttachment);

	// Create the color attachment reference
	VkAttachmentReference colorAttachmentReference = {};
	colorAttachmentReference.attachment = 0;
	colorAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	_renderPassColorAttachmentReferences.push_back(colorAttachmentReference);

	// Create the depth/stencil attachment and reference if required
	if (hasDepthTexture)
	{
		// Create the depth/stencil attachment
		VkAttachmentDescription depthAttachment = {};
		depthAttachment.format = _depthStencilTextureFormat;
		depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depthAttachment.initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		_renderPassAttachments.push_back(depthAttachment);

		// Create the depth/stencil attachment reference
		VkAttachmentReference depthAttachmentReference = {};
		depthAttachmentReference.attachment = 1;
		depthAttachmentReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		_renderPassDepthStencilAttachmentReferences.push_back(depthAttachmentReference);
	}

	// Record the framebuffer size and sample count and
	_framebufferSize = _drawState.windowSizeInPixels;
	_sampleCount = ITextureBuffer::SampleCount::SampleCount1;
	_sampleCountNative = VulkanTextureBuffer2D::GetNativeSampleCountFromSampleCount(_sampleCount);

	// Create our template render pass object
	if (!CreateTemplateRenderPass(commandBuffer))
	{
		return false;
	}

	// Retrieve our swap chain images
	uint32_t swapchainImageCount = 0;
	vkGetSwapchainImagesKHR(_renderer->GetDevice(), _swapchain, &swapchainImageCount, nullptr);
	_swapchainImages.resize(swapchainImageCount);
	vkGetSwapchainImagesKHR(_renderer->GetDevice(), _swapchain, &swapchainImageCount, _swapchainImages.data());

	// Create image views and framebuffers for each swapchain image
	_swapchainImageViews.resize(_swapchainImages.size());
	_swapchainFramebuffers.resize(_swapchainImages.size());
	for (size_t swapchainImageIndex = 0; swapchainImageIndex < _swapchainImages.size(); ++swapchainImageIndex)
	{
		// Transition the swapchain image to presentation mode
		const auto& swapchainImage = _swapchainImages[swapchainImageIndex];
		memoryManager->RecordTransitionImageLayout(commandBuffer, swapchainImage, _bestSurfaceFormat.format, 1, 1, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

		// Create an image view for this swapchain image
		auto& swapchainImageView = _swapchainImageViews[swapchainImageIndex];
		VkImageViewCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		createInfo.image = swapchainImage;
		createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		createInfo.format = _bestSurfaceFormat.format;
		createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		createInfo.subresourceRange.baseMipLevel = 0;
		createInfo.subresourceRange.levelCount = 1;
		createInfo.subresourceRange.baseArrayLayer = 0;
		createInfo.subresourceRange.layerCount = 1;
		VkResult createSwapchainImageViewResult = vkCreateImageView(_renderer->GetDevice(), &createInfo, nullptr, &swapchainImageView);
		if (createSwapchainImageViewResult != VK_SUCCESS)
		{
			_log->Error("Could not create swapchain image views with error code {0}", createSwapchainImageViewResult);
			return false;
		}

		// Create a framebuffer for this swapchain image
		auto& swapchainFramebuffer = _swapchainFramebuffers[swapchainImageIndex];
		VkImageView viewAttachments[2];
		viewAttachments[0] = swapchainImageView;
		viewAttachments[1] = _swapchainDepthImageView;
		VkFramebufferCreateInfo frameBufferCreateInfo = {};
		frameBufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		frameBufferCreateInfo.renderPass = _templateRenderPass;
		frameBufferCreateInfo.attachmentCount = (hasDepthTexture ? 2 : 1);
		frameBufferCreateInfo.pAttachments = &viewAttachments[0];
		frameBufferCreateInfo.width = (uint32_t)_drawState.windowSizeInPixels.X();
		frameBufferCreateInfo.height = (uint32_t)_drawState.windowSizeInPixels.Y();
		frameBufferCreateInfo.layers = 1;
		VkResult createSwapchainFramebufferResult = vkCreateFramebuffer(_renderer->GetDevice(), &frameBufferCreateInfo, nullptr, &swapchainFramebuffer);
		if (createSwapchainFramebufferResult != VK_SUCCESS)
		{
			_log->Error("Could not create vulkan framebuffer object with error code {0}", createSwapchainFramebufferResult);
			return false;
		}
	}

	// Pre-build our present info for when we need to swap our buffers
	_presentInfo = {};
	_presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	_presentInfo.waitSemaphoreCount = 0;
	_presentInfo.swapchainCount = 1;
	_presentInfo.pSwapchains = &_swapchain;
	_presentInfo.pImageIndices = &_swapchainImageIndex;
	_presentInfo.pResults = nullptr;
	_presentInfo.waitSemaphoreCount = 1;
	_presentInfo.pWaitSemaphores = &_presentSemaphore;
	return true;
}

//----------------------------------------------------------------------------------------
bool VulkanFrameBuffer::CreateTextureObjects(VkCommandBuffer commandBuffer)
{
	// Clear the render pass attachment information in preparation for regenerating it below
	_renderPassAttachments.clear();
	_renderPassColorAttachmentReferences.clear();
	_renderPassDepthStencilAttachmentReferences.clear();

	// Create our attachment descriptions and references for each bound texture, calculate the framebuffer size, sample
	// count, and build our set of image views.
	V2UInt32 framebufferSize = V2UInt32(0, 0);
	ITextureBuffer::SampleCount framebufferSampleCount = ITextureBuffer::SampleCount::SampleCount1;
	bool addedDepthStencilAttachment = false;
	int depthStencilAttachmentIndex = 0;
	std::vector<VkImageView> imageViews;
	for (auto& boundTextureInfo : _drawState.boundTextures)
	{
		// If this is either a depth or stencil attachment, and we've already added a depth/stencil attachment, skip it.
		// We need this check here, as on our API we require depth and stencil attachments to be bound separately, even
		// though they're a combined attachment in Vulkan.
		if ((boundTextureInfo.type != AttachmentType::Color) && addedDepthStencilAttachment)
		{
			boundTextureInfo.attachmentNo = depthStencilAttachmentIndex;
			continue;
		}

		// Obtain the size and sample count for this texture, and ensure it is compatible with other bound textures.
		auto* texture = boundTextureInfo.texture;
		auto textureDimensions = texture->MipmapLevelDimensions(0);
		auto textureSampleCount = texture->GetSampleCount();
		if (_renderPassAttachments.empty())
		{
			framebufferSize = textureDimensions;
			framebufferSampleCount = textureSampleCount;
		}
		else if (framebufferSampleCount != textureSampleCount)
		{
			_log->Error("Mismatched sample counts detected for framebuffer texture bindings. All framebuffer textures must share the same sample count to be combined into a framebuffer.");
			return false;
		}
		else if ((framebufferSize.X() != textureDimensions.X()) || (framebufferSize.Y() != textureDimensions.Y()))
		{
			_log->Error("Mismatched sizes detected for framebuffer texture bindings. All framebuffer textures must share the same size to be combined into a framebuffer.");
			return false;
		}

		// Create the attachment description
		auto attachmentIndex = (uint32_t)_renderPassAttachments.size();
		VkAttachmentDescription attachmentDescription = {};
		attachmentDescription.format = texture->GetNativeFormat();
		attachmentDescription.samples = VulkanTextureBuffer2D::GetNativeSampleCountFromSampleCount(texture->GetSampleCount());
		attachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachmentDescription.initialLayout = texture->GetDefaultImageLayout();
		attachmentDescription.finalLayout = texture->GetDefaultImageLayout();
		_renderPassAttachments.push_back(attachmentDescription);

		// Create the attachment reference
		if (boundTextureInfo.type == AttachmentType::Color)
		{
			boundTextureInfo.colorAttachmentIndex = (int)_renderPassColorAttachmentReferences.size();
			VkAttachmentReference colorAttachmentReference = {};
			colorAttachmentReference.attachment = attachmentIndex;
			colorAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			_renderPassColorAttachmentReferences.push_back(colorAttachmentReference);
		}
		else
		{
			boundTextureInfo.colorAttachmentIndex = 0;
			VkAttachmentReference depthStencilAttachmentReference = {};
			depthStencilAttachmentReference.attachment = attachmentIndex;
			depthStencilAttachmentReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
			_renderPassDepthStencilAttachmentReferences.push_back(depthStencilAttachmentReference);
		}

		// Record the attachment index for this texture
		boundTextureInfo.attachmentNo = attachmentIndex;
		if (boundTextureInfo.type != AttachmentType::Color)
		{
			addedDepthStencilAttachment = true;
			depthStencilAttachmentIndex = attachmentIndex;
		}

		// Add the image view for this texture to the set of image views
		imageViews.push_back(boundTextureInfo.texture->GetImageView());
	}
	for (auto& boundTextureInfo : _drawState.resolveTextures)
	{
		// Create the attachment description
		auto attachmentIndex = (int)_renderPassAttachments.size();
		auto* texture = boundTextureInfo.texture;
		VkAttachmentDescription attachmentDescription = {};
		attachmentDescription.format = texture->GetNativeFormat();
		attachmentDescription.samples = VulkanTextureBuffer2D::GetNativeSampleCountFromSampleCount(ITextureBuffer::SampleCount::SampleCount1);
		attachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachmentDescription.initialLayout = texture->GetDefaultImageLayout();
		attachmentDescription.finalLayout = texture->GetDefaultImageLayout();
		_renderPassAttachments.push_back(attachmentDescription);

		// Record the attachment index for this resolve texture
		boundTextureInfo.attachmentNo = attachmentIndex;

		// Add the image view for this texture to the set of image views
		imageViews.push_back(boundTextureInfo.texture->GetImageView());
	}

	// Record the calculated framebuffer size and sample count
	_framebufferSize = framebufferSize;
	_sampleCount = framebufferSampleCount;
	_sampleCountNative = VulkanTextureBuffer2D::GetNativeSampleCountFromSampleCount(_sampleCount);

	// Create our template render pass object
	if (!CreateTemplateRenderPass(commandBuffer))
	{
		return false;
	}

	// Create our framebuffer object
	VkFramebufferCreateInfo frameBufferInfo = {};
	frameBufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	frameBufferInfo.renderPass = _templateRenderPass;
	frameBufferInfo.attachmentCount = (uint32_t)imageViews.size();
	frameBufferInfo.pAttachments = imageViews.data();
	frameBufferInfo.width = framebufferSize.X();
	frameBufferInfo.height = framebufferSize.Y();
	frameBufferInfo.layers = 1;
	VkResult createTextureFramebufferResult = vkCreateFramebuffer(_renderer->GetDevice(), &frameBufferInfo, nullptr, &_textureFramebuffer);
	if (createTextureFramebufferResult != VK_SUCCESS)
	{
		_log->Error("Could not create vulkan framebuffer object with error code {0}", createTextureFramebufferResult);
		return false;
	}
	return true;
}

//----------------------------------------------------------------------------------------
bool VulkanFrameBuffer::CreateTemplateRenderPass(VkCommandBuffer commandBuffer)
{
	// Create the subpass description
	VkSubpassDescription subpassDescription = {};
	subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpassDescription.colorAttachmentCount = (uint32_t)_renderPassColorAttachmentReferences.size();
	subpassDescription.pColorAttachments = (!_renderPassColorAttachmentReferences.empty() ? _renderPassColorAttachmentReferences.data() : nullptr);
	subpassDescription.pDepthStencilAttachment = (!_renderPassDepthStencilAttachmentReferences.empty() ? _renderPassDepthStencilAttachmentReferences.data() : nullptr);

	// Define external dependencies matching the render pass nodes which will be used with this framebuffer. This is a
	// template render pass for framebuffer creation only, but keeping the dependency structure aligned keeps the Vulkan
	// objects conservative and avoids relying on implementation-specific implicit synchronization.
	std::array<VkSubpassDependency, 2> subpassDependencies = {};
	subpassDependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
	subpassDependencies[0].dstSubpass = 0;
	subpassDependencies[0].srcStageMask = VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_TRANSFER_BIT;
	subpassDependencies[0].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT;
	subpassDependencies[0].dstStageMask = VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT;
	subpassDependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	subpassDependencies[1].srcSubpass = 0;
	subpassDependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
	subpassDependencies[1].srcStageMask = VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT;
	subpassDependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	subpassDependencies[1].dstStageMask = VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_TRANSFER_BIT;
	subpassDependencies[1].dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

	// Create the template render pass object to define our framebuffer. Note that as per Vulcan spec 8.2 "Render Pass
	// Compatibility", although we have to define a render pass in order to create our framebuffer, we don't need to use
	// it with the framebuffer once it's created. We can instead use another render pass as long as it's compatible. If
	// we have a single subpass like we do here, we can define all the per-render pass information such as load/store
	// operations, clear operations, and multi-sample texture resolve operations, on the render pass nodes themselves,
	// and vary it between different render passes against this same framebuffer as required.
	VkRenderPassCreateInfo renderPassCreateInfo = {};
	renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassCreateInfo.attachmentCount = (uint32_t)_renderPassAttachments.size();
	renderPassCreateInfo.pAttachments = (!_renderPassAttachments.empty() ? _renderPassAttachments.data() : nullptr);
	renderPassCreateInfo.subpassCount = 1;
	renderPassCreateInfo.pSubpasses = &subpassDescription;
	renderPassCreateInfo.dependencyCount = (uint32_t)subpassDependencies.size();
	renderPassCreateInfo.pDependencies = subpassDependencies.data();
	VkResult createRenderPassResult = vkCreateRenderPass(_renderer->GetDevice(), &renderPassCreateInfo, nullptr, &_templateRenderPass);
	if (createRenderPassResult != VK_SUCCESS)
	{
		_log->Error("vkCreateRenderPass failed with error code {0}", createRenderPassResult);
		return false;
	}
	return true;
}

//----------------------------------------------------------------------------------------
bool VulkanFrameBuffer::CreateSurface(WindowColorSpaceMode windowColorSpaceMode)
{
	// Create the surface
	bool createdSurface = false;
#ifdef COBALT_RENDERER_WIN32_SUPPORT
	if (_drawState.windowType == IFrameBuffer::WindowInfoBase::WindowType::Win32)
	{
		// This is safe to call on our render thread, off the UI thread, as long as the UI thread is still pumping
		// messages.
		const auto* windowInfo = std::get_if<WindowInfoWin32>(&_drawState.windowInfo);
		VkWin32SurfaceCreateInfoKHR surfaceInfo = {};
		surfaceInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
		surfaceInfo.hwnd = windowInfo->windowHandle;
		surfaceInfo.hinstance = (windowInfo->instanceHandle != nullptr ? windowInfo->instanceHandle : GetModuleHandle(nullptr));
		VkResult createSurfaceResult = vkCreateWin32SurfaceKHR(_renderer->GetInstance(), &surfaceInfo, nullptr, &_surface);
		if (createSurfaceResult != VK_SUCCESS)
		{
			_log->Error("Could not create surface - vkCreateWin32SurfaceKHR failed with error code {0}", createSurfaceResult);
			return false;
		}
		createdSurface = true;
	}
#endif
#ifdef COBALT_RENDERER_XLIB_SUPPORT
	if (_drawState.windowType == IFrameBuffer::WindowInfoBase::WindowType::Xlib)
	{
		// This is safe to call on our render thread, off the UI thread, as long as XInitThreads() has been called by
		// the host application before X11 is initialized.
		const auto* windowInfo = std::get_if<WindowInfoXlib>(&_drawState.windowInfo);
		VkXlibSurfaceCreateInfoKHR surfaceInfo = {};
		surfaceInfo.sType = VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR;
		surfaceInfo.pNext = nullptr;
		surfaceInfo.flags = 0;
		surfaceInfo.dpy = windowInfo->display;
		surfaceInfo.window = windowInfo->window;
		VkResult createSurfaceResult = vkCreateXlibSurfaceKHR(_renderer->GetInstance(), &surfaceInfo, nullptr, &_surface);
		if (createSurfaceResult != VK_SUCCESS)
		{
			_log->Error("Could not create surface - vkCreateXlibSurfaceKHR failed with error code {0}", createSurfaceResult);
			return false;
		}
		createdSurface = true;
	}
#endif
#ifdef COBALT_RENDERER_XCB_SUPPORT
	if (_drawState.windowType == IFrameBuffer::WindowInfoBase::WindowType::XCB)
	{
		// This is safe to call on our render thread, off the UI thread.
		const auto* windowInfo = std::get_if<WindowInfoXCB>(&_drawState.windowInfo);
		VkXcbSurfaceCreateInfoKHR surfaceInfo = {};
		surfaceInfo.sType = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR;
		surfaceInfo.pNext = nullptr;
		surfaceInfo.flags = 0;
		surfaceInfo.connection = windowInfo->connection;
		surfaceInfo.window = windowInfo->window;
		VkResult createSurfaceResult = vkCreateXcbSurfaceKHR(_renderer->GetInstance(), &surfaceInfo, nullptr, &_surface);
		if (createSurfaceResult != VK_SUCCESS)
		{
			_log->Error("Could not create surface - vkCreateXcbSurfaceKHR failed with error code {0}", createSurfaceResult);
			return false;
		}
		createdSurface = true;
	}
#endif
#ifdef COBALT_RENDERER_WAYLAND_SUPPORT
	if (_drawState.windowType == IFrameBuffer::WindowInfoBase::WindowType::Wayland)
	{
		// Documentation is poor, but this is safe to call on our render thread, off the UI thread.
		const auto* windowInfo = std::get_if<WindowInfoWayland>(&_drawState.windowInfo);
		VkWaylandSurfaceCreateInfoKHR surfaceInfo = {};
		surfaceInfo.sType = VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR;
		surfaceInfo.pNext = nullptr;
		surfaceInfo.flags = 0;
		surfaceInfo.display = windowInfo->display;
		surfaceInfo.surface = windowInfo->surface;
		VkResult createSurfaceResult = vkCreateWaylandSurfaceKHR(_renderer->GetInstance(), &surfaceInfo, nullptr, &_surface);
		if (createSurfaceResult != VK_SUCCESS)
		{
			_log->Error("Could not create surface - vkCreateWaylandSurfaceKHR failed with error code {0}", createSurfaceResult);
			return false;
		}
		createdSurface = true;
	}
#endif
#ifdef COBALT_RENDERER_APPKIT_SUPPORT
	if (_drawState.windowType == IFrameBuffer::WindowInfoBase::WindowType::AppKit)
	{
		// This is safe to call on our render thread, off the UI thread.
		const auto* windowInfo = std::get_if<WindowInfoAppKit>(&_drawState.windowInfo);
		VkMetalSurfaceCreateInfoEXT surfaceInfo = {};
		surfaceInfo.sType = VK_STRUCTURE_TYPE_METAL_SURFACE_CREATE_INFO_EXT;
		surfaceInfo.pLayer = (CAMetalLayer*)((__bridge NSView*)windowInfo->view).layer;
		VkResult createSurfaceResult = vkCreateMetalSurfaceEXT(_renderer->GetInstance(), &surfaceInfo, nullptr, &_surface);
		if (createSurfaceResult != VK_SUCCESS)
		{
			_log->Error("Could not create surface - vkCreateMetalSurfaceEXT failed with error code {0}", createSurfaceResult);
			return false;
		}
		createdSurface = true;
	}
#endif
#ifdef COBALT_RENDERER_METALLAYER_SUPPORT
	if (_drawState.windowType == IFrameBuffer::WindowInfoBase::WindowType::MetalLayer)
	{
		// This is safe to call on our render thread, off the UI thread.
		const auto* windowInfo = std::get_if<WindowInfoMetalLayer>(&_drawState.windowInfo);
		VkMetalSurfaceCreateInfoEXT surfaceInfo = {};
		surfaceInfo.sType = VK_STRUCTURE_TYPE_METAL_SURFACE_CREATE_INFO_EXT;
		surfaceInfo.pLayer = windowInfo->metalLayer;
		VkResult createSurfaceResult = vkCreateMetalSurfaceEXT(_renderer->GetInstance(), &surfaceInfo, nullptr, &_surface);
		if (createSurfaceResult != VK_SUCCESS)
		{
			_log->Error("Could not create surface - vkCreateMetalSurfaceEXT failed with error code {0}", createSurfaceResult);
			return false;
		}
		createdSurface = true;
	}
#endif
	if (!createdSurface)
	{
		_log->Error("Could not create surface. No supported window information was provided.");
		return false;
	}

	// Select present queue
	VkPhysicalDevice physicalDevice = _renderer->GetPhysicalDevice();
	VkBool32 presentSupport = VK_FALSE;
	vkGetPhysicalDeviceSurfaceSupportKHR(_renderer->GetPhysicalDevice(), _renderer->GetPresentQueueFamily(), _surface, &presentSupport);
	if (presentSupport != VK_TRUE)
	{
		_log->Error("Selected presentation queue is not compatible with the created surface");
		return false;
	}
	_presentQueue = _renderer->GetPresentQueue();

	// Get surface present mode support details
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, _surface, &_surfaceCapabilities);
	uint32_t modeCount = 0;
	vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, _surface, &modeCount, nullptr);
	if (modeCount != 0)
	{
		_surfacePresentModes.resize(modeCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, _surface, &modeCount, _surfacePresentModes.data());
	}

	// Get surface format support details
	uint32_t formatCount = 0;
	vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, _surface, &formatCount, nullptr);
	std::vector<VkSurfaceFormatKHR> surfaceFormats;
	if (formatCount != 0)
	{
		surfaceFormats.resize(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, _surface, &formatCount, surfaceFormats.data());
	}

	// Select the colour format to use for the window framebuffer
	VkFormat colorBufferFormat;
	VkColorSpaceKHR colorSpaceMode;
	GetWindowColorFormat(_drawState.windowColorSpaceMode, colorBufferFormat, colorSpaceMode);

	// Select surface format. If we have one entry of VK_FORMAT_UNDEFINED we can select our preferred choice, otherwise
	// look for the best match.
	if ((surfaceFormats.size() != 1) || (surfaceFormats[0].format != VK_FORMAT_UNDEFINED))
	{
		// Look for preferred choice
		bool found = false;
		for (const auto& availableSurfaceFormat : surfaceFormats)
		{
			if ((availableSurfaceFormat.format == colorBufferFormat) && (availableSurfaceFormat.colorSpace == colorSpaceMode))
			{
				found = true;
				break;
			}
		}

		// Otherwise, default to first choice.
		if (!found)
		{
			_log->Info("Couldn't find preferred format choice, selecting default, format {0}, colorSpace {1}", surfaceFormats[0].format, surfaceFormats[0].colorSpace);
			colorBufferFormat = surfaceFormats[0].format;
			colorSpaceMode = surfaceFormats[0].colorSpace;
		}
	}
	_bestSurfaceFormat.format = colorBufferFormat;
	_bestSurfaceFormat.colorSpace = colorSpaceMode;
	return true;
}

//----------------------------------------------------------------------------------------
void VulkanFrameBuffer::DeleteWindowObjects()
{
	// Destroy the render pass
	VkDevice device = _renderer->GetDevice();
	vkDestroyRenderPass(device, _templateRenderPass, nullptr);
	_templateRenderPass = VK_NULL_HANDLE;

	// Destroy the framebuffers
	for (uint32_t i = 0; i < _swapchainImageViews.size(); ++i)
	{
		vkDestroyFramebuffer(device, _swapchainFramebuffers[i], nullptr);
		vkDestroyImageView(device, _swapchainImageViews[i], nullptr);
		_swapchainFramebuffers[i] = VK_NULL_HANDLE;
		_swapchainImageViews[i] = VK_NULL_HANDLE;
	}
	if (_swapchainDepthImageView != VK_NULL_HANDLE)
	{
		vkDestroyImageView(device, _swapchainDepthImageView, nullptr);
		_renderer->GetMemoryManager()->DestroyImage(_swapchainDepthImage, _swapchainDepthAllocation);
		_swapchainDepthImageView = VK_NULL_HANDLE;
	}
	_swapchainDepthImageView = VK_NULL_HANDLE;
	_swapchainDepthImage = VK_NULL_HANDLE;
	_swapchainDepthAllocation = VK_NULL_HANDLE;
	if (_headlessColorImage != VK_NULL_HANDLE)
	{
		_renderer->GetMemoryManager()->DestroyImage(_headlessColorImage, _headlessColorAllocation);
		_headlessColorImage = VK_NULL_HANDLE;
		_headlessColorAllocation = VK_NULL_HANDLE;
	}

	// Destroy the swap chain
	if (_swapchain != VK_NULL_HANDLE)
	{
		vkDestroySwapchainKHR(device, _swapchain, nullptr);
		_swapchain = VK_NULL_HANDLE;
	}

	// Destroy the surface
	if (_surface != VK_NULL_HANDLE)
	{
		vkDestroySurfaceKHR(_renderer->GetInstance(), _surface, nullptr);
		_surface = VK_NULL_HANDLE;
	}
}

//----------------------------------------------------------------------------------------
void VulkanFrameBuffer::DeleteTextureObjects()
{
	// Destroy the framebuffer
	VkDevice device = _renderer->GetDevice();
	vkDestroyFramebuffer(device, _textureFramebuffer, nullptr);
	_textureFramebuffer = VK_NULL_HANDLE;

	// Destroy the render pass
	vkDestroyRenderPass(device, _templateRenderPass, nullptr);
	_templateRenderPass = VK_NULL_HANDLE;
}

//----------------------------------------------------------------------------------------
void VulkanFrameBuffer::GetWindowColorFormat(WindowColorSpaceMode mode, VkFormat& format, VkColorSpaceKHR& colorSpace)
{
	format = VK_FORMAT_B8G8R8A8_UNORM;
	colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
}

//----------------------------------------------------------------------------------------
bool VulkanFrameBuffer::IsDepthStencilFormatSupported(VkPhysicalDevice physicalDevice, VkFormat format)
{
	VkFormatProperties formatProperties;
	vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &formatProperties);
	return ((formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) != 0);
}

//----------------------------------------------------------------------------------------
void VulkanFrameBuffer::GetWindowDepthStencilFormat(WindowDepthStencilMode mode, VkPhysicalDevice physicalDevice, VkFormat& format, bool& hasScencilComponent)
{
	switch (mode)
	{
	case IFrameBuffer::WindowDepthStencilMode::DepthUNorm16:
		format = VK_FORMAT_D16_UNORM;
		if (!IsDepthStencilFormatSupported(physicalDevice, format))
		{
			GetWindowDepthStencilFormat(IFrameBuffer::WindowDepthStencilMode::DepthUNorm24, physicalDevice, format, hasScencilComponent);
		}
		hasScencilComponent = false;
		return;
	case IFrameBuffer::WindowDepthStencilMode::DepthUNorm24:
		format = VK_FORMAT_D24_UNORM_S8_UINT;
		if (!IsDepthStencilFormatSupported(physicalDevice, format))
		{
			GetWindowDepthStencilFormat(IFrameBuffer::WindowDepthStencilMode::DepthFloat32, physicalDevice, format, hasScencilComponent);
		}
		hasScencilComponent = false;
		return;
	case IFrameBuffer::WindowDepthStencilMode::DepthUNorm24StencilUInt8:
		format = VK_FORMAT_D24_UNORM_S8_UINT;
		if (!IsDepthStencilFormatSupported(physicalDevice, format))
		{
			GetWindowDepthStencilFormat(IFrameBuffer::WindowDepthStencilMode::DepthFloat32StencilUInt8, physicalDevice, format, hasScencilComponent);
		}
		hasScencilComponent = true;
		return;
	case IFrameBuffer::WindowDepthStencilMode::DepthFloat32:
		format = VK_FORMAT_D32_SFLOAT;
		if (!IsDepthStencilFormatSupported(physicalDevice, format))
		{
			GetWindowDepthStencilFormat(IFrameBuffer::WindowDepthStencilMode::DepthFloat32StencilUInt8, physicalDevice, format, hasScencilComponent);
		}
		hasScencilComponent = false;
		return;
	case IFrameBuffer::WindowDepthStencilMode::DepthFloat32StencilUInt8:
		format = VK_FORMAT_D32_SFLOAT_S8_UINT;
		hasScencilComponent = true;
		return;
	}
	format = VK_FORMAT_UNDEFINED;
	hasScencilComponent = false;
}

//----------------------------------------------------------------------------------------
// Update methods
//----------------------------------------------------------------------------------------
void VulkanFrameBuffer::AcquireNextImage()
{
	// If this is a headless window, there's no presentation to do, so abort any further processing.
	if (_drawState.headlessWindow)
	{
		return;
	}

	// Select image to render to
	if (_drawState.boundToWindow)
	{
		// Prepare semaphore will be signalled once the image can be presented
		VkResult imageResult = vkAcquireNextImageKHR(_renderer->GetDevice(), _swapchain, std::numeric_limits<uint64_t>::max(), _prepareSemaphore, VK_NULL_HANDLE, &_swapchainImageIndex);

		// Swap out the present semaphore. Note that we need to keep a separate semaphore for each framebuffer image,
		// and swap it over after calling vkAcquireNextImageKHR above, as this is the only way to ensure we're
		// synchronized with the present operation and guarantee the present semaphore isn't still in use. See the
		// following:
		// https://docs.vulkan.org/guide/latest/swapchain_semaphore_reuse.html
		if ((imageResult == VK_SUCCESS) || (imageResult == VK_SUBOPTIMAL_KHR))
		{
			_presentSemaphore = _swapchainPresentSemaphores[_swapchainImageIndex];
		}

		// Log any issues which occurred
		if ((imageResult == VK_ERROR_OUT_OF_DATE_KHR) || (imageResult == VK_SUBOPTIMAL_KHR))
		{
			if (_renderer->DebugLoggingEnabled())
			{
				_log->Debug("vkQueuePresentKHR: Swapchain is out of date or suboptimal");
			}
			_drawState.framebufferInvalid = true;
		}
		else if (imageResult != VK_SUCCESS)
		{
			_log->Warning("vkAcquireNextImageKHR: Could not get image from swapchain");
			_drawState.framebufferInvalid = true;
		}
	}
}

//----------------------------------------------------------------------------------------
void VulkanFrameBuffer::PresentToWindow()
{
	// If this is a headless window, there's no presentation to do, so abort any further processing.
	if (_drawState.headlessWindow)
	{
		return;
	}

	// Present the image
	VkResult result = vkQueuePresentKHR(_presentQueue, &_presentInfo);
	if ((result == VK_ERROR_OUT_OF_DATE_KHR) || (result == VK_SUBOPTIMAL_KHR))
	{
		if (_renderer->DebugLoggingEnabled())
		{
			_log->Debug("vkQueuePresentKHR: Swapchain is out of date or suboptimal");
		}
		_drawState.framebufferInvalid = true;
	}
	else if (result != VK_SUCCESS)
	{
		_log->Warning("vkQueuePresentKHR: Could not present swapchain image");
		_drawState.framebufferInvalid = true;
	}

	// When running under MoltenVK on macOS, we've observed that when re-binding to a window which was previously used
	// with a different format (IE, like depth/stencil mode), the first frame will succeed, but the second frame will
	// fail with a VK_SUBOPTIMAL_KHR return, however despite that code supposed to be meaning the draw succeeded, we
	// have observed that the render operation will always fail. This means we can't just re-create the swapchain on the
	// next frame like we should be able to. We also can't predict when this issue will occur, as the first frame always
	// draws fine without any warnings, while the second fails completely. This happens when an entirely new render tree
	// is setup as well, where no existing state is carried over. To deal with this, we always re-create the swapchain
	// after the first frame draw on a newly bound window when running on macOS.
#ifdef __APPLE__
	if (_firstFrameForWindowBufferPending)
	{
		_drawState.framebufferInvalid = true;
		_firstFrameForWindowBufferPending = false;
	}
#endif
}

//----------------------------------------------------------------------------------------
VkSemaphore VulkanFrameBuffer::GetPresentSemaphore()
{
	return _presentSemaphore;
}

//----------------------------------------------------------------------------------------
VkSemaphore VulkanFrameBuffer::GetPrepareSemaphore()
{
	return _prepareSemaphore;
}

//----------------------------------------------------------------------------------------
// Build state methods
//----------------------------------------------------------------------------------------
void VulkanFrameBuffer::MigrateBuildStateToDrawState()
{
	// We need to preserve the "referenceNo" field of the "boundTextures" entry of the draw state here, since that gets
	// populated when the framebuffer is being created from the draw process. If the framebuffer is invalid in the build
	// state though, this array may have changed, and the "referenceNo" field will be recalculated anyway when the
	// framebuffer is bound next, so we don't need to preserve the numbers in this case.
	if (!_buildState.framebufferInvalid)
	{
		_buildState.boundTextures = _drawState.boundTextures;
		_buildState.resolveTextures = _drawState.resolveTextures;
	}

	// If we're running on macOS, track whether we're potentially switching to a new bound window. See notes in
	// PresentToWindow() for why we do this.
#ifdef __APPLE__
	if (_buildState.boundToWindow && !_buildState.headlessWindow && _buildState.framebufferInvalid)
	{
		_firstFrameForWindowBufferPending = true;
	}
#endif

	// If there are pending updates to process in the current draw state, carry them over to the new build state too.
	_buildState.framebufferInvalid |= _drawState.framebufferInvalid;
	_buildState.viewportChanged |= _drawState.viewportChanged;

	// Transfer all state data from the (now updated) build state into the draw state
	_drawState = _buildState;

	// Since the pending update flags will now be handled by the draw state, and rolled back into the build state if
	// they aren't processed, clear the update flags in the new build state.
	_buildState.framebufferInvalid = false;
	_buildState.viewportChanged = false;

	// Reset the flag indicating that the object has been modified
	_stateModified.clear(std::memory_order_relaxed);
}

//----------------------------------------------------------------------------------------
void VulkanFrameBuffer::FlagBuildStateModified()
{
	if (!_stateModified.test_and_set(std::memory_order_acquire))
	{
		_renderer->FlagObjectModified(this);
	}
}

} // namespace cobalt::graphics
