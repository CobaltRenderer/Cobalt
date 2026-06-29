// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "Direct3DFrameBuffer.h"
#include "Direct3DFrameBufferOutput.h"
#include "Direct3DRenderer.h"
#include <Internal/RendererSupport/KnownDynamicCast.h>
#include <vector>
namespace cobalt::graphics {
using Microsoft::WRL::ComPtr;

//----------------------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------------------
Direct3DFrameBuffer::Direct3DFrameBuffer(cobalt::logging::ILogger* log, Direct3DRenderer* renderer)
: _log(log), _renderer(renderer)
{
	_buildState.viewportRegionStartPos = {};
	_buildState.viewportRegionSize = {};
	_buildState.framebufferInvalid = false;
	_buildState.boundToWindow = false;
	_buildState.headlessWindow = false;
	_buildState.viewportInvalid = false;
	_buildState.scissorRegionDefined = false;
	_framebufferCreated = false;
	_checkedTearingFeaturePresent = false;
	_tearingFeaturePresent = false;
	_renderTargetViews.reserve(D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT);
}

//----------------------------------------------------------------------------------------
Direct3DFrameBuffer::~Direct3DFrameBuffer()
{
	DeleteNativeObjects();
}

//----------------------------------------------------------------------------------------
// Initialization methods
//----------------------------------------------------------------------------------------
void Direct3DFrameBuffer::Delete()
{
	_renderer->DeleteObject(this);
}

//----------------------------------------------------------------------------------------
// Binding methods
//----------------------------------------------------------------------------------------
SuccessToken Direct3DFrameBuffer::BindTexture(ITextureBuffer2D* texture, AttachmentType type, size_t index)
{
	// Ensure the specified texture is able to be bound
	auto* textureResolved = KnownDynamicCast<Direct3DTextureBuffer2D*>(texture);
	if (((uint32_t)textureResolved->GetUsageFlags() & (uint32_t)ITextureBuffer::UsageFlags::FrameBufferOutput) == 0)
	{
		_log->Error("Attempted to bind texture to framebuffer when the usage flags for the texture don't allow framebuffer output.");
		return false;
	}

	// Add this texture to the list of bound textures for this framebuffer
	std::unique_lock<std::mutex> lock(_accessMutex);
	BoundTextureInfo textureInfo = {};
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
	_buildState.windowInfo = std::monostate();
	_buildState.framebufferInvalid = true;
	lock.unlock();
	FlagBuildStateModified();
	return true;
}

//----------------------------------------------------------------------------------------
void Direct3DFrameBuffer::UnbindTexture(AttachmentType type, size_t index)
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
SuccessToken Direct3DFrameBuffer::BindMultiSamplingResolveTexture(ITextureBuffer2D* texture, AttachmentType type, size_t index)
{
	// Ensure the specified texture is able to be bound
	auto* textureResolved = KnownDynamicCast<Direct3DTextureBuffer2D*>(texture);
	if (((uint32_t)textureResolved->GetUsageFlags() & (uint32_t)ITextureBuffer::UsageFlags::MultiSampleResolve) == 0)
	{
		_log->Error("Attempted to bind resolve texture to framebuffer when the usage flags for the texture don't allow multisample texture resolution.");
		return false;
	}
	if (type != IFrameBuffer::AttachmentType::Color)
	{
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
void Direct3DFrameBuffer::UnbindMultiSamplingResolveTexture(AttachmentType type, size_t index)
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
SuccessToken Direct3DFrameBuffer::BindWindow(const WindowInfoBase& windowInfo, WindowDepthStencilMode depthStencilMode, WindowColorSpaceMode colorSpaceMode, WindowBindingFlags bindingFlags)
{
	// Update the framebuffer state, and mark it as modified.
	std::unique_lock<std::mutex> lock(_accessMutex);
	if ((windowInfo.windowType == IFrameBuffer::WindowInfoBase::WindowType::Headless) && (windowInfo.structureSizeInBytes == sizeof(WindowInfoHeadless)))
	{
		_buildState.windowInfo = *reinterpret_cast<const WindowInfoHeadless*>(&windowInfo);
		_buildState.headlessWindow = true;
	}
	else if ((windowInfo.windowType == IFrameBuffer::WindowInfoBase::WindowType::Win32) && (windowInfo.structureSizeInBytes == sizeof(WindowInfoWin32)))
	{
		_buildState.windowInfo = *reinterpret_cast<const WindowInfoWin32*>(&windowInfo);
		_buildState.headlessWindow = false;
	}
	else
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
	_buildState.viewportInvalid = true;
	lock.unlock();
	FlagBuildStateModified();
	return true;
}

//----------------------------------------------------------------------------------------
SuccessToken Direct3DFrameBuffer::NotifyWindowResized(const V2UInt32& windowSizeInPixels)
{
	// Ensure we're currently bound to a window
	std::unique_lock<std::mutex> lock(_accessMutex);
	if (!_buildState.boundToWindow)
	{
		_log->Warning("NotifyWindowResized called when the framebuffer is not currently bound to a window.");
		return false;
	}

	// Update the framebuffer state, and mark it as modified.
	if (auto* windowInfo = std::get_if<WindowInfoHeadless>(&_buildState.windowInfo))
	{
		windowInfo->windowSizeInPixels = windowSizeInPixels;
	}
	if (auto* windowInfo = std::get_if<WindowInfoWin32>(&_buildState.windowInfo))
	{
		windowInfo->windowSizeInPixels = windowSizeInPixels;
	}
	_buildState.viewportInvalid = true;
	lock.unlock();
	FlagBuildStateModified();
	return true;
}

//----------------------------------------------------------------------------------------
void Direct3DFrameBuffer::BindFrameBuffer(ID3D12GraphicsCommandList* commandList, ID3D12CommandQueue* commandQueue, IDXGIFactory4* dxgiFactory, bool performResourceStateTransition)
{
	// If the native objects for this framebuffer have previously been created but are no longer valid, release them
	// now.
	if (_framebufferCreated && _drawState.framebufferInvalid)
	{
		DeleteNativeObjects();
		_drawState.framebufferInvalid = false;
		_framebufferCreated = false;
	}

	// If we need to create or update our native framebuffer objects, do it now.
	if (!_framebufferCreated)
	{
		if (_drawState.boundToWindow)
		{
			CreateNativeObjectsForWindowTarget(_renderer->GetDevice(), commandQueue, dxgiFactory, false);
		}
		else
		{
			CreateNativeObjectsForTextureTarget();
		}
		_framebufferCreated = true;
		_drawState.framebufferInvalid = false;
		_drawState.viewportInvalid = true;

		// Increment the framebuffer object last update token
		if (++_frameBufferObjectLastUpdateToken > 100000)
		{
			_frameBufferObjectLastUpdateToken = 1;
		}
	}
	else if (_drawState.boundToWindow && _drawState.viewportInvalid)
	{
		// Resize the framebuffer if the window has been resized
		V2UInt32 windowSizeInPixels = {};
		if (_drawState.headlessWindow)
		{
			const auto* windowInfo = std::get_if<WindowInfoHeadless>(&_drawState.windowInfo);
			windowSizeInPixels = windowInfo->windowSizeInPixels;
		}
		else
		{
			const auto* windowInfo = std::get_if<WindowInfoWin32>(&_drawState.windowInfo);
			windowSizeInPixels = windowInfo->windowSizeInPixels;
		}
		if ((_framebufferSize.X() != windowSizeInPixels.X()) || (_framebufferSize.Y() != windowSizeInPixels.Y()))
		{
			CreateNativeObjectsForWindowTarget(_renderer->GetDevice(), commandQueue, dxgiFactory, true);
		}
	}

	// If the viewport settings have changed, update the viewport details now.
	if (_drawState.viewportInvalid)
	{
		UpdateViewport();
		DeleteCaptureStagingTextures();
		_drawState.viewportInvalid = false;
	}

	// Transition our buffers to render targets if required
	if (performResourceStateTransition)
	{
		if (!_drawState.boundToWindow)
		{
			bool alreadyTransitionedDepthStencilTexture = false;
			for (const BoundTextureInfo& boundTexture : _drawState.boundTextures)
			{
				if (boundTexture.type != AttachmentType::Color)
				{
					if (alreadyTransitionedDepthStencilTexture)
					{
						continue;
					}
					alreadyTransitionedDepthStencilTexture = true;
				}
				D3D12_RESOURCE_STATES newResourceState = (boundTexture.type != AttachmentType::Color ? D3D12_RESOURCE_STATE_DEPTH_WRITE : D3D12_RESOURCE_STATE_RENDER_TARGET);
				auto renderTargetTransition = CD3DX12_RESOURCE_BARRIER::Transition(boundTexture.texture->GetTexture(), boundTexture.texture->GetLastResourceState(), newResourceState);
				commandList->ResourceBarrier(1, &renderTargetTransition);
				boundTexture.texture->SetLastResourceState(newResourceState);
			}
		}
		else
		{
			if (!_drawState.headlessWindow)
			{
				// Note that we don't transition the depth/stencil texture if present, since that doesn't get presented.
				auto renderTargetTransitionColor = CD3DX12_RESOURCE_BARRIER::Transition(_swapChainBuffers[_currentWindowBackBufferIndex].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
				commandList->ResourceBarrier(1, &renderTargetTransitionColor);
			}
		}
	}

	// Setup the viewport
	commandList->RSSetViewports(1, &_viewport);

	// Under Direct3D 12, scissor testing is always enabled, and unless we define at least one scissor rectangle,
	// nothing will be drawn to the framebuffer. We therefore always pass through a scissor rectangle here, which is set
	// to the entire viewport region when the caller has requested the scissor test be disabled.
	commandList->RSSetScissorRects(1, &_scissorRect);

	// Retrieve the descriptor handle for the depth/stencil view
	const CD3DX12_CPU_DESCRIPTOR_HANDLE* depthStencilViewNativeHandle = nullptr;
	DescriptorHandle* depthStencilViewHandle = _depthStencilView.get();
	if (depthStencilViewHandle != nullptr)
	{
		depthStencilViewNativeHandle = &depthStencilViewHandle->GetNativeCPUHandle();
	}

	// Bind our render targets for this framebuffer
	size_t renderTargetCount = _renderTargetViewsFlatArray.size();
	if (renderTargetCount > D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT)
	{
		renderTargetCount = D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT;
		if (!_warnedAboutRenderTargetLimitExceeded)
		{
			_log->Warning("A framebuffer has {0} bound render targets, when a max of {1} are supported. Limiting to {2}.", _renderTargetViewsFlatArray.size(), D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT, renderTargetCount);
			_warnedAboutRenderTargetLimitExceeded = true;
		}
	}
	commandList->OMSetRenderTargets((UINT)renderTargetCount, _renderTargetViewsFlatArray.data(), FALSE, depthStencilViewNativeHandle);
}

//----------------------------------------------------------------------------------------
void Direct3DFrameBuffer::UnbindFrameBuffer(ID3D12GraphicsCommandList* commandList)
{
	// Transition our texture buffers out of a render target state
	if (!_drawState.boundToWindow)
	{
		bool alreadyTransitionedDepthStencilTexture = false;
		for (const BoundTextureInfo& boundTexture : _drawState.boundTextures)
		{
			if (boundTexture.type != AttachmentType::Color)
			{
				if (alreadyTransitionedDepthStencilTexture)
				{
					continue;
				}
				alreadyTransitionedDepthStencilTexture = true;
			}
			D3D12_RESOURCE_STATES newResourceState = boundTexture.texture->GetDefaultResourceState();
			auto renderTargetTransition = CD3DX12_RESOURCE_BARRIER::Transition(boundTexture.texture->GetTexture(), boundTexture.texture->GetLastResourceState(), newResourceState);
			commandList->ResourceBarrier(1, &renderTargetTransition);
			boundTexture.texture->SetLastResourceState(newResourceState);
		}
	}
	else
	{
		if (!_drawState.headlessWindow)
		{
			// Note that we don't transition the depth/stencil texture if present, since that doesn't get presented.
			auto renderTargetTransitionColor = CD3DX12_RESOURCE_BARRIER::Transition(_swapChainBuffers[_currentWindowBackBufferIndex].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
			commandList->ResourceBarrier(1, &renderTargetTransitionColor);
		}
	}
}

//----------------------------------------------------------------------------------------
void Direct3DFrameBuffer::GetDataFormats(DXGI_FORMAT (&renderTargetFormats)[8], UINT& populatedRenderTargetFormatEntries, DXGI_FORMAT& depthStencilFormat) const
{
	// Ensure we can fit all our render targets in the supplied render target formats array
	constexpr uint32_t arrayLength = (sizeof(renderTargetFormats) / sizeof(renderTargetFormats[0]));
	auto renderTargetFormatsLength = (uint32_t)_renderTargetFormats.size();
	if (renderTargetFormatsLength > arrayLength)
	{
		renderTargetFormatsLength = arrayLength;
	}

	// Populate the list of render target formats
	populatedRenderTargetFormatEntries = renderTargetFormatsLength;
	for (uint32_t i = 0; i < arrayLength; ++i)
	{
		renderTargetFormats[i] = (i < renderTargetFormatsLength) ? _renderTargetFormats[i] : DXGI_FORMAT_UNKNOWN;
	}

	// Populate the depth stencil format
	depthStencilFormat = _depthStencilFormat;
}

//----------------------------------------------------------------------------------------
bool Direct3DFrameBuffer::IsBoundToWindow() const
{
	return _drawState.boundToWindow && !_drawState.headlessWindow;
}

//----------------------------------------------------------------------------------------
int Direct3DFrameBuffer::GetFrameBufferObjectLastUpdateToken() const
{
	return _frameBufferObjectLastUpdateToken;
}

//----------------------------------------------------------------------------------------
ITextureBuffer::SampleCount Direct3DFrameBuffer::GetSampleCount(UINT& nativeSampleCount, UINT& sampleQuality) const
{
	nativeSampleCount = _nativeSampleCount;
	sampleQuality = _sampleQuality;
	return _sampleCount;
}

//----------------------------------------------------------------------------------------
// Viewport methods
//----------------------------------------------------------------------------------------
void Direct3DFrameBuffer::DefineViewportRegion(const V2UInt32& startPos, const V2UInt32& size)
{
	// Update the framebuffer state, and mark it as modified.
	std::unique_lock<std::mutex> lock(_accessMutex);
	_buildState.viewportRegionStartPos = startPos;
	_buildState.viewportRegionSize = size;
	_buildState.viewportInvalid = true;
	lock.unlock();
	FlagBuildStateModified();
}

//----------------------------------------------------------------------------------------
void Direct3DFrameBuffer::DefineScissorRegion(const V2UInt32& startPos, const V2UInt32& size)
{
	// Update the framebuffer state, and mark it as modified.
	std::unique_lock<std::mutex> lock(_accessMutex);
	_buildState.scissorRegionDefined = true;
	_buildState.scissorRegionStartPos = startPos;
	_buildState.scissorRegionSize = size;
	_buildState.viewportInvalid = true;
	lock.unlock();
	FlagBuildStateModified();
}

//----------------------------------------------------------------------------------------
void Direct3DFrameBuffer::RemoveScissorRegion()
{
	// Update the framebuffer state, and mark it as modified.
	std::unique_lock<std::mutex> lock(_accessMutex);
	_buildState.scissorRegionDefined = false;
	_buildState.viewportInvalid = true;
	lock.unlock();
	FlagBuildStateModified();
}

//----------------------------------------------------------------------------------------
// Output capture methods
//----------------------------------------------------------------------------------------
bool Direct3DFrameBuffer::HasCaptureTargets() const
{
	return !_drawState.captureTargets.empty();
}

//----------------------------------------------------------------------------------------
void Direct3DFrameBuffer::AddOutputCaptureTarget(IFrameBufferOutput* captureTarget, AttachmentType type, size_t index)
{
	std::unique_lock<std::mutex> lock(_accessMutex);
	CaptureTargetInfo captureTargetInfo = {};
	captureTargetInfo.captureTarget = KnownDynamicCast<Direct3DFrameBufferOutput*>(captureTarget);
	captureTargetInfo.type = type;
	captureTargetInfo.index = index;
	_buildState.captureTargets.push_back(captureTargetInfo);
	lock.unlock();
	FlagBuildStateModified();
}

//----------------------------------------------------------------------------------------
void Direct3DFrameBuffer::RemoveOutputCaptureTarget(IFrameBufferOutput* captureTarget)
{
	std::unique_lock<std::mutex> lock(_accessMutex);
	auto* captureTargetResolved = KnownDynamicCast<Direct3DFrameBufferOutput*>(captureTarget);
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
}

//----------------------------------------------------------------------------------------
void Direct3DFrameBuffer::CaptureFrameBufferOutput(ID3D12GraphicsCommandList* commandList)
{
	// Ensure we have enough staging slots for the currently bound capture targets. Existing compatible buffers are
	// reused below, but newly added targets need new staging entries.
	auto captureTargetCount = (uint32_t)_drawState.captureTargets.size();
	if (_stagingTexturesForCapture.size() < captureTargetCount)
	{
		_stagingTexturesForCapture.resize(captureTargetCount);
	}

	// Store our framebuffer output in any capture targets that have been bound
	for (uint32_t i = 0; i < captureTargetCount; ++i)
	{
		// Attempt to locate the bound texture and image format for the target framebuffer output
		CaptureTargetInfo& captureTargetInfo = _drawState.captureTargets[i];
		ID3D12Resource* sourceTextureBuffer = nullptr;
		ITextureBuffer::ImageFormat imageFormat{};
		ITextureBuffer::DataFormat dataFormat{};
		size_t elementCount{};
		size_t elementSizeInBytes{};
		size_t pixelOffsetInBytes{};
		size_t pixelStrideInBytes{};
		DXGI_FORMAT textureFormat{};
		D3D12_RESOURCE_STATES lastResourceState{};
		if (_drawState.boundToWindow)
		{
			if (captureTargetInfo.index == 0)
			{
				// Retrieve the texture resource and image format for the target window texture
				if (captureTargetInfo.type == IFrameBuffer::AttachmentType::Color)
				{
					lastResourceState = (_drawState.headlessWindow ? D3D12_RESOURCE_STATE_RENDER_TARGET : D3D12_RESOURCE_STATE_PRESENT);
					sourceTextureBuffer = _swapChainBuffers[_currentWindowBackBufferIndex].Get();
					textureFormat = _colorBufferFormat;
				}
				else if ((captureTargetInfo.type == IFrameBuffer::AttachmentType::Depth) || (captureTargetInfo.type == IFrameBuffer::AttachmentType::Stencil))
				{
					lastResourceState = D3D12_RESOURCE_STATE_DEPTH_WRITE;
					sourceTextureBuffer = _depthStencilTexture.Get();
					textureFormat = _depthStencilFormat;
				}

				// Extract additional information about the image format
				if (sourceTextureBuffer != nullptr)
				{
					if (!GetFormatFromNativeFormat(textureFormat, imageFormat, dataFormat, elementCount, elementSizeInBytes, pixelOffsetInBytes, pixelStrideInBytes, captureTargetInfo.type == IFrameBuffer::AttachmentType::Stencil))
					{
						_log->Error("Failed to identify a compatible data format for framebuffer capture with native format {0}", textureFormat);
						continue;
					}
				}
			}
		}
		else
		{
			for (const BoundTextureInfo& boundTexture : _drawState.boundTextures)
			{
				if ((boundTexture.type == captureTargetInfo.type) && (boundTexture.index == captureTargetInfo.index))
				{
					const auto& textureObject = *boundTexture.texture;
					textureFormat = textureObject.GetTextureFormat();
					sourceTextureBuffer = textureObject.GetTexture();
					textureObject.GetTextureFormatInfo(imageFormat, dataFormat, elementCount, elementSizeInBytes, pixelOffsetInBytes, pixelStrideInBytes, (boundTexture.type == IFrameBuffer::AttachmentType::Stencil));
					lastResourceState = textureObject.GetLastResourceState();
					break;
				}
			}
		}

		// Direct3D 12 doesn't allow us to read interleaved depth buffer formats directly. Instead, we need to read
		// using the typeless formats below, and let the implementation handle some of the conversion work for us.
		if ((textureFormat == DXGI_FORMAT_D24_UNORM_S8_UINT) || (textureFormat == DXGI_FORMAT_D32_FLOAT_S8X24_UINT))
		{
			if (captureTargetInfo.type == IFrameBuffer::AttachmentType::Depth)
			{
				textureFormat = DXGI_FORMAT_R32_TYPELESS;
			}
			else if (captureTargetInfo.type == IFrameBuffer::AttachmentType::Stencil)
			{
				textureFormat = (textureFormat == DXGI_FORMAT_D24_UNORM_S8_UINT ? DXGI_FORMAT_X24_TYPELESS_G8_UINT : DXGI_FORMAT_X32_TYPELESS_G8X24_UINT);
			}
		}

		// If we couldn't locate the target framebuffer output, log an error, and advance to the next capture entry.
		if (sourceTextureBuffer == nullptr)
		{
			_log->Error("Failed to locate target framebuffer output for target type {0} and index {1}", captureTargetInfo.type, captureTargetInfo.index);
			continue;
		}

		// Calculate the width and height of the region to capture. Note that for depth/stencil buffers, and for
		// multisampled color buffers, we are required to capture the entire subresource, so we can't use a cropped
		// capture region here. See the following:
		// https://learn.microsoft.com/en-us/windows/win32/api/d3d12/nf-d3d12-id3d12graphicscommandlist-copytextureregion
		// Also note that if we're capturing the stencil buffer, we need to select the second "plane slice", as the
		// depth component is stored in the first slice. See the following for more information:
		// https://learn.microsoft.com/en-us/windows/win32/direct3d12/subresources#plane-slice
		UINT sourceSubresource = (captureTargetInfo.type == IFrameBuffer::AttachmentType::Stencil ? 1U : 0U);
		bool fullSubresourceCapture = (captureTargetInfo.type != IFrameBuffer::AttachmentType::Color) || (_sampleCount != ITextureBuffer::SampleCount::SampleCount1);
		uint32_t captureWidth = _framebufferSize.X();
		uint32_t captureHeight = _framebufferSize.Y();
		Direct3DFrameBufferOutput* captureTarget = captureTargetInfo.captureTarget;
		V2UInt32 requestedImageOffset = captureTarget->GetRequestedImageOffset();
		V2UInt32 requestedImageSize = captureTarget->GetRequestedImageSize();
		V2UInt32 croppedImageSize = Direct3DFrameBufferOutput::CalculateCroppedImageDimensions(V2UInt32(captureWidth, captureHeight), requestedImageOffset, requestedImageSize);

		// Calculate our memory buffer format and layout requirements
		D3D12_PLACED_SUBRESOURCE_FOOTPRINT bufferFootprint;
		size_t uploadBufferSizeInBytes = 0;
		if (fullSubresourceCapture)
		{
			UINT64 requiredBufferSizeInBytes = 0;
			auto sourceDesc = sourceTextureBuffer->GetDesc();
			_renderer->GetDevice()->GetCopyableFootprints(&sourceDesc, sourceSubresource, 1, 0, &bufferFootprint, nullptr, nullptr, &requiredBufferSizeInBytes);
			uploadBufferSizeInBytes = (size_t)requiredBufferSizeInBytes;

			// When we copy a full depth/stencil or multisampled subresource, the copied buffer layout is defined by the
			// footprint returned from GetCopyableFootprints rather than the original source texture format. Update the
			// per-pixel layout metadata here to describe the copied buffer contents accurately for later readback.
			if (captureTargetInfo.type == IFrameBuffer::AttachmentType::Stencil)
			{
				// Treat the captured stencil plane as tightly packed 8-bit values in the readback buffer. Explicitly use an
				// R8-compatible placed footprint format for the copy destination rather than relying on the combined
				// depth/stencil resource format.
				bufferFootprint.Footprint.Format = DXGI_FORMAT_R8_TYPELESS;
				elementSizeInBytes = 1;
				pixelStrideInBytes = 1;
				pixelOffsetInBytes = 0;
			}
			else if (captureTargetInfo.type == IFrameBuffer::AttachmentType::Depth)
			{
				if ((dataFormat == ITextureBuffer::DataFormat::DepthUNorm24) || (dataFormat == ITextureBuffer::DataFormat::DepthUNorm24StencilUInt8))
				{
					elementSizeInBytes = 3;
				}
				else
				{
					elementSizeInBytes = 4;
				}
				pixelStrideInBytes = 4;
				pixelOffsetInBytes = 0;
			}
		}
		else
		{
			UINT64 requiredBufferSizeInBytes = 0;
			auto copyRegionDesc = CD3DX12_RESOURCE_DESC::Tex2D(textureFormat, croppedImageSize.X(), croppedImageSize.Y(), 1, 1, 1, 0, D3D12_RESOURCE_FLAG_NONE);
			_renderer->GetDevice()->GetCopyableFootprints(&copyRegionDesc, 0, 1, 0, &bufferFootprint, nullptr, nullptr, &requiredBufferSizeInBytes);
			uploadBufferSizeInBytes = (size_t)requiredBufferSizeInBytes;
		}

		// Retrieve or create a staging texture compatible with the framebuffer output
		auto& stagingTextureInfo = _stagingTexturesForCapture[i];
		if ((stagingTextureInfo.buffer == nullptr) || (stagingTextureInfo.bufferSizeInBytes < uploadBufferSizeInBytes))
		{
			DeleteCaptureStagingTexture(stagingTextureInfo);

			// Create a readback buffer for this capture target
			auto resourceDescription = CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSizeInBytes);
			D3D12MA::ALLOCATION_DESC allocationDesc = {};
			allocationDesc.Flags = D3D12MA::ALLOCATION_FLAG_NONE;
			allocationDesc.HeapType = D3D12_HEAP_TYPE_READBACK;
			auto& memoryManager = _renderer->MemoryManager();
			HRESULT createResourceResult = memoryManager.CreateResource(&allocationDesc, &resourceDescription, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, &stagingTextureInfo.allocation, IID_PPV_ARGS(&stagingTextureInfo.buffer));
			if (FAILED(createResourceResult))
			{
				_log->Error("Failed to create staging buffer for framebuffer capture with error code {0}", createResourceResult);
				continue;
			}
			stagingTextureInfo.bufferSizeInBytes = uploadBufferSizeInBytes;
		}
		ID3D12Resource* stagingTexture = stagingTextureInfo.buffer.Get();

		// Attempt to capture the framebuffer output
		if (!CaptureFrameBufferOutput(commandList, captureTargetInfo, sourceTextureBuffer, stagingTexture, lastResourceState, sourceSubresource, bufferFootprint, croppedImageSize, imageFormat, dataFormat, elementCount, elementSizeInBytes, pixelOffsetInBytes, pixelStrideInBytes))
		{
			_log->Error("Failed to capture framebuffer output for target type {0} and index {1}", captureTargetInfo.type, captureTargetInfo.index);
			continue;
		}
	}
}

//----------------------------------------------------------------------------------------
bool Direct3DFrameBuffer::CaptureFrameBufferOutput(ID3D12GraphicsCommandList* commandList, CaptureTargetInfo& captureTargetInfo, ID3D12Resource* sourceTextureBuffer, ID3D12Resource* stagingTextureBuffer, D3D12_RESOURCE_STATES lastResourceState, UINT sourceSubresource, const D3D12_PLACED_SUBRESOURCE_FOOTPRINT& bufferFootprint, const V2UInt32& croppedImageSize, ITextureBuffer::ImageFormat imageFormat, ITextureBuffer::DataFormat dataFormat, size_t elementCount, size_t elementSizeInBytes, size_t pixelOffsetInBytes, size_t pixelStrideInBytes)
{
	// Retrieve the requested capture settings from our framebuffer output object
	Direct3DFrameBufferOutput* captureTarget = captureTargetInfo.captureTarget;
	V2UInt32 requestedImageOffset = captureTarget->GetRequestedImageOffset();
	V2UInt32 requestedImageSize = captureTarget->GetRequestedImageSize();

	// Determine whether this copy must capture the whole subresource. In those cases, cropping is handled after the
	// readback rather than via the CopyTextureRegion source box.
	bool fullSubresourceCapture = (captureTargetInfo.type != IFrameBuffer::AttachmentType::Color) || (_sampleCount != ITextureBuffer::SampleCount::SampleCount1);

	// Transition our buffer into a state where we can transfer into it from our staging buffer
	auto acquireBarrier = CD3DX12_RESOURCE_BARRIER::Transition(sourceTextureBuffer, lastResourceState, D3D12_RESOURCE_STATE_COPY_SOURCE);
	commandList->ResourceBarrier(1, &acquireBarrier);

	// Transfer our framebuffer output to the staging texture
	CD3DX12_TEXTURE_COPY_LOCATION sourceLocation(sourceTextureBuffer, sourceSubresource);
	CD3DX12_TEXTURE_COPY_LOCATION targetLocation(stagingTextureBuffer, bufferFootprint);
	if (fullSubresourceCapture)
	{
		commandList->CopyTextureRegion(&targetLocation, 0, 0, 0, &sourceLocation, nullptr);
	}
	else
	{
		D3D12_BOX sourceRegion;
		sourceRegion.left = requestedImageOffset.X();
		sourceRegion.right = sourceRegion.left + croppedImageSize.X();
		sourceRegion.top = requestedImageOffset.Y();
		sourceRegion.bottom = sourceRegion.top + croppedImageSize.Y();
		sourceRegion.front = 0;
		sourceRegion.back = 1;
		commandList->CopyTextureRegion(&targetLocation, 0, 0, 0, &sourceLocation, &sourceRegion);
	}

	// Transition our buffer back into its default state
	auto releaseBarrier = CD3DX12_RESOURCE_BARRIER::Transition(sourceTextureBuffer, D3D12_RESOURCE_STATE_COPY_SOURCE, lastResourceState);
	commandList->ResourceBarrier(1, &releaseBarrier);

	// Store info on this framebuffer capture process for later processing
	size_t rowStrideInBytes = bufferFootprint.Footprint.RowPitch;
	size_t bufferSizeInBytes = rowStrideInBytes * bufferFootprint.Footprint.Height;
	captureTargetInfo.completionPending = true;
	captureTargetInfo.stagingTextureBuffer = stagingTextureBuffer;
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
void Direct3DFrameBuffer::CompleteCaptureFrameBufferOutput()
{
	// Complete any pending framebuffer capture processes
	auto captureTargetCount = (uint32_t)_drawState.captureTargets.size();
	for (uint32_t i = 0; i < captureTargetCount; ++i)
	{
		CaptureTargetInfo& captureTargetInfo = _drawState.captureTargets[i];
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
bool Direct3DFrameBuffer::CompleteCaptureFrameBufferOutput(const CaptureTargetInfo& captureTargetInfo)
{
	// Map our staging texture into memory so we can access its data
	uint8_t* stagingTextureMapped = nullptr;
	if (FAILED(captureTargetInfo.stagingTextureBuffer->Map(0, nullptr, reinterpret_cast<void**>(&stagingTextureMapped))))
	{
		_log->Error("Failed to map staging texture for framebuffer capture");
		return false;
	}

	// Write the texture data into our capture target
	Direct3DFrameBufferOutput* captureTarget = captureTargetInfo.captureTarget;
	captureTarget->StoreCaptureData(captureTargetInfo.croppedImageSize, stagingTextureMapped, captureTargetInfo.bufferSizeInBytes, captureTargetInfo.imageFormat, captureTargetInfo.dataFormat, captureTargetInfo.elementCount, captureTargetInfo.elementSizeInBytes, captureTargetInfo.pixelOffsetInBytes, captureTargetInfo.pixelStrideInBytes, captureTargetInfo.rowStrideInBytes, (captureTargetInfo.type == IFrameBuffer::AttachmentType::Stencil));

	// Unmap our staging texture
	CD3DX12_RANGE writtenRange(0, 0);
	captureTargetInfo.stagingTextureBuffer->Unmap(0, &writtenRange);

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
void Direct3DFrameBuffer::DeleteCaptureStagingTexture(CaptureStagingTextureInfo& entry)
{
	// D3D12MA requires releasing the resource before the allocation.
	// https://gpuopen-librariesandsdks.github.io/D3D12MemoryAllocator/html/quick_start.html#quick_start_resource_reference_counting
	entry.buffer.Reset();
	if (entry.allocation != nullptr)
	{
		entry.allocation->Release();
		entry.allocation = nullptr;
	}
	entry.bufferSizeInBytes = 0;
}

//----------------------------------------------------------------------------------------
void Direct3DFrameBuffer::DeleteCaptureStagingTextures()
{
	for (auto& entry : _stagingTexturesForCapture)
	{
		DeleteCaptureStagingTexture(entry);
	}
	_stagingTexturesForCapture.clear();
}

//----------------------------------------------------------------------------------------
bool Direct3DFrameBuffer::GetFormatFromNativeFormat(DXGI_FORMAT nativeFormat, ITextureBuffer::ImageFormat& imageFormat, ITextureBuffer::DataFormat& dataFormat, size_t& elementCount, size_t& elementSizeInBytes, size_t& pixelOffsetInBytes, size_t& pixelStrideInBytes, bool stencilComponent)
{
	//##TODO## DXGI_FORMAT_R10G10B10A2_UNORM
	switch (nativeFormat)
	{
	case DXGI_FORMAT_B8G8R8A8_UNORM:
		imageFormat = ITextureBuffer::ImageFormat::BGRA;
		dataFormat = ITextureBuffer::DataFormat::UInt8;
		elementCount = ITextureBuffer::ElementCountPerPixelFromFormat(imageFormat);
		elementSizeInBytes = ITextureBuffer::ByteSizePerElementFromFormat(dataFormat);
		pixelStrideInBytes = elementSizeInBytes * elementCount;
		pixelOffsetInBytes = 0;
		return true;
	case DXGI_FORMAT_R8G8B8A8_UNORM:
		imageFormat = ITextureBuffer::ImageFormat::RGBA;
		dataFormat = ITextureBuffer::DataFormat::UInt8;
		elementCount = ITextureBuffer::ElementCountPerPixelFromFormat(imageFormat);
		elementSizeInBytes = ITextureBuffer::ByteSizePerElementFromFormat(dataFormat);
		pixelStrideInBytes = elementSizeInBytes * elementCount;
		pixelOffsetInBytes = 0;
		return true;
	case DXGI_FORMAT_R16G16B16A16_FLOAT:
		imageFormat = ITextureBuffer::ImageFormat::RGBA;
		dataFormat = ITextureBuffer::DataFormat::Float16;
		elementCount = ITextureBuffer::ElementCountPerPixelFromFormat(imageFormat);
		elementSizeInBytes = ITextureBuffer::ByteSizePerElementFromFormat(dataFormat);
		pixelStrideInBytes = elementSizeInBytes * elementCount;
		pixelOffsetInBytes = 0;
		return true;
	case DXGI_FORMAT_R32G32B32_FLOAT:
		imageFormat = ITextureBuffer::ImageFormat::RGB;
		dataFormat = ITextureBuffer::DataFormat::Float32;
		elementCount = ITextureBuffer::ElementCountPerPixelFromFormat(imageFormat);
		elementSizeInBytes = ITextureBuffer::ByteSizePerElementFromFormat(dataFormat);
		pixelStrideInBytes = elementSizeInBytes * elementCount;
		pixelOffsetInBytes = 0;
		return true;
	case DXGI_FORMAT_R32G32B32A32_FLOAT:
		imageFormat = ITextureBuffer::ImageFormat::RGBA;
		dataFormat = ITextureBuffer::DataFormat::Float32;
		elementCount = ITextureBuffer::ElementCountPerPixelFromFormat(imageFormat);
		elementSizeInBytes = ITextureBuffer::ByteSizePerElementFromFormat(dataFormat);
		pixelStrideInBytes = elementSizeInBytes * elementCount;
		pixelOffsetInBytes = 0;
		return true;
	case DXGI_FORMAT_D16_UNORM:
		imageFormat = ITextureBuffer::ImageFormat::Depth;
		dataFormat = ITextureBuffer::DataFormat::DepthUNorm16;
		elementCount = ITextureBuffer::ElementCountPerPixelFromFormat(imageFormat);
		elementSizeInBytes = ITextureBuffer::ByteSizePerElementFromFormat(dataFormat);
		pixelStrideInBytes = elementSizeInBytes * elementCount;
		pixelOffsetInBytes = 0;
		return true;
	case DXGI_FORMAT_D24_UNORM_S8_UINT:
		imageFormat = ITextureBuffer::ImageFormat::DepthAndStencil;
		dataFormat = ITextureBuffer::DataFormat::DepthUNorm24StencilUInt8;
		elementCount = 1;
		// Note that Direct3D stores depth information in the first "plane slice" and stencil in the second. When
		// reading data out of them, we need to use stride and size values for the formats they expose the slices as.
		// See the following for more information:
		// https://learn.microsoft.com/en-us/windows/win32/direct3d12/subresources#plane-slice
		if (!stencilComponent)
		{
			// Depth plane is 32-bit typeless, with the 24-bit depth payload stored at the start of each 32-bit element.
			elementSizeInBytes = 3;
			pixelStrideInBytes = 4;
			pixelOffsetInBytes = 0;
		}
		else
		{
			// Stencil plane is exposed through DXGI_FORMAT_X24_TYPELESS_G8_UINT, so the returned buffer still advances
			// in 32-bit units with the stencil payload in the final byte.
			elementSizeInBytes = 1;
			pixelStrideInBytes = 4;
			pixelOffsetInBytes = 3;
		}
		return true;
	case DXGI_FORMAT_D32_FLOAT:
		imageFormat = ITextureBuffer::ImageFormat::Depth;
		dataFormat = ITextureBuffer::DataFormat::Float32;
		elementCount = ITextureBuffer::ElementCountPerPixelFromFormat(imageFormat);
		elementSizeInBytes = ITextureBuffer::ByteSizePerElementFromFormat(dataFormat);
		pixelStrideInBytes = elementSizeInBytes * elementCount;
		pixelOffsetInBytes = 0;
		return true;
	case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
		imageFormat = ITextureBuffer::ImageFormat::DepthAndStencil;
		dataFormat = ITextureBuffer::DataFormat::DepthFloat32StencilUInt8;
		elementCount = 1;
		// Note that Direct3D stores depth information in the first "plane slice" and stencil in the second. When
		// reading data out of them, we need to use stride and size values for the formats they expose the slices as.
		// See the following for more information:
		// https://learn.microsoft.com/en-us/windows/win32/direct3d12/subresources#plane-slice
		if (!stencilComponent)
		{
			// Depth plane is 32-bit typeless
			elementSizeInBytes = 4;
			pixelStrideInBytes = 4;
			pixelOffsetInBytes = 0;
		}
		else
		{
			// Stencil plane is exposed through DXGI_FORMAT_X32_TYPELESS_G8X24_UINT, so the returned buffer still
			// advances in 64-bit units with the stencil payload beginning at byte offset 4.
			elementSizeInBytes = 1;
			pixelStrideInBytes = 8;
			pixelOffsetInBytes = 4;
		}
		return true;
	}
	return false;
}

//----------------------------------------------------------------------------------------
// Framebuffer update methods
//----------------------------------------------------------------------------------------
bool Direct3DFrameBuffer::PresentToWindow()
{
	if (_drawState.headlessWindow)
	{
		return true;
	}

	// Present our back buffer to the window
	bool limitToVsync = ((_drawState.windowBindingFlags & WindowBindingFlags::LimitSwapToVSync) != WindowBindingFlags::None);
	bool allowTearing = ((_drawState.windowBindingFlags & WindowBindingFlags::AllowTearing) != WindowBindingFlags::None);
	UINT syncInterval = (limitToVsync ? 1 : 0);
	UINT presentFlags = (((syncInterval == 0) && allowTearing && _tearingFeaturePresent) ? DXGI_PRESENT_ALLOW_TEARING : 0);
	auto presentResult = _swapChain->Present(syncInterval, presentFlags);
	if (FAILED(presentResult))
	{
		_log->Error("Present failed on swap chain with error code {0}", presentResult);
		return false;
	}

	// Set the current back buffer resource as the active render target
	_currentWindowBackBufferIndex = _swapChain->GetCurrentBackBufferIndex();
	_renderTargetViewsFlatArray[0] = _renderTargetViews[_currentWindowBackBufferIndex]->GetNativeCPUHandle();
	return true;
}

//----------------------------------------------------------------------------------------
void Direct3DFrameBuffer::ClearRenderView(ID3D12GraphicsCommandList* commandList, size_t index, const V4Float32& val)
{
	// Ensure the supplied index number is valid
	if (index >= _renderTargetViewsFlatArray.size())
	{
		_log->Error("Attempted to clear render target view with invalid index {0}", index);
		return;
	}

	// Clear the render target view
	commandList->ClearRenderTargetView(_renderTargetViewsFlatArray[index], val.data(), 0, nullptr);
}

//----------------------------------------------------------------------------------------
void Direct3DFrameBuffer::ClearDepthStencilView(ID3D12GraphicsCommandList* commandList, float depthVal, uint32_t stencilVal)
{
	// Ensure a depth view has been bound
	auto* depthStencilView = _depthStencilView.get();
	if (depthStencilView == nullptr)
	{
		_log->Error("Attempted to clear depth view when no depth texture has been bound to the framebuffer");
		return;
	}

	// Clear the depth view
	commandList->ClearDepthStencilView(depthStencilView->GetNativeCPUHandle(), _depthStencilClearFlags, depthVal, UINT8(stencilVal), 0, nullptr);
}

//----------------------------------------------------------------------------------------
void Direct3DFrameBuffer::UpdateViewport()
{
	// Setup the viewport
	_viewport.Width = (FLOAT)_drawState.viewportRegionSize.X();
	_viewport.Height = (FLOAT)_drawState.viewportRegionSize.Y();
	_viewport.MinDepth = 0.0f;
	_viewport.MaxDepth = 1.0f;
	_viewport.TopLeftX = (FLOAT)_drawState.viewportRegionStartPos.X();
	_viewport.TopLeftY = (FLOAT)_drawState.viewportRegionStartPos.Y();

	// Setup the scissor rectangle
	if (_drawState.scissorRegionDefined)
	{
		_scissorRect.left = _drawState.scissorRegionStartPos.X();
		_scissorRect.right = _scissorRect.left + _drawState.scissorRegionSize.X();
		_scissorRect.top = _drawState.scissorRegionStartPos.Y();
		_scissorRect.bottom = _scissorRect.top + _drawState.scissorRegionSize.Y();
	}
	else
	{
		_scissorRect.left = _drawState.viewportRegionStartPos.X();
		_scissorRect.right = _scissorRect.left + _drawState.viewportRegionSize.X();
		_scissorRect.top = _drawState.viewportRegionStartPos.Y();
		_scissorRect.bottom = _scissorRect.top + _drawState.viewportRegionSize.Y();
	}
}

//----------------------------------------------------------------------------------------
bool Direct3DFrameBuffer::CreateNativeObjectsForWindowTarget(ID3D12Device* device, ID3D12CommandQueue* commandQueue, IDXGIFactory4* dxgiFactory, bool resizeExistingSwapChain)
{
	// Select the colour format to use for the window framebuffer
	DXGI_FORMAT colorTextureFormat;
	GetWindowColorFormat(_drawState.windowColorSpaceMode, colorTextureFormat);

	// Select the depth/stencil format to use for the window framebuffer
	DXGI_FORMAT depthStencilTextureFormat = DXGI_FORMAT_UNKNOWN;
	bool depthStencilTextureHasStencilComponent = false;
	bool hasDepthTexture = (_drawState.windowDepthStencilMode != WindowDepthStencilMode::None);
	if (hasDepthTexture)
	{
		GetWindowDepthStencilFormat(_drawState.windowDepthStencilMode, depthStencilTextureFormat, depthStencilTextureHasStencilComponent);
	}

	// Set the depth/stencil buffer clear flags
	_depthStencilClearFlags = D3D12_CLEAR_FLAG_DEPTH;
	if (depthStencilTextureHasStencilComponent)
	{
		_depthStencilClearFlags |= D3D12_CLEAR_FLAG_STENCIL;
	}

	// Get the current window size. Note that we need the actual physical window size here, not the viewport size, which
	// may be a restricted part of the window.
	V2UInt32 windowSizeInPixels = {};
	const WindowInfoWin32* nativeWindowInfo = nullptr;
	if (_drawState.headlessWindow)
	{
		const auto* windowInfo = std::get_if<WindowInfoHeadless>(&_drawState.windowInfo);
		windowSizeInPixels = windowInfo->windowSizeInPixels;
	}
	else
	{
		nativeWindowInfo = std::get_if<WindowInfoWin32>(&_drawState.windowInfo);
		windowSizeInPixels = nativeWindowInfo->windowSizeInPixels;
	}
	UINT windowSizeX = (UINT)windowSizeInPixels.X();
	UINT windowSizeY = (UINT)windowSizeInPixels.Y();

	// A headless window target is represented by renderer-owned offscreen textures rather than a DXGI swap chain.
	if (_drawState.headlessWindow)
	{
		_renderTargetViewsFlatArray.clear();
		_renderTargetViews.clear();
		_swapChainBuffers.clear();
		_depthStencilView.reset();
		_depthStencilTexture.Reset();
		_renderTargetFormats.clear();
		_swapChain.Reset();

		auto heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
		auto colorResourceDescription = CD3DX12_RESOURCE_DESC::Tex2D(colorTextureFormat, windowSizeX, windowSizeY, 1, 1, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);
		ComPtr<ID3D12Resource> headlessColorBuffer;
		HRESULT createHeadlessColorBufferReturn = device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &colorResourceDescription, D3D12_RESOURCE_STATE_RENDER_TARGET, nullptr, IID_PPV_ARGS(&headlessColorBuffer));
		if (FAILED(createHeadlessColorBufferReturn))
		{
			_log->Error("Failed to create headless color buffer with error code {0}", createHeadlessColorBufferReturn);
			return false;
		}
		_swapChainBuffers.push_back(std::move(headlessColorBuffer));

		auto renderTargetView = _renderer->AllocateDescriptor(Direct3DHeapManager::ResourceType::RenderTargetView);
		device->CreateRenderTargetView(_swapChainBuffers[0].Get(), nullptr, renderTargetView->GetNativeCPUHandle());
		_renderTargetViews.push_back(std::move(renderTargetView));
		_renderTargetViewsFlatArray.resize(1);
		_renderTargetViewsFlatArray[0] = _renderTargetViews[0]->GetNativeCPUHandle();
		_renderTargetFormats.push_back(colorTextureFormat);
		_currentWindowBackBufferIndex = 0;
		_colorBufferFormat = colorTextureFormat;

		if (hasDepthTexture)
		{
			auto resourceDescription = CD3DX12_RESOURCE_DESC::Tex2D(depthStencilTextureFormat, windowSizeX, windowSizeY, 1, 1, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);
			CD3DX12_CLEAR_VALUE depthClearValue(depthStencilTextureFormat, 1.0f, 0);
			HRESULT createHeadlessDepthBufferReturn = _renderer->GetDevice()->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &resourceDescription, D3D12_RESOURCE_STATE_DEPTH_WRITE, &depthClearValue, IID_PPV_ARGS(&_depthStencilTexture));
			if (FAILED(createHeadlessDepthBufferReturn))
			{
				_log->Error("Failed to create headless depth buffer with error code {0}", createHeadlessDepthBufferReturn);
				return false;
			}

			D3D12_DEPTH_STENCIL_VIEW_DESC depthStencilViewDescription = {};
			depthStencilViewDescription.Format = depthStencilTextureFormat;
			depthStencilViewDescription.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
			_depthStencilView = _renderer->AllocateDescriptor(Direct3DHeapManager::ResourceType::DepthStencilView);
			device->CreateDepthStencilView(_depthStencilTexture.Get(), &depthStencilViewDescription, _depthStencilView->GetNativeCPUHandle());
		}

		_framebufferSize = windowSizeInPixels;
		_sampleCount = ITextureBuffer::SampleCount::SampleCount1;
		_nativeSampleCount = Direct3DTextureBuffer2D::GetNativeSampleCountFromSampleCount(_sampleCount);
		_sampleQuality = 0;
		_depthStencilFormat = depthStencilTextureFormat;
		return true;
	}

	// Build our set of swap chain flags
	if (!_checkedTearingFeaturePresent)
	{
		_tearingFeaturePresent = _renderer->IsFeaturePresent(DXGI_FEATURE_PRESENT_ALLOW_TEARING);
		_checkedTearingFeaturePresent = true;
	}
	bool allowTearing = _tearingFeaturePresent && ((_drawState.windowBindingFlags & WindowBindingFlags::AllowTearing) != WindowBindingFlags::None);
	UINT swapChainFlags = (allowTearing ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0);

	// Create or resize our swap chain
	if (resizeExistingSwapChain)
	{
		// Synchronize with swap chain usage if we're re-creating the swap chain
		if (!WaitForWindowTargetRelease())
		{
			_log->Error("WaitForWindowTargetRelease failed");
			return false;
		}

		// Remove any bound render target and depth views
		_renderTargetViewsFlatArray.clear();
		_renderTargetViews.clear();
		_swapChainBuffers.clear();
		_depthStencilView.reset();
		_depthStencilTexture.Reset();

		// Attempt to resize the swap chain
		HRESULT resizeBuffersReturn = _swapChain->ResizeBuffers(0, windowSizeX, windowSizeY, DXGI_FORMAT_UNKNOWN, swapChainFlags);
		if (FAILED(resizeBuffersReturn))
		{
			_log->Error("ResizeBuffers failed with error code {0}", resizeBuffersReturn);
			return false;
		}
	}
	else
	{
		// Create the swap chain
		ComPtr<IDXGISwapChain1> swapChainOld;
		DXGI_FORMAT renderTargetFormat = colorTextureFormat;
		DXGI_SWAP_CHAIN_DESC1 swapChainDescription = {};
		swapChainDescription.Width = windowSizeX;
		swapChainDescription.Height = windowSizeY;
		swapChainDescription.Format = renderTargetFormat;
		swapChainDescription.SampleDesc.Count = 1;
		swapChainDescription.SampleDesc.Quality = 0;
		swapChainDescription.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swapChainDescription.BufferCount = 2;
		swapChainDescription.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
		swapChainDescription.Flags = swapChainFlags;
		HRESULT createSwapChainForHwndReturn = dxgiFactory->CreateSwapChainForHwnd(commandQueue, nativeWindowInfo->windowHandle, &swapChainDescription, nullptr, nullptr, &swapChainOld);
		if (FAILED(createSwapChainForHwndReturn))
		{
			_log->Error("CreateSwapChainForHwnd failed with error code {0}", createSwapChainForHwndReturn);
			return false;
		}

		// Retrieve the newer swap chain interface required for Direct3D 12
		HRESULT querySwapChain3Return = swapChainOld.As(&_swapChain);
		if (FAILED(querySwapChain3Return))
		{
			_log->Error("Failed to retrieve the IDXGISwapChain3 interface from the swap chain with error code {0}", querySwapChain3Return);
			return false;
		}

		// Record the format of the render target
		_renderTargetFormats.push_back(renderTargetFormat);
	}

	// Create our render target views for the framebuffer
	_renderTargetViews.resize(FrameCount);
	_swapChainBuffers.resize(FrameCount);
	for (uint32_t i = 0; i < FrameCount; ++i)
	{
		ComPtr<ID3D12Resource> swapChainBuffer;
		HRESULT getSwapChainBufferReturn = _swapChain->GetBuffer(i, IID_PPV_ARGS(&swapChainBuffer));
		if (FAILED(getSwapChainBufferReturn))
		{
			_log->Error("GetBuffer failed with error code {0}", getSwapChainBufferReturn);
			return false;
		}

		auto renderTargetView = _renderer->AllocateDescriptor(Direct3DHeapManager::ResourceType::RenderTargetView);
		device->CreateRenderTargetView(swapChainBuffer.Get(), nullptr, renderTargetView->GetNativeCPUHandle());
		_renderTargetViews[i] = std::move(renderTargetView);
		_swapChainBuffers[i] = std::move(swapChainBuffer);
	}

	// Record the format of the color target
	_colorBufferFormat = colorTextureFormat;

	// Set the current back buffer resource as the active render target
	_currentWindowBackBufferIndex = _swapChain->GetCurrentBackBufferIndex();
	_renderTargetViewsFlatArray.resize(1);
	_renderTargetViewsFlatArray[0] = _renderTargetViews[_currentWindowBackBufferIndex]->GetNativeCPUHandle();

	// Clear any existing depth stencil texture
	_depthStencilTexture.Reset();
	_depthStencilView.reset();

	// Create a depth stencil texture if required
	if (hasDepthTexture)
	{
		// Create a depth stencil texture for the framebuffer
		auto heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
		auto resourceDescription = CD3DX12_RESOURCE_DESC::Tex2D(depthStencilTextureFormat, windowSizeX, windowSizeY, 1, 1, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);
		CD3DX12_CLEAR_VALUE depthClearValue(depthStencilTextureFormat, 1.0f, 0);
		HRESULT createWindowDepthBufferReturn = _renderer->GetDevice()->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &resourceDescription, D3D12_RESOURCE_STATE_DEPTH_WRITE, &depthClearValue, IID_PPV_ARGS(&_depthStencilTexture));
		if (FAILED(createWindowDepthBufferReturn))
		{
			_log->Error("Failed to create window depth buffer with error code {0}", createWindowDepthBufferReturn);
			return false;
		}

		// Create a depth stencil view
		D3D12_DEPTH_STENCIL_VIEW_DESC
		depthStencilViewDescription = {};
		depthStencilViewDescription.Format = depthStencilTextureFormat;
		depthStencilViewDescription.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
		_depthStencilView = _renderer->AllocateDescriptor(Direct3DHeapManager::ResourceType::DepthStencilView);
		device->CreateDepthStencilView(_depthStencilTexture.Get(), &depthStencilViewDescription, _depthStencilView->GetNativeCPUHandle());
	}

	// Record the framebuffer size and sample count
	_framebufferSize = windowSizeInPixels;
	_sampleCount = ITextureBuffer::SampleCount::SampleCount1;
	_nativeSampleCount = Direct3DTextureBuffer2D::GetNativeSampleCountFromSampleCount(_sampleCount);
	_sampleQuality = 0;

	// Record the format of the depth stencil target
	_depthStencilFormat = depthStencilTextureFormat;
	return true;
}

//----------------------------------------------------------------------------------------
bool Direct3DFrameBuffer::WaitForWindowTargetRelease()
{
	// Add a synchronization fence if we're detaching from a window, to ensure that the buffer is finished being used
	// before we try and release it. Without this, we'll get an exception on a debug context when we try and free the
	// swap chain, and of course, potential stability issues without one.
	Microsoft::WRL::ComPtr<ID3D12Fence> bufferFreedFence;
	HRESULT createBufferFreedFenceReturn = _renderer->GetDevice()->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&bufferFreedFence));
	if (FAILED(createBufferFreedFenceReturn))
	{
		_log->Error("CreateFence failed when waiting for window target release with error code {0}", createBufferFreedFenceReturn);
		return false;
	}
	HANDLE bufferFreedEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	if (bufferFreedEvent == nullptr)
	{
		DWORD createBufferFreedEventLastError = GetLastError();
		_log->Error("Failed to create window target release event with error code {0}", createBufferFreedEventLastError);
		return false;
	}
	_renderer->GetDrawCommandQueue()->Signal(bufferFreedFence.Get(), 1);
	if (bufferFreedFence->GetCompletedValue() < 1)
	{
		bufferFreedFence->SetEventOnCompletion(1, bufferFreedEvent);
		WaitForSingleObject(bufferFreedEvent, INFINITE);
	}
	CloseHandle(bufferFreedEvent);
	return true;
}

//----------------------------------------------------------------------------------------
bool Direct3DFrameBuffer::CreateNativeObjectsForTextureTarget()
{
	// Reset our recorded render target formats
	_depthStencilFormat = DXGI_FORMAT_UNKNOWN;
	_depthStencilClearFlags = {};

	// Iterate our bound texture resources and create views of each one. We also calculate the framebuffer sample count
	// here.
	bool latchedInitialTextureInfo = false;
	V2UInt32 framebufferSize = V2UInt32(0, 0);
	ITextureBuffer::SampleCount framebufferSampleCount = ITextureBuffer::SampleCount::SampleCount1;
	bool foundDepthStencilTarget = false;
	for (const BoundTextureInfo& textureInfo : _drawState.boundTextures)
	{
		// Obtain the size and sample count for this texture, and ensure it is compatible with other bound textures.
		auto* texture = textureInfo.texture;
		auto textureDimensions = texture->MipmapLevelDimensions(0);
		auto textureSampleCount = texture->GetSampleCount();
		if (!latchedInitialTextureInfo)
		{
			framebufferSize = textureDimensions;
			framebufferSampleCount = textureSampleCount;
			latchedInitialTextureInfo = true;
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

		if (textureInfo.type != AttachmentType::Color)
		{
			// Set the depth/stencil buffer clear flags
			if (textureInfo.type == IFrameBuffer::AttachmentType::Depth)
			{
				_depthStencilClearFlags |= D3D12_CLEAR_FLAG_DEPTH;
			}
			else if (textureInfo.type == IFrameBuffer::AttachmentType::Stencil)
			{
				_depthStencilClearFlags |= D3D12_CLEAR_FLAG_STENCIL;
			}

			if (!foundDepthStencilTarget)
			{
				// Create a depth/stencil view
				ID3D12Resource* textureResource = texture->GetTexture();
				_depthStencilView = _renderer->AllocateDescriptor(Direct3DHeapManager::ResourceType::DepthStencilView);
				_renderer->GetDevice()->CreateDepthStencilView(textureResource, nullptr, _depthStencilView->GetNativeCPUHandle());

				// Record the format of the depth stencil target
				_depthStencilFormat = texture->GetTextureFormat();
				foundDepthStencilTarget = true;
			}
		}
		else
		{
			// Create a render target view
			auto renderTargetView = _renderer->AllocateDescriptor(Direct3DHeapManager::ResourceType::RenderTargetView);
			ID3D12Resource* textureResource = texture->GetTexture();
			_renderer->GetDevice()->CreateRenderTargetView(textureResource, nullptr, renderTargetView->GetNativeCPUHandle());

			// Add this render target view to the list of render target views
			if (textureInfo.index >= _renderTargetViews.size())
			{
				_renderTargetViewsFlatArray.resize(textureInfo.index + 1);
				_renderTargetViews.resize(textureInfo.index + 1);
			}
			_renderTargetViewsFlatArray[textureInfo.index] = renderTargetView->GetNativeCPUHandle();
			_renderTargetViews[textureInfo.index] = std::move(renderTargetView);

			// Record the format of the render target
			if (_renderTargetFormats.size() <= textureInfo.index)
			{
				_renderTargetFormats.resize(textureInfo.index + 1, DXGI_FORMAT_UNKNOWN);
			}
			_renderTargetFormats[textureInfo.index] = textureInfo.texture->GetTextureFormat();
		}
	}

	// Record the calculated framebuffer size and sample count
	_framebufferSize = framebufferSize;
	_sampleCount = framebufferSampleCount;
	_nativeSampleCount = Direct3DTextureBuffer2D::GetNativeSampleCountFromSampleCount(_sampleCount);
	_sampleQuality = 0;
	return true;
}

//----------------------------------------------------------------------------------------
void Direct3DFrameBuffer::DeleteNativeObjects()
{
	// Synchronize with swap chain usage if we're unbinding from a window
	if (_drawState.boundToWindow && !_drawState.headlessWindow && !WaitForWindowTargetRelease())
	{
		_log->Error("WaitForWindowTargetRelease failed");
		return;
	}

	// Delete any staging texture buffers we've allocated
	DeleteCaptureStagingTextures();

	// Delete the remaining allocated objects
	_renderTargetViewsFlatArray.clear();
	_renderTargetViews.clear();
	_swapChainBuffers.clear();
	_depthStencilView.reset();
	_depthStencilTexture.Reset();
	_renderTargetFormats.clear();
	_swapChain.Reset();
}

//----------------------------------------------------------------------------------------
void Direct3DFrameBuffer::GetWindowColorFormat(WindowColorSpaceMode mode, DXGI_FORMAT& textureFormat)
{
	textureFormat = DXGI_FORMAT_B8G8R8A8_UNORM;
}

//----------------------------------------------------------------------------------------
void Direct3DFrameBuffer::GetWindowDepthStencilFormat(WindowDepthStencilMode mode, DXGI_FORMAT& textureFormat, bool& hasScencilComponent)
{
	switch (mode)
	{
	case IFrameBuffer::WindowDepthStencilMode::DepthUNorm16:
		textureFormat = DXGI_FORMAT_D16_UNORM;
		hasScencilComponent = false;
		return;
	case IFrameBuffer::WindowDepthStencilMode::DepthUNorm24:
		textureFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
		hasScencilComponent = false;
		return;
	case IFrameBuffer::WindowDepthStencilMode::DepthUNorm24StencilUInt8:
		textureFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
		hasScencilComponent = true;
		return;
	case IFrameBuffer::WindowDepthStencilMode::DepthFloat32:
		textureFormat = DXGI_FORMAT_D32_FLOAT;
		hasScencilComponent = false;
		return;
	case IFrameBuffer::WindowDepthStencilMode::DepthFloat32StencilUInt8:
		textureFormat = DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
		hasScencilComponent = true;
		return;
	}
	textureFormat = DXGI_FORMAT_UNKNOWN;
	hasScencilComponent = false;
}

//----------------------------------------------------------------------------------------
void Direct3DFrameBuffer::ResolveMultiSamplingAttachmentToTexture(ID3D12GraphicsCommandList* commandList, AttachmentType type, size_t index, size_t resolveIndex)
{
	// Locate the source texture
	bool foundSourceEntry = false;
	Direct3DTextureBuffer2D* sourceTexture = nullptr;
	for (const auto& entry : _drawState.boundTextures)
	{
		if ((entry.type == type) && (entry.index == index))
		{
			foundSourceEntry = true;
			sourceTexture = entry.texture;
		}
	}
	if (!foundSourceEntry)
	{
		_log->Error("Failed to locate the source multisampling resolve attachment with type {0} and index {1}", type, index);
		return;
	}

	// Locate the resolve texture
	bool foundResolveEntry = false;
	Direct3DTextureBuffer2D* resolveTexture = nullptr;
	for (const auto& entry : _drawState.resolveTextures)
	{
		if ((entry.type == type) && (entry.index == resolveIndex))
		{
			foundResolveEntry = true;
			resolveTexture = entry.texture;
		}
	}
	if (!foundResolveEntry)
	{
		_log->Error("Failed to locate the target multisampling resolve attachment with type {0} and index {1}", type, resolveIndex);
		return;
	}

	// Determine if we're performing a copy or a resolve operation
	bool copyResolveRequired = (_sampleCount == ITextureBuffer::SampleCount::SampleCount1);
	D3D12_RESOURCE_STATES sourceResolveState = (copyResolveRequired ? D3D12_RESOURCE_STATE_COPY_SOURCE : D3D12_RESOURCE_STATE_RESOLVE_SOURCE);
	D3D12_RESOURCE_STATES targetResolveState = (copyResolveRequired ? D3D12_RESOURCE_STATE_COPY_DEST : D3D12_RESOURCE_STATE_RESOLVE_DEST);

	// Transition the texture resources into the correct state for the operation
	auto sourceTextureTransitionStart = CD3DX12_RESOURCE_BARRIER::Transition(sourceTexture->GetTexture(), sourceTexture->GetLastResourceState(), sourceResolveState);
	commandList->ResourceBarrier(1, &sourceTextureTransitionStart);
	auto targetTextureTransitionStart = CD3DX12_RESOURCE_BARRIER::Transition(resolveTexture->GetTexture(), resolveTexture->GetLastResourceState(), targetResolveState);
	commandList->ResourceBarrier(1, &targetTextureTransitionStart);

	// Perform the texture resolve operation, emulating it with a direct copy if a 1x sample count is active.
	if (!copyResolveRequired)
	{
		commandList->ResolveSubresource(resolveTexture->GetTexture(), 0, sourceTexture->GetTexture(), 0, resolveTexture->GetNativeTextureFormat());
	}
	else
	{
		commandList->CopyResource(resolveTexture->GetTexture(), sourceTexture->GetTexture());
	}

	// Transition the texture resources back to their previous state
	auto sourceTextureTransitionEnd = CD3DX12_RESOURCE_BARRIER::Transition(sourceTexture->GetTexture(), sourceResolveState, sourceTexture->GetLastResourceState());
	commandList->ResourceBarrier(1, &sourceTextureTransitionEnd);
	auto targetTextureTransitionEnd = CD3DX12_RESOURCE_BARRIER::Transition(resolveTexture->GetTexture(), targetResolveState, resolveTexture->GetLastResourceState());
	commandList->ResourceBarrier(1, &targetTextureTransitionEnd);
}

//----------------------------------------------------------------------------------------
// Build state methods
//----------------------------------------------------------------------------------------
void Direct3DFrameBuffer::MigrateBuildStateToDrawState()
{
	// If there are pending updates to process in the current draw state, carry them over to the new build state too.
	_buildState.framebufferInvalid |= _drawState.framebufferInvalid;
	_buildState.viewportInvalid |= _drawState.viewportInvalid;

	// Transfer all state data from the (now updated) build state into the draw state
	_drawState = _buildState;

	// Since the pending update flags will now be handled by the draw state, and rolled back into the build state if
	// they aren't processed, clear the update flags in the new build state.
	_buildState.framebufferInvalid = false;
	_buildState.viewportInvalid = false;

	// Reset the flag indicating that the object has been modified
	_stateModified.clear(std::memory_order_relaxed);
}

//----------------------------------------------------------------------------------------
void Direct3DFrameBuffer::FlagBuildStateModified()
{
	if (!_stateModified.test_and_set(std::memory_order_acquire))
	{
		_renderer->FlagObjectModified(this);
	}
}

} // namespace cobalt::graphics
