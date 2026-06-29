// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include "Deleter.h"
#include "SuccessToken.h"
#include "VectorTypes.h"
#include <memory>
namespace cobalt { namespace graphics {
class ITextureBuffer2D;
class IFrameBufferOutput;

class IFrameBuffer
{
public:
	// Enumerations
	enum class AttachmentType
	{
		Color,
		Depth,
		Stencil,
	};
	enum class WindowDepthStencilMode
	{
		None,
		DepthUNorm16,
		DepthUNorm24,
		DepthUNorm24StencilUInt8,
		DepthFloat32,
		DepthFloat32StencilUInt8,
	};
	enum class WindowColorSpaceMode
	{
		Default,
		//##TODO## Consider support for color space options. Note that we don't want to expose features to pick
		// arbitrary formats for the color buffer, as our options are heavily restricted by the OS and change over time,
		// and the OS also makes assumptions about color space based on what format is selected. There are really only
		// four color buffer formats that are supported by the latest presentation modes on Windows currently, and two of
		// them are for HDR rendering. The remaining two are RGBA/BGRA 8-bit formats, and the renderer should be left to
		// pick BGRA where appropriate without the application needing to make the decision. For a window buffer all
		// we care about is color reproduction anyway, so it makes more sense to pick options based on color space, so
		// that the renderers can take the extra steps that are required to properly support the requested mode. The
		// current setting of "Default" is deliberately vague. Although right now it will create a color buffer with 8
		// bits per component, that may change in the future if 12-bit panels became the norm, and we wanted to bump up
		// to HDR level quality by default if sensible. Apart from the default setting, we should consider adding options
		// to explicitly select a color space like sRGB, AdobeRGB, BT709, BT2020 etc. Knowing the target color space
		// allows us to properly setup all parts of the swap chain and presentation model to correctly generate and/or
		// convert color values into the target color space, and pick an appropriate data format to match. See the
		// following article for some useful information:
		// https://docs.microsoft.com/en-us/windows/win32/direct3darticles/high-dynamic-range
	};
	enum class WindowBindingFlags : uint64_t
	{
		None = 0,
		LimitSwapToVSync = 0x00000001,
		AllowTearing = 0x00000002,
	};

	// Structures
	struct WindowInfoBase
	{
		enum class WindowType
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
		WindowType windowType = {};

	protected:
		WindowInfoBase() = default;
	};

	// Typedefs
	typedef std::unique_ptr<IFrameBuffer, Deleter<IFrameBuffer>> unique_ptr;

public:
	// Initialization methods
	virtual void Delete() = 0;

	// Binding methods
	virtual SuccessToken BindTexture(ITextureBuffer2D* texture, AttachmentType type, size_t index = 0) = 0;
	virtual void UnbindTexture(AttachmentType type, size_t index = 0) = 0;
	virtual SuccessToken BindMultiSamplingResolveTexture(ITextureBuffer2D* texture, AttachmentType type, size_t index = 0) = 0;
	virtual void UnbindMultiSamplingResolveTexture(AttachmentType type, size_t index = 0) = 0;
	virtual SuccessToken BindWindow(const WindowInfoBase& windowInfo, WindowDepthStencilMode depthStencilMode, WindowColorSpaceMode colorSpaceMode = WindowColorSpaceMode::Default, WindowBindingFlags bindingFlags = WindowBindingFlags::None) = 0;
	virtual SuccessToken NotifyWindowResized(const V2UInt32& windowSizeInPixels) = 0;

	// Viewport methods
	virtual void DefineViewportRegion(const V2UInt32& startPos, const V2UInt32& size) = 0;
	virtual void DefineScissorRegion(const V2UInt32& startPos, const V2UInt32& size) = 0;
	virtual void RemoveScissorRegion() = 0;

	// Output capture methods
	virtual void AddOutputCaptureTarget(IFrameBufferOutput* captureTarget, AttachmentType type, size_t index = 0) = 0;
	virtual void RemoveOutputCaptureTarget(IFrameBufferOutput* captureTarget) = 0;

protected:
	// Constructors
	~IFrameBuffer() = default;
};

}} // namespace cobalt::graphics
#include "IFrameBuffer.inl"
