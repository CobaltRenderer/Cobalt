// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "Direct3DFrameBuffer.h"
#include "Direct3DFrameBufferOutput.h"
#include "Direct3DRenderer.h"
#include "Direct3DTextureBuffer2D.h"
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
	_checkedFeatureAvailability = false;
	_tearingFeaturePresent = false;
	_flipDiscardModePresent = false;
	_renderTargetViews.reserve(D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT);
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
void Direct3DFrameBuffer::BindFrameBuffer(ID3D11Device1* device, ID3D11DeviceContext1* context, IDXGIFactory2* dxgiFactory)
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
			CreateNativeObjectsForWindowTarget(device, dxgiFactory, false);
		}
		else
		{
			CreateNativeObjectsForTextureTarget(device);
		}
		_framebufferCreated = true;
		_drawState.framebufferInvalid = false;
		_drawState.viewportInvalid = true;
	}
	else if (_drawState.boundToWindow && _drawState.viewportInvalid)
	{
		CreateNativeObjectsForWindowTarget(device, dxgiFactory, true);
	}

	// If the viewport settings have changed, update the viewport details now.
	if (_drawState.viewportInvalid)
	{
		UpdateViewport();
		_stagingTexturesForCapture.clear();
		_drawState.viewportInvalid = false;
	}

	// Setup the viewport
	context->RSSetViewports(1, &_viewport);

	// Setup the scissor rectangle. Note that we've made this always on for Direct3D 11 here, even though it's
	// theoretically optional in the API. We did this because the scissor test is always on in Direct3D 12 and Vulkan,
	// which most likely indicates that the hardware unit for this is always functioning too and can't really be
	// disabled. To simulate the scissor test being off, we simply use the viewport region as the scissor region too,
	// which is almost certainly what's really happening anyway when we turn it off here.
	context->RSSetScissorRects(1, &_scissorRect);

	// Bind our render targets for this framebuffer
	size_t renderTargetCount = _renderTargetViewsFlatArray.size();
	if (renderTargetCount > D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT)
	{
		renderTargetCount = D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT;
		if (!_warnedAboutRenderTargetLimitExceeded)
		{
			_log->Warning("A framebuffer has {0} bound render targets, when a max of {1} are supported. Limiting to {2}.", _renderTargetViewsFlatArray.size(), D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT, renderTargetCount);
			_warnedAboutRenderTargetLimitExceeded = true;
		}
	}
	context->OMSetRenderTargets((UINT)renderTargetCount, _renderTargetViewsFlatArray.data(), _depthStencilView.Get());
}

//----------------------------------------------------------------------------------------
bool Direct3DFrameBuffer::IsBoundToWindow() const
{
	return _drawState.boundToWindow && !_drawState.headlessWindow;
}

//----------------------------------------------------------------------------------------
ITextureBuffer::SampleCount Direct3DFrameBuffer::GetSampleCount() const
{
	return _sampleCount;
}

//----------------------------------------------------------------------------------------
ID3D11DepthStencilView* Direct3DFrameBuffer::GetDepthStencilView() const
{
	return _depthStencilView.Get();
}

//----------------------------------------------------------------------------------------
const std::vector<ID3D11RenderTargetView*>& Direct3DFrameBuffer::GetRenderTargetViews() const
{
	return _renderTargetViewsFlatArray;
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
void Direct3DFrameBuffer::CaptureFrameBufferOutput(ID3D11Device1* device, ID3D11DeviceContext1* context)
{
	// Store our framebuffer output in any capture targets that have been bound
	auto captureTargetCount = (uint32_t)_drawState.captureTargets.size();
	_stagingTexturesForCapture.resize(captureTargetCount);
	for (uint32_t i = 0; i < captureTargetCount; ++i)
	{
		// Attempt to locate the bound texture and image format for the target framebuffer output
		CaptureTargetInfo& captureTargetInfo = _drawState.captureTargets[i];
		ID3D11Texture2D* sourceTextureBuffer = nullptr;
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
				// Retrieve the texture resource for the target window texture
				if (captureTargetInfo.type == IFrameBuffer::AttachmentType::Color)
				{
					sourceTextureBuffer = _backBuffer.Get();
				}
				else if ((captureTargetInfo.type == IFrameBuffer::AttachmentType::Depth) || (captureTargetInfo.type == IFrameBuffer::AttachmentType::Stencil))
				{
					sourceTextureBuffer = _depthStencilTexture.Get();
				}

				// Extract additional information about the image format
				if (sourceTextureBuffer != nullptr)
				{
					D3D11_TEXTURE2D_DESC textureDescription;
					sourceTextureBuffer->GetDesc(&textureDescription);
					if (!GetFormatFromNativeFormat(textureDescription.Format, imageFormat, dataFormat, elementCount, elementSizeInBytes, pixelOffsetInBytes, pixelStrideInBytes, captureTargetInfo.type == IFrameBuffer::AttachmentType::Stencil))
					{
						_log->Error("Failed to identify a compatible data format for framebuffer capture with native format {0}", textureDescription.Format);
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
					sourceTextureBuffer = textureObject.GetTexture();
					textureObject.GetTextureFormatInfo(imageFormat, dataFormat, elementCount, elementSizeInBytes, pixelOffsetInBytes, pixelStrideInBytes, (boundTexture.type == IFrameBuffer::AttachmentType::Stencil));
					break;
				}
			}
		}

		// If we couldn't locate the target framebuffer output, log an error, and advance to the next capture entry.
		if (sourceTextureBuffer == nullptr)
		{
			_log->Error("Failed to locate target framebuffer output for target type {0} and index {1}", captureTargetInfo.type, captureTargetInfo.index);
			continue;
		}

		// Retrieve or create a staging texture compatible with the framebuffer output
		ID3D11Texture2D* stagingTexture = _stagingTexturesForCapture[i].Get();
		if (stagingTexture == nullptr)
		{
			D3D11_TEXTURE2D_DESC textureDescription;
			sourceTextureBuffer->GetDesc(&textureDescription);
			textureDescription.Usage = D3D11_USAGE_STAGING;
			textureDescription.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
			textureDescription.BindFlags = 0;
			textureDescription.MiscFlags = 0;
			HRESULT createStagingTextureForCaptureReturn = device->CreateTexture2D(&textureDescription, nullptr, &_stagingTexturesForCapture[i]);
			if (FAILED(createStagingTextureForCaptureReturn))
			{
				_log->Error("Failed to create staging texture for framebuffer capture with error code {0}", createStagingTextureForCaptureReturn);
				continue;
			}
			stagingTexture = _stagingTexturesForCapture[i].Get();
		}

		// Attempt to capture the framebuffer output
		if (!CaptureFrameBufferOutput(context, captureTargetInfo, sourceTextureBuffer, stagingTexture, imageFormat, dataFormat, elementCount, elementSizeInBytes, pixelOffsetInBytes, pixelStrideInBytes))
		{
			_log->Error("Failed to capture framebuffer output for target type {0} and index {1}", captureTargetInfo.type, captureTargetInfo.index);
			continue;
		}
	}
}

//----------------------------------------------------------------------------------------
bool Direct3DFrameBuffer::CaptureFrameBufferOutput(ID3D11DeviceContext1* context, const CaptureTargetInfo& captureTargetInfo, ID3D11Texture2D* sourceTextureBuffer, ID3D11Texture2D* stagingTextureBuffer, ITextureBuffer::ImageFormat imageFormat, ITextureBuffer::DataFormat dataFormat, size_t elementCount, size_t elementSizeInBytes, size_t pixelOffsetInBytes, size_t pixelStrideInBytes)
{
	// Retrieve image information from our staging texture
	D3D11_TEXTURE2D_DESC stagingTextureDescription;
	stagingTextureBuffer->GetDesc(&stagingTextureDescription);
	auto imageWidth = stagingTextureDescription.Width;
	auto imageHeight = stagingTextureDescription.Height;

	// Retrieve the requested capture settings from our framebuffer output object
	Direct3DFrameBufferOutput* captureTarget = captureTargetInfo.captureTarget;
	V2UInt32 requestedImageOffset = captureTarget->GetRequestedImageOffset();
	V2UInt32 requestedImageSize = captureTarget->GetRequestedImageSize();
	bool detachAfterCapture = captureTarget->IsDetachingAfterCapture();

	// Calculate the cropped size of the image, taking into account the requested image offset and size, if any.
	V2UInt32 croppedImageSize = Direct3DFrameBufferOutput::CalculateCroppedImageDimensions(V2UInt32(imageWidth, imageHeight), requestedImageOffset, requestedImageSize);

	// Transfer our framebuffer output to the staging texture
	if ((croppedImageSize.X() == imageWidth) && (croppedImageSize.Y() == imageHeight))
	{
		// Copy the entire buffer
		context->CopyResource(stagingTextureBuffer, sourceTextureBuffer);
	}
	else
	{
		// Copy the cropped region from the buffer
		D3D11_BOX box{};
		box.left = requestedImageOffset.X();
		box.top = requestedImageOffset.Y();
		box.right = requestedImageOffset.X() + croppedImageSize.X();
		box.bottom = requestedImageOffset.Y() + croppedImageSize.Y();
		box.front = 0;
		box.back = 1;
		context->CopySubresourceRegion1(stagingTextureBuffer, 0, 0, 0, 0, sourceTextureBuffer, 0, &box, D3D11_COPY_DISCARD);
	}

	// Map our staging texture into memory so we can access its data
	D3D11_MAPPED_SUBRESOURCE mappedResource;
	if (FAILED(context->Map(stagingTextureBuffer, 0, D3D11_MAP_READ, 0, &mappedResource)))
	{
		_log->Error("Failed to map staging texture for framebuffer capture");
		return false;
	}
	const auto* sourceData = reinterpret_cast<const unsigned char*>(mappedResource.pData);

	// Write the texture data into our capture target
	size_t rowStrideInBytes = mappedResource.RowPitch;
	size_t bufferSizeInBytes = mappedResource.DepthPitch;
	captureTarget->StoreCaptureData(croppedImageSize, sourceData, bufferSizeInBytes, imageFormat, dataFormat, elementCount, elementSizeInBytes, pixelOffsetInBytes, pixelStrideInBytes, rowStrideInBytes, (captureTargetInfo.type == IFrameBuffer::AttachmentType::Stencil));

	// Unmap our staging texture
	context->Unmap(stagingTextureBuffer, 0);

	// Record this captured framebuffer output with the renderer
	_renderer->AddCurrentFrameBufferOutput(captureTarget);

	// Now that we've captured a frame, detach the output capture target if requested.
	if (detachAfterCapture)
	{
		RemoveOutputCaptureTarget(captureTarget);
	}
	return true;
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
		if (!stencilComponent)
		{
			elementSizeInBytes = 3;
			pixelStrideInBytes = 4;
			pixelOffsetInBytes = 0;
		}
		else
		{
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
		if (!stencilComponent)
		{
			elementSizeInBytes = 4;
			pixelStrideInBytes = 8;
			pixelOffsetInBytes = 0;
		}
		else
		{
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
	// If this is a headless window, there's no presentation to do, so abort any further processing.
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
	return true;
}

//----------------------------------------------------------------------------------------
void Direct3DFrameBuffer::ClearRenderView(ID3D11DeviceContext1* context, size_t index, const V4Float32& val)
{
	// Ensure the supplied index number is valid
	if (index >= _renderTargetViews.size())
	{
		_log->Error("Attempted to clear render target view with invalid index {0}", index);
		return;
	}

	// Clear the render target view
	context->ClearRenderTargetView(_renderTargetViews[index].Get(), val.data());
}

//----------------------------------------------------------------------------------------
void Direct3DFrameBuffer::ClearDepthStencilView(ID3D11DeviceContext1* context, float depthVal, uint32_t stencilVal)
{
	// Ensure a depth view has been bound
	auto* depthStencilView = _depthStencilView.Get();
	if (depthStencilView == nullptr)
	{
		_log->Error("Attempted to clear depth view when no depth texture has been bound to the framebuffer");
		return;
	}

	// Clear the depth view
	context->ClearDepthStencilView(depthStencilView, _depthStencilClearFlags, depthVal, UINT8(stencilVal));
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
bool Direct3DFrameBuffer::CreateNativeObjectsForWindowTarget(ID3D11Device1* device, IDXGIFactory2* dxgiFactory, bool resizeExistingSwapChain)
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
	_depthStencilClearFlags = D3D11_CLEAR_DEPTH;
	if (depthStencilTextureHasStencilComponent)
	{
		_depthStencilClearFlags = D3D11_CLEAR_FLAG((unsigned int)_depthStencilClearFlags | (unsigned int)D3D11_CLEAR_STENCIL);
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
		_backBuffer.Reset();
		_depthStencilView.Reset();
		_depthStencilTexture.Reset();
		_swapChain.Reset();

		D3D11_TEXTURE2D_DESC colorTextureDescription = {};
		colorTextureDescription.Width = windowSizeX;
		colorTextureDescription.Height = windowSizeY;
		colorTextureDescription.MipLevels = 1;
		colorTextureDescription.ArraySize = 1;
		colorTextureDescription.SampleDesc.Count = 1;
		colorTextureDescription.SampleDesc.Quality = 0;
		colorTextureDescription.Format = colorTextureFormat;
		colorTextureDescription.BindFlags = D3D11_BIND_RENDER_TARGET;
		colorTextureDescription.CPUAccessFlags = 0;
		colorTextureDescription.Usage = D3D11_USAGE_DEFAULT;
		HRESULT createHeadlessColorTextureReturn = device->CreateTexture2D(&colorTextureDescription, nullptr, &_backBuffer);
		if (FAILED(createHeadlessColorTextureReturn))
		{
			_log->Error("CreateTexture2D failed for headless framebuffer with error code {0}", createHeadlessColorTextureReturn);
			return false;
		}

		ComPtr<ID3D11RenderTargetView> renderTargetView;
		if (FAILED(device->CreateRenderTargetView(_backBuffer.Get(), nullptr, &renderTargetView)))
		{
			_log->Error("CreateRenderTargetView failed");
			return false;
		}
		_renderTargetViewsFlatArray.push_back(renderTargetView.Get());
		_renderTargetViews.push_back(std::move(renderTargetView));

		if (hasDepthTexture)
		{
			D3D11_TEXTURE2D_DESC descDepth = {};
			descDepth.Width = windowSizeX;
			descDepth.Height = windowSizeY;
			descDepth.MipLevels = 1;
			descDepth.ArraySize = 1;
			descDepth.SampleDesc.Count = 1;
			descDepth.SampleDesc.Quality = 0;
			descDepth.Format = depthStencilTextureFormat;
			descDepth.BindFlags = D3D11_BIND_DEPTH_STENCIL;
			descDepth.CPUAccessFlags = 0;
			descDepth.Usage = D3D11_USAGE_DEFAULT;
			HRESULT createHeadlessDepthTextureReturn = device->CreateTexture2D(&descDepth, nullptr, &_depthStencilTexture);
			if (FAILED(createHeadlessDepthTextureReturn))
			{
				_log->Error("CreateTexture2D failed for headless depth buffer with error code {0}", createHeadlessDepthTextureReturn);
				return false;
			}

			D3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDescription = {};
			depthStencilViewDescription.Format = descDepth.Format;
			depthStencilViewDescription.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
			if (FAILED(device->CreateDepthStencilView(_depthStencilTexture.Get(), &depthStencilViewDescription, &_depthStencilView)))
			{
				_log->Error("CreateDepthStencilView failed");
				return false;
			}
		}

		_sampleCount = ITextureBuffer::SampleCount::SampleCount1;
		return true;
	}

	// Build our set of swap chain flags
	if (!_checkedFeatureAvailability)
	{
		Microsoft::WRL::ComPtr<IDXGIFactory5> dxgiFactory5;
		_flipDiscardModePresent = SUCCEEDED(dxgiFactory->QueryInterface(IID_PPV_ARGS(&dxgiFactory5)));
		_tearingFeaturePresent = _renderer->IsFeaturePresent(DXGI_FEATURE_PRESENT_ALLOW_TEARING);
		_checkedFeatureAvailability = true;
	}
	bool allowTearing = _tearingFeaturePresent && ((_drawState.windowBindingFlags & WindowBindingFlags::AllowTearing) != WindowBindingFlags::None);
	UINT swapChainFlags = (allowTearing ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0);

	// Create or resize our swap chain
	if (resizeExistingSwapChain)
	{
		// Remove any bound render target and depth views
		_renderTargetViewsFlatArray.clear();
		_renderTargetViews.clear();
		_backBuffer.Reset();
		_depthStencilView.Reset();
		_depthStencilTexture.Reset();

		// Attempt to resize the swap chain
		HRESULT resizeBuffersResult = _swapChain->ResizeBuffers(0, windowSizeX, windowSizeY, DXGI_FORMAT_UNKNOWN, swapChainFlags);
		if (FAILED(resizeBuffersResult))
		{
			_log->Error("ResizeBuffers failed: {0}", resizeBuffersResult);
			return false;
		}
	}
	else
	{
		// Create a new swap chain. Note that we currently require DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL as a minimum, as we
		// currently assume DXGI 1.2 support, which requires Windows 8+. If we wanted to support Windows 7, we would
		// need to fall back to DXGI_SWAP_EFFECT_DISCARD, but this would involve other changes as Direct3D 11.1 is only
		// partially supported on Windows 7.
		DXGI_SWAP_CHAIN_DESC1 swapChainDescription = {};
		swapChainDescription.Width = windowSizeX;
		swapChainDescription.Height = windowSizeY;
		swapChainDescription.Format = colorTextureFormat;
		swapChainDescription.SampleDesc.Count = 1;
		swapChainDescription.SampleDesc.Quality = 0;
		swapChainDescription.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swapChainDescription.BufferCount = 2;
		swapChainDescription.SwapEffect = (_flipDiscardModePresent ? DXGI_SWAP_EFFECT_FLIP_DISCARD : DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL);
		swapChainDescription.Flags = swapChainFlags;
		HRESULT createSwapChainForHwndReturn = dxgiFactory->CreateSwapChainForHwnd(device, nativeWindowInfo->windowHandle, &swapChainDescription, nullptr, nullptr, &_swapChain);
		if (FAILED(createSwapChainForHwndReturn))
		{
			_log->Error("CreateSwapChainForHwnd failed with error code {0}", createSwapChainForHwndReturn);
			return false;
		}
	}

	// Create a render target view for the framebuffer
	HRESULT getWindowBackBufferReturn = _swapChain->GetBuffer(0, IID_PPV_ARGS(&_backBuffer));
	if (FAILED(getWindowBackBufferReturn))
	{
		_log->Error("GetBuffer failed with error code {0}", getWindowBackBufferReturn);
		return false;
	}
	ComPtr<ID3D11RenderTargetView> renderTargetView;
	if (FAILED(device->CreateRenderTargetView(_backBuffer.Get(), nullptr, &renderTargetView)))
	{
		_log->Error("CreateRenderTargetView failed");
		return false;
	}
	_renderTargetViewsFlatArray.push_back(renderTargetView.Get());
	_renderTargetViews.push_back(std::move(renderTargetView));

	// Clear any existing depth stencil texture
	_depthStencilTexture.Reset();
	_depthStencilView.Reset();

	// Create a depth stencil texture if required
	if (hasDepthTexture)
	{
		// Create a depth stencil texture for the framebuffer
		D3D11_TEXTURE2D_DESC descDepth = {};
		descDepth.Width = windowSizeX;
		descDepth.Height = windowSizeY;
		descDepth.MipLevels = 1;
		descDepth.ArraySize = 1;
		descDepth.SampleDesc.Count = 1;
		descDepth.SampleDesc.Quality = 0;
		descDepth.Format = depthStencilTextureFormat;
		descDepth.BindFlags = D3D11_BIND_DEPTH_STENCIL;
		descDepth.CPUAccessFlags = 0;
		descDepth.Usage = D3D11_USAGE_DEFAULT;
		HRESULT createWindowDepthTextureReturn = device->CreateTexture2D(&descDepth, nullptr, &_depthStencilTexture);
		if (FAILED(createWindowDepthTextureReturn))
		{
			_log->Error("CreateTexture2D failed for window depth buffer with error code {0}", createWindowDepthTextureReturn);
			return false;
		}

		// Create a depth stencil view
		D3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDescription = {};
		depthStencilViewDescription.Format = descDepth.Format;
		depthStencilViewDescription.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
		if (FAILED(device->CreateDepthStencilView(_depthStencilTexture.Get(), &depthStencilViewDescription, &_depthStencilView)))
		{
			_log->Error("CreateDepthStencilView failed");
			return false;
		}
	}

	// Record the framebuffer sample count
	_sampleCount = ITextureBuffer::SampleCount::SampleCount1;
	return true;
}

//----------------------------------------------------------------------------------------
bool Direct3DFrameBuffer::CreateNativeObjectsForTextureTarget(ID3D11Device1* device)
{
	// Reset our depth/stencil clear flags
	_depthStencilClearFlags = {};

	// Iterate our bound texture resources and create views of each one. We also calculate the framebuffer sample count
	// here.
	bool latchedInitialSampleCount = false;
	ITextureBuffer::SampleCount framebufferSampleCount = ITextureBuffer::SampleCount::SampleCount1;
	bool foundDepthStencilTarget = false;
	for (const BoundTextureInfo& textureInfo : _drawState.boundTextures)
	{
		// Obtain the sample count for this texture, and ensure it is compatible with other bound textures.
		auto* texture = textureInfo.texture;
		auto textureSampleCount = texture->GetSampleCount();
		if (!latchedInitialSampleCount)
		{
			framebufferSampleCount = textureSampleCount;
			latchedInitialSampleCount = true;
		}
		else if (framebufferSampleCount != textureSampleCount)
		{
			_log->Error("Mismatched sample counts detected for framebuffer texture bindings. All framebuffer textures must share the same sample count to be combined into a framebuffer.");
			return false;
		}

		if (textureInfo.type != AttachmentType::Color)
		{
			// Set the depth/stencil buffer clear flags
			if (textureInfo.type == IFrameBuffer::AttachmentType::Depth)
			{
				_depthStencilClearFlags = D3D11_CLEAR_FLAG((unsigned int)_depthStencilClearFlags | (unsigned int)D3D11_CLEAR_DEPTH);
			}
			else if (textureInfo.type == IFrameBuffer::AttachmentType::Stencil)
			{
				_depthStencilClearFlags = D3D11_CLEAR_FLAG((unsigned int)_depthStencilClearFlags | (unsigned int)D3D11_CLEAR_STENCIL);
			}

			if (!foundDepthStencilTarget)
			{
				// Create a depth/stencil view
				D3D11_DEPTH_STENCIL_VIEW_DESC
				depthStencilViewDescription = {};
				depthStencilViewDescription.Format = textureInfo.texture->GetNativeTextureFormat();
				depthStencilViewDescription.ViewDimension = (textureSampleCount != ITextureBuffer::SampleCount::SampleCount1 ? D3D11_DSV_DIMENSION_TEXTURE2DMS : D3D11_DSV_DIMENSION_TEXTURE2D);
				depthStencilViewDescription.Texture2D.MipSlice = 0;
				if (FAILED(device->CreateDepthStencilView(textureInfo.texture->GetTexture(), &depthStencilViewDescription, &_depthStencilView)))
				{
					_log->Error("CreateDepthStencilView failed");
					continue;
				}
				foundDepthStencilTarget = true;
			}
		}
		else
		{
			// Create a render target view
			ComPtr<ID3D11RenderTargetView> renderTargetView;
			if (FAILED(device->CreateRenderTargetView(textureInfo.texture->GetTexture(), nullptr, &renderTargetView)))
			{
				_log->Error("CreateRenderTargetView failed for color buffer with index {0}", textureInfo.index);
				continue;
			}

			// Add this render target view to the list of render target views
			if (textureInfo.index >= _renderTargetViews.size())
			{
				_renderTargetViewsFlatArray.resize(textureInfo.index + 1, nullptr);
				_renderTargetViews.resize(textureInfo.index + 1, nullptr);
			}
			_renderTargetViewsFlatArray[textureInfo.index] = renderTargetView.Get();
			_renderTargetViews[textureInfo.index] = std::move(renderTargetView);
		}
	}

	// Record the calculated framebuffer sample count
	_sampleCount = framebufferSampleCount;
	return true;
}

//----------------------------------------------------------------------------------------
void Direct3DFrameBuffer::DeleteNativeObjects()
{
	_stagingTexturesForCapture.clear();
	_renderTargetViewsFlatArray.clear();
	_renderTargetViews.clear();
	_backBuffer.Reset();
	_depthStencilView.Reset();
	_depthStencilTexture.Reset();
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
void Direct3DFrameBuffer::ResolveMultiSamplingAttachmentToTexture(ID3D11DeviceContext1* context, AttachmentType type, size_t index, size_t resolveIndex)
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

	// Perform the texture resolve operation
	context->ResolveSubresource(resolveTexture->GetTexture(), 0, sourceTexture->GetTexture(), 0, resolveTexture->GetNativeTextureFormat());
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
