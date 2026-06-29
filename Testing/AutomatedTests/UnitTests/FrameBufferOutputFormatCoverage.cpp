// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "../UnitTestBase.h"
#include <array>
#include <string>
namespace cobalt::graphics::testing {
using namespace cobalt::graphics;

namespace {

enum class ClearDataType
{
	Float,
	Int,
	UInt,
};

struct OutputFormatCase
{
	const char* name;
	ITextureBuffer::ImageFormat imageFormat;
	ITextureBuffer::DataFormat dataFormat;
	ClearDataType clearDataType;
};

bool IsFloatDataFormat(ITextureBuffer::DataFormat dataFormat)
{
	return ((dataFormat == ITextureBuffer::DataFormat::Float16) || (dataFormat == ITextureBuffer::DataFormat::Float32));
}

bool IsIntegerDataFormat(ITextureBuffer::DataFormat dataFormat)
{
	return (
	  (dataFormat == ITextureBuffer::DataFormat::Int8) ||
	  (dataFormat == ITextureBuffer::DataFormat::Int16) ||
	  (dataFormat == ITextureBuffer::DataFormat::Int32) ||
	  (dataFormat == ITextureBuffer::DataFormat::UInt8) ||
	  (dataFormat == ITextureBuffer::DataFormat::UInt16) ||
	  (dataFormat == ITextureBuffer::DataFormat::UInt32));
}

bool IsSingleChannelImageFormat(ITextureBuffer::ImageFormat imageFormat)
{
	return ((imageFormat == ITextureBuffer::ImageFormat::R) || (imageFormat == ITextureBuffer::ImageFormat::X));
}

bool IsTwoChannelImageFormat(ITextureBuffer::ImageFormat imageFormat)
{
	return ((imageFormat == ITextureBuffer::ImageFormat::RG) || (imageFormat == ITextureBuffer::ImageFormat::XY));
}

V4Float32 FloatClearDataFor(const OutputFormatCase& outputFormatCase)
{
	// Integer framebuffer captures are converted into UInt8 inspection images by the test harness, so clearing an
	// integer target through the float overload needs byte-scale values rather than normalized 0-1 colour values.
	if (IsIntegerDataFormat(outputFormatCase.dataFormat))
	{
		return V4Float32(192.0f, 80.0f, 32.0f, 255.0f);
	}

	if (IsSingleChannelImageFormat(outputFormatCase.imageFormat))
	{
		return V4Float32(0.75f, 0.0f, 0.0f, 1.0f);
	}
	if (IsTwoChannelImageFormat(outputFormatCase.imageFormat))
	{
		return V4Float32(0.75f, 0.35f, 0.0f, 1.0f);
	}
	return V4Float32(0.25f, 0.55f, 0.85f, 1.0f);
}

V4Int32 IntClearDataFor(const OutputFormatCase& outputFormatCase)
{
	if (IsFloatDataFormat(outputFormatCase.dataFormat))
	{
		return V4Int32(0, 1, 0, 1);
	}
	if (IsSingleChannelImageFormat(outputFormatCase.imageFormat))
	{
		return V4Int32(192, 0, 0, 255);
	}
	return V4Int32(192, 80, 32, 255);
}

V4UInt32 UIntClearDataFor(const OutputFormatCase& outputFormatCase)
{
	if (IsFloatDataFormat(outputFormatCase.dataFormat))
	{
		return V4UInt32(1, 0, 1, 1);
	}
	if (IsSingleChannelImageFormat(outputFormatCase.imageFormat))
	{
		return V4UInt32(192, 0, 0, 255);
	}
	return V4UInt32(64, 176, 224, 255);
}

std::string ApproximateCapturedRGBFor(const OutputFormatCase& outputFormatCase)
{
	if (outputFormatCase.clearDataType == ClearDataType::Int)
	{
		if (IsFloatDataFormat(outputFormatCase.dataFormat))
		{
			return "#00FF00";
		}
		return IsSingleChannelImageFormat(outputFormatCase.imageFormat) ? "#C00000" : "#C05020";
	}
	if (outputFormatCase.clearDataType == ClearDataType::UInt)
	{
		if (IsFloatDataFormat(outputFormatCase.dataFormat))
		{
			return "#FF00FF";
		}
		return IsSingleChannelImageFormat(outputFormatCase.imageFormat) ? "#C00000" : "#40B0E0";
	}
	if (IsIntegerDataFormat(outputFormatCase.dataFormat))
	{
		if (IsSingleChannelImageFormat(outputFormatCase.imageFormat))
		{
			return "#C00000";
		}
		if (IsTwoChannelImageFormat(outputFormatCase.imageFormat))
		{
			return "#C05000";
		}
		return "#C05020";
	}
	if (IsSingleChannelImageFormat(outputFormatCase.imageFormat))
	{
		return "#BF0000";
	}
	if (IsTwoChannelImageFormat(outputFormatCase.imageFormat))
	{
		return "#BF5900";
	}
	return "#408CD9";
}

std::string ImageDescriptionFor(const OutputFormatCase& outputFormatCase)
{
	return "Captured framebuffer attachment using the requested format and clear-data path. The image should be approximately " + ApproximateCapturedRGBFor(outputFormatCase) + ".";
}

size_t ReadSizeInBytes(ITextureBuffer::SourceImageFormat imageFormat, ITextureBuffer::SourceDataFormat dataFormat, const V2UInt32& imageSize)
{
	return (size_t)imageSize.X() * (size_t)imageSize.Y() * ITextureBuffer::ElementCountPerPixelFromFormat(imageFormat) * ITextureBuffer::ByteSizePerElementFromFormat(dataFormat);
}

} // namespace

DEFINE_UNIT_TEST_WITH_BASE("Framebuffer/FramebufferOutputFormatCoverage", UnitTestBase)
{
	// Retrieve our important objects
	auto& renderer = session.Renderer();
	auto& device = session.Device();
	const V2UInt32 imageSize = session.TestWindowSize();

	// Exercise framebuffer-output capture and readback through a range of attachment formats. Some cases deliberately
	// clear with a different data type to prove the renderer converts clear data to the attachment type before using
	// native APIs that require an exact type match. Some APIs promote unsupported channel layouts to a wider backing
	// format, so the test reads each capture using the format reported by the output object rather than assuming the
	// requested format survived unchanged.
	const std::array<OutputFormatCase, 22> outputFormatCases = {{
	  {"RFloat32", ITextureBuffer::ImageFormat::R, ITextureBuffer::DataFormat::Float32, ClearDataType::Float},
	  {"RGFloat32", ITextureBuffer::ImageFormat::RG, ITextureBuffer::DataFormat::Float32, ClearDataType::Float},
	  {"RGBFloat32", ITextureBuffer::ImageFormat::RGB, ITextureBuffer::DataFormat::Float32, ClearDataType::Float},
	  {"RGBAFloat16", ITextureBuffer::ImageFormat::RGBA, ITextureBuffer::DataFormat::Float16, ClearDataType::Float},
	  {"RGBAFloat32", ITextureBuffer::ImageFormat::RGBA, ITextureBuffer::DataFormat::Float32, ClearDataType::Float},
	  {"RGBAFloat32IntClear", ITextureBuffer::ImageFormat::RGBA, ITextureBuffer::DataFormat::Float32, ClearDataType::Int},
	  {"RGBAFloat32UIntClear", ITextureBuffer::ImageFormat::RGBA, ITextureBuffer::DataFormat::Float32, ClearDataType::UInt},
	  {"RGBAUNorm8", ITextureBuffer::ImageFormat::RGBA, ITextureBuffer::DataFormat::UNorm8, ClearDataType::Float},
	  {"RGBAUNorm16", ITextureBuffer::ImageFormat::RGBA, ITextureBuffer::DataFormat::UNorm16, ClearDataType::Float},
	  {"RGBANorm8", ITextureBuffer::ImageFormat::RGBA, ITextureBuffer::DataFormat::Norm8, ClearDataType::Float},
	  {"BGRAUNorm8", ITextureBuffer::ImageFormat::BGRA, ITextureBuffer::DataFormat::UNorm8, ClearDataType::Float},
	  {"XFloat32", ITextureBuffer::ImageFormat::X, ITextureBuffer::DataFormat::Float32, ClearDataType::Float},
	  {"XYFloat32", ITextureBuffer::ImageFormat::XY, ITextureBuffer::DataFormat::Float32, ClearDataType::Float},
	  {"XYZFloat32", ITextureBuffer::ImageFormat::XYZ, ITextureBuffer::DataFormat::Float32, ClearDataType::Float},
	  {"XYZWFloat32", ITextureBuffer::ImageFormat::XYZW, ITextureBuffer::DataFormat::Float32, ClearDataType::Float},
	  {"RInt32", ITextureBuffer::ImageFormat::R, ITextureBuffer::DataFormat::Int32, ClearDataType::Int},
	  {"RInt32FloatClear", ITextureBuffer::ImageFormat::R, ITextureBuffer::DataFormat::Int32, ClearDataType::Float},
	  {"RInt32UIntClear", ITextureBuffer::ImageFormat::R, ITextureBuffer::DataFormat::Int32, ClearDataType::UInt},
	  {"RUInt32", ITextureBuffer::ImageFormat::R, ITextureBuffer::DataFormat::UInt32, ClearDataType::UInt},
	  {"RUInt32FloatClear", ITextureBuffer::ImageFormat::R, ITextureBuffer::DataFormat::UInt32, ClearDataType::Float},
	  {"RUInt32IntClear", ITextureBuffer::ImageFormat::R, ITextureBuffer::DataFormat::UInt32, ClearDataType::Int},
	  {"RGBAUInt32", ITextureBuffer::ImageFormat::RGBA, ITextureBuffer::DataFormat::UInt32, ClearDataType::UInt},
	}};

	size_t capturedCaseCount = 0;
	for (const auto& outputFormatCase : outputFormatCases)
	{
		const std::string caseName(outputFormatCase.name);
		if (!device.IsTextureFormatSupported(outputFormatCase.imageFormat, outputFormatCase.dataFormat))
		{
			session.AddTestSkipped(outputFormatCase.name, "This framebuffer-output format case was skipped because the current device does not support the requested texture format.");
			continue;
		}

		auto texture = renderer.CreateTextureBuffer2D();
		texture->SetUsageFlags(ITextureBuffer::UsageFlags::FrameBufferOutput);
		texture->SetTextureFormat(outputFormatCase.imageFormat, outputFormatCase.dataFormat);
		texture->SetTextureDimensions(imageSize);
		if (!texture->AllocateMemory())
		{
			session.AddTestSkipped(outputFormatCase.name, "This framebuffer-output format case was skipped because the requested texture format could not be allocated as a framebuffer attachment.");
			continue;
		}

		auto frameBuffer = renderer.CreateFrameBuffer();
		frameBuffer->DefineViewportRegion(V2UInt32(0, 0), imageSize);
		if (!frameBuffer->BindTexture(texture.get(), IFrameBuffer::AttachmentType::Color))
		{
			session.AddTestSkipped(outputFormatCase.name, "This framebuffer-output format case was skipped because the requested texture format could not be bound as a framebuffer attachment.");
			continue;
		}

		auto renderPassNode = renderer.CreateRenderPassNode();
		renderPassNode->BindFrameBuffer(frameBuffer.get());
		if (outputFormatCase.clearDataType == ClearDataType::Int)
		{
			renderPassNode->SetAttachmentClearData(IFrameBuffer::AttachmentType::Color, 0, IntClearDataFor(outputFormatCase));
		}
		else if (outputFormatCase.clearDataType == ClearDataType::UInt)
		{
			renderPassNode->SetAttachmentClearData(IFrameBuffer::AttachmentType::Color, 0, UIntClearDataFor(outputFormatCase));
		}
		else
		{
			renderPassNode->SetAttachmentClearData(IFrameBuffer::AttachmentType::Color, 0, FloatClearDataFor(outputFormatCase));
		}
		renderer.SetRenderPasses(&renderPassNode, 1);

		auto frameBufferCapture = renderer.CreateFrameBufferOutput();
		frameBufferCapture->SetDetachAfterCapture(true);
		frameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
		DrawOneFrame();

		REQUIRE(frameBufferCapture->HasCapturedOutput());
		REQUIRE(frameBufferCapture->GetImageDimensions() == imageSize);
		auto optimalImageFormat = frameBufferCapture->GetOptimalImageFormat();
		auto optimalDataFormat = frameBufferCapture->GetOptimalDataFormat();
		std::vector<uint8_t> rawData(ReadSizeInBytes(optimalImageFormat, optimalDataFormat, imageSize));
		REQUIRE(frameBufferCapture->ReadBufferData(rawData.data(), rawData.size(), optimalImageFormat, optimalDataFormat));
		session.AddTestImageResult(caseName, ImageDescriptionFor(outputFormatCase), std::move(frameBufferCapture), IImageDiff::Algorithm::NaiveDiff, 0.98);

		++capturedCaseCount;
		renderer.RemoveAllRenderPasses();
	}

	if (capturedCaseCount == 0)
	{
		session.AddTestFailure("FramebufferOutputFormatCoverage", "No framebuffer-output format cases captured an image.", "A valid working renderer is expected to support at least one compatible framebuffer-output format.");
		return false;
	}
	return true;
}

} // namespace cobalt::graphics::testing
