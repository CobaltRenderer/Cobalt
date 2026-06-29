// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "../UnitTestBase.h"
#include <array>
#include <cmath>
namespace cobalt::graphics::testing {
using namespace cobalt::graphics;

namespace {

// Define our shader programs
const std::string VertexShader = R"(
struct VSInput {
    float4 position : position;
};

float4 main(VSInput IN) : SV_POSITION
{
    return IN.position;
}
)";
const std::string FragmentShader = R"(
uniform float4 drawColor;

float4 main() : SV_TARGET0
{
    return drawColor;
}
)";

} // namespace

DEFINE_UNIT_TEST_WITH_BASE("Framebuffer/FramebufferOutputLifecycle", UnitTestBase)
{
	// Retrieve our important objects
	auto& renderer = session.Renderer();
	auto& uiThread = session.UIThread();
	auto& testWindowInfo = *session.TestWindowPlatformInfo();
	const V2UInt32 frameBufferSize = session.TestWindowSize();

	// Use deliberately asymmetrical capture regions so swapped X/Y size or offset values are visible in the results.
	const V2UInt32 captureOffset((frameBufferSize.X() * 3) / 16, (frameBufferSize.Y() * 5) / 32);
	const V2UInt32 captureSize((frameBufferSize.X() * 19) / 32, (frameBufferSize.Y() * 17) / 32);
	const V2UInt32 subCaptureOffset(captureSize.X() / 5, captureSize.Y() / 7);
	const V2UInt32 subCaptureSize((captureSize.X() * 9) / 20, (captureSize.Y() * 11) / 20);
	const V2UInt32 clippedSubCaptureOffset((captureSize.X() * 3) / 5, (captureSize.Y() * 2) / 3);
	const V2UInt32 clippedSubCaptureSize(captureSize.X() / 3, (captureSize.Y() * 5) / 8);
	const V2UInt32 clippedSubCaptureExpectedSize(clippedSubCaptureSize.X(), captureSize.Y() - clippedSubCaptureOffset.Y());
	const V2UInt32 xClippedSubCaptureOffset((captureSize.X() * 7) / 8, captureSize.Y() / 4);
	const V2UInt32 xClippedSubCaptureSize(captureSize.X() / 3, captureSize.Y() / 5);
	const V2UInt32 xClippedSubCaptureExpectedSize(captureSize.X() - xClippedSubCaptureOffset.X(), xClippedSubCaptureSize.Y());
	const V2UInt32 outOfBoundsCaptureOffsetX(captureSize.X(), subCaptureOffset.Y());
	const V2UInt32 outOfBoundsCaptureOffsetY(subCaptureOffset.X(), captureSize.Y());

	// Create a window-bound framebuffer so output capture exercises the same path as the user-facing back buffer.
	auto frameBuffer = renderer.CreateFrameBuffer();
	frameBuffer->DefineViewportRegion(V2UInt32(0, 0), frameBufferSize);
	REQUIRE(uiThread.InvokeSync([&] { return frameBuffer->BindWindow(testWindowInfo, IFrameBuffer::WindowDepthStencilMode::None); }));

	// Readback from an output target that has never captured a frame should fail rather than returning stale data.
	auto uncapturedFrameBufferCapture = renderer.CreateFrameBufferOutput();
	V4UInt8 uncapturedPixel;
	REQUIRE(!uncapturedFrameBufferCapture->ReadBufferData(&uncapturedPixel, 1));

	// Create and compile our shader program
	auto shaderProgram = renderer.CreateShaderProgram();
	REQUIRE(shaderProgram->LoadShaderStage(IShaderProgram::ShaderStage::Vertex, ShaderSourceInfoHLSL(VertexShader)));
	REQUIRE(shaderProgram->LoadShaderStage(IShaderProgram::ShaderStage::Fragment, ShaderSourceInfoHLSL(FragmentShader)));
	REQUIRE(shaderProgram->CompileProgram());

	// Retrieve the shader attribute and state IDs used by the renderable node.
	auto positionAttributeId = shaderProgram->GetVertexAttributeId("position");
	auto drawColorStateId = shaderProgram->GetStateValueId("drawColor");

	// Create our vertex buffer and populate it with the standard downward-pointing triangle.
	std::vector<V4Float32> positionVertexData;
	Geometry().CreateRGBTrianglePositions(0.5f, positionVertexData);
	VertexAttribute<V4Float32> vertexAttributePosition(positionVertexData.size(), IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadNever, IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadOften);
	auto vertexBuffer = renderer.CreateVertexBuffer();
	REQUIRE(vertexBuffer->BindVertexAttribute(vertexAttributePosition));
	REQUIRE(vertexAttributePosition.SetInitialData(positionVertexData));
	REQUIRE(vertexBuffer->AllocateMemory());

	// Create a renderable node whose color can be changed between captures.
	auto renderableNode = renderer.CreateRenderableNode();
	REQUIRE(renderableNode->BindVertexAttribute(vertexAttributePosition, positionAttributeId));
	REQUIRE(renderableNode->SetPrimitiveMode(IRenderableNode::PrimitiveMode::Triangles));
	renderableNode->SetStateValue(drawColorStateId, V4Float32(1.0f, 0.0f, 0.0f, 1.0f));

	// Create the render tree used by all capture lifecycle checks.
	auto groupNode = renderer.CreateStateGroupNode();
	groupNode->SetPolygonCullMode(IStateGroupNode::PolygonCullMode::None);
	groupNode->AddChildNode(renderableNode.get());

	// Create our program node
	auto programNode = renderer.CreateProgramNode();
	REQUIRE(programNode->BindShaderProgram(shaderProgram.get()));
	programNode->AddChildNode(groupNode.get());

	auto renderPassNode = renderer.CreateRenderPassNode();
	renderPassNode->BindFrameBuffer(frameBuffer.get());
	renderPassNode->SetAttachmentClearData(IFrameBuffer::AttachmentType::Color, 0, V4Float32(0.0f, 0.0f, 0.0f, 1.0f));
	renderPassNode->AddChildNode(programNode.get());
	renderer.SetRenderPasses(&renderPassNode, 1);

	// Capture the full frame first so the uncropped triangle output is visible in the test results.
	auto baselineFrameBufferCapture = renderer.CreateFrameBufferOutput();
	baselineFrameBufferCapture->SetDetachAfterCapture(true);
	frameBuffer->AddOutputCaptureTarget(baselineFrameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
	DrawOneFrame();
	session.AddTestImageResult("RedTriangleBaseline", "A full-frame red triangle on a black-cleared background with no capture cropping.", std::move(baselineFrameBufferCapture), IImageDiff::Algorithm::AllOfThem, 0.95);

	// Verify detach-after-capture removes a target after its first captured frame.
	auto detachedFrameBufferCapture = renderer.CreateFrameBufferOutput();
	detachedFrameBufferCapture->SetDetachAfterCapture(true);
	detachedFrameBufferCapture->SetFrameBufferCaptureRegion(captureOffset, captureSize);
	frameBuffer->AddOutputCaptureTarget(detachedFrameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
	DrawOneFrame();
	REQUIRE(detachedFrameBufferCapture->HasCapturedOutput());
	REQUIRE(detachedFrameBufferCapture->GetImageDimensions() == captureSize);
	detachedFrameBufferCapture->ClearCapturedOutput();
	DrawOneFrame();
	REQUIRE(!detachedFrameBufferCapture->HasCapturedOutput());

	// Capture a cropped region and keep one target attached for subsequent frames.
	auto frameBufferCapture = renderer.CreateFrameBufferOutput();
	frameBufferCapture->SetDetachAfterCapture(false);
	frameBufferCapture->SetFrameBufferCaptureRegion(captureOffset, captureSize);
	REQUIRE(!frameBufferCapture->HasCapturedOutput());
	frameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);

	// Capture the same cropped red image into a detached target for reference-image comparison.
	auto redFrameBufferCapture = renderer.CreateFrameBufferOutput();
	redFrameBufferCapture->SetDetachAfterCapture(true);
	redFrameBufferCapture->SetFrameBufferCaptureRegion(captureOffset, captureSize);
	frameBuffer->AddOutputCaptureTarget(redFrameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
	DrawOneFrame();

	// Verify captured-output metadata, including full, clipped, and out-of-bounds crop calculations.
	REQUIRE(frameBufferCapture->HasCapturedOutput());
	REQUIRE(frameBufferCapture->GetImageDimensions() == captureSize);
	REQUIRE(frameBufferCapture->GetCroppedImageDimensions(V2UInt32(0, 0), V2UInt32(0, 0)) == captureSize);
	REQUIRE(frameBufferCapture->GetCroppedImageDimensions(subCaptureOffset, subCaptureSize) == subCaptureSize);
	REQUIRE(frameBufferCapture->GetCroppedImageDimensions(clippedSubCaptureOffset, clippedSubCaptureSize) == clippedSubCaptureExpectedSize);
	REQUIRE(frameBufferCapture->GetCroppedImageDimensions(xClippedSubCaptureOffset, xClippedSubCaptureSize) == xClippedSubCaptureExpectedSize);
	REQUIRE(frameBufferCapture->GetCroppedImageDimensions(outOfBoundsCaptureOffsetX, subCaptureSize) == V2UInt32(0, 0));
	REQUIRE(frameBufferCapture->GetCroppedImageDimensions(outOfBoundsCaptureOffsetY, subCaptureSize) == V2UInt32(0, 0));
	REQUIRE(ITextureBuffer::ElementCountPerPixelFromFormat(frameBufferCapture->GetOptimalImageFormat()) > 0);
	REQUIRE(ITextureBuffer::ByteSizePerElementFromFormat(frameBufferCapture->GetOptimalDataFormat()) > 0);

	// Read the full cropped output so later subregion reads can be compared against the same captured image.
	std::vector<V4UInt8> capturedData;
	REQUIRE(frameBufferCapture->ReadBufferData(capturedData));
	REQUIRE(capturedData.size() == ((size_t)captureSize.X() * (size_t)captureSize.Y()));
	session.AddTestImageResult("CroppedRedCapture", "A cropped capture from the full-frame red triangle output.", std::move(redFrameBufferCapture), IImageDiff::Algorithm::AllOfThem, 0.95);

	// Verify typed and explicit-format readback conversions from the captured output.
	std::vector<V4Float32> capturedFloatData;
	REQUIRE(frameBufferCapture->ReadBufferData(capturedFloatData));
	REQUIRE(capturedFloatData.size() == capturedData.size());
	std::vector<V4UInt8> capturedBgraData(capturedData.size());
	REQUIRE(frameBufferCapture->ReadBufferData(static_cast<void*>(capturedBgraData.data()), capturedBgraData.size() * sizeof(V4UInt8), ITextureBuffer::SourceImageFormat::BGRA, ITextureBuffer::SourceDataFormat::UNorm8));
	bool locatedConvertedRedPixel = false;
	for (size_t i = 0; i < capturedData.size(); ++i)
	{
		if ((capturedData[i].X() > 200) && (capturedData[i].Y() < 64) && (capturedData[i].Z() < 64) && (capturedData[i].W() > 200))
		{
			REQUIRE(capturedBgraData[i].X() < 64);
			REQUIRE(capturedBgraData[i].Y() < 64);
			REQUIRE(capturedBgraData[i].Z() > 200);
			REQUIRE(capturedBgraData[i].W() > 200);
			locatedConvertedRedPixel = true;
			break;
		}
	}
	REQUIRE(locatedConvertedRedPixel);
	std::vector<V4UInt8> tooSmallReadBuffer(1);
	REQUIRE(!frameBufferCapture->ReadBufferData(static_cast<void*>(tooSmallReadBuffer.data()), tooSmallReadBuffer.size() * sizeof(V4UInt8), ITextureBuffer::SourceImageFormat::RGBA, ITextureBuffer::SourceDataFormat::UNorm8, V2UInt32(0, 0), captureSize));
	std::array<uint8_t, 16> invalidReadbackFormatBuffer{};
	REQUIRE(!frameBufferCapture->ReadBufferData(static_cast<void*>(invalidReadbackFormatBuffer.data()), invalidReadbackFormatBuffer.size(), ITextureBuffer::SourceImageFormat::RGBA, ITextureBuffer::SourceDataFormat::DXT1));

	// Verify direct subregion reads return the same pixels as indexing into the full captured output.
	auto verifySubCaptureMatchesCapturedData = [&](const V2UInt32& readOffset, const V2UInt32& requestedReadSize) {
		const V2UInt32 actualReadSize = frameBufferCapture->GetCroppedImageDimensions(readOffset, requestedReadSize);
		std::vector<V4UInt8> rawReadData((size_t)actualReadSize.X() * (size_t)actualReadSize.Y());
		REQUIRE(frameBufferCapture->ReadBufferData(rawReadData.data(), rawReadData.size(), readOffset, requestedReadSize));
		for (uint32_t y = 0; y < actualReadSize.Y(); ++y)
		{
			for (uint32_t x = 0; x < actualReadSize.X(); ++x)
			{
				const size_t sourceIndex = ((size_t)y + (size_t)readOffset.Y()) * (size_t)captureSize.X() + ((size_t)x + (size_t)readOffset.X());
				const size_t targetIndex = (size_t)y * (size_t)actualReadSize.X() + (size_t)x;
				REQUIRE(rawReadData[targetIndex] == capturedData[sourceIndex]);
			}
		}
	};
	verifySubCaptureMatchesCapturedData(subCaptureOffset, subCaptureSize);
	verifySubCaptureMatchesCapturedData(clippedSubCaptureOffset, clippedSubCaptureSize);
	verifySubCaptureMatchesCapturedData(xClippedSubCaptureOffset, xClippedSubCaptureSize);

	// Out-of-bounds reads should produce an empty region rather than stale data or an error.
	std::vector<V4UInt8> outOfBoundsReadData;
	REQUIRE(frameBufferCapture->ReadBufferData(outOfBoundsReadData, outOfBoundsCaptureOffsetX, subCaptureSize));
	REQUIRE(outOfBoundsReadData.empty());
	REQUIRE(frameBufferCapture->ReadBufferData(outOfBoundsReadData, outOfBoundsCaptureOffsetY, subCaptureSize));
	REQUIRE(outOfBoundsReadData.empty());

	// Clearing captured output should reset the captured state without detaching the target.
	frameBufferCapture->ClearCapturedOutput();
	REQUIRE(!frameBufferCapture->HasCapturedOutput());
	renderableNode->SetStateValue(drawColorStateId, V4Float32(0.0f, 1.0f, 0.0f, 1.0f));

	// Capture the green frame into a detached image target while the persistent target recaptures the same frame.
	auto greenFrameBufferCapture = renderer.CreateFrameBufferOutput();
	greenFrameBufferCapture->SetDetachAfterCapture(true);
	greenFrameBufferCapture->SetFrameBufferCaptureRegion(captureOffset, captureSize);
	frameBuffer->AddOutputCaptureTarget(greenFrameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
	DrawOneFrame();
	REQUIRE(frameBufferCapture->HasCapturedOutput());
	REQUIRE(frameBufferCapture->ReadBufferData(capturedData));
	REQUIRE(capturedData.size() == ((size_t)captureSize.X() * (size_t)captureSize.Y()));
	session.AddTestImageResult("CroppedGreenCapture", "A cropped capture from the full-frame green triangle output after clearing the persistent capture output.", std::move(greenFrameBufferCapture), IImageDiff::Algorithm::AllOfThem, 0.95);

	// Removing the persistent capture target should prevent further captures even when more frames are drawn.
	frameBuffer->RemoveOutputCaptureTarget(frameBufferCapture.get());
	frameBufferCapture->ClearCapturedOutput();
	renderPassNode->SetAttachmentClearData(IFrameBuffer::AttachmentType::Color, 0, V4Float32(0.0f, 0.0f, 1.0f, 1.0f));
	DrawOneFrame();
	REQUIRE(!frameBufferCapture->HasCapturedOutput());
	session.AddTestSuccess("FrameBufferOutputLifecycle", "Framebuffer output capture, uncropped baseline output, detach-after-capture, clipped and out-of-bounds readback, persistent capture targets, clearing, color switching, and removal behaved as expected.");

	// Remove all our defined render passes
	renderer.RemoveAllRenderPasses();
	renderer.WaitForDeferredDeletionComplete();

	// Capture offscreen depth and stencil attachments directly so non-color readback is covered by this lifecycle test.
	{
		const V2UInt32 offscreenSize(32, 24);
		auto colorTexture = renderer.CreateTextureBuffer2D();
		colorTexture->SetUsageFlags(ITextureBuffer::UsageFlags::FrameBufferOutput);
		colorTexture->SetTextureFormat(ITextureBuffer::ImageFormat::RGBA, ITextureBuffer::DataFormat::UNorm8);
		colorTexture->SetTextureDimensions(offscreenSize);
		REQUIRE(colorTexture->AllocateMemory());

		auto depthStencilTexture = renderer.CreateTextureBuffer2D();
		depthStencilTexture->SetUsageFlags(ITextureBuffer::UsageFlags::FrameBufferOutput);
		depthStencilTexture->SetTextureFormat(ITextureBuffer::ImageFormat::DepthAndStencil, ITextureBuffer::DataFormat::DepthUNorm24StencilUInt8);
		depthStencilTexture->SetTextureDimensions(offscreenSize);
		REQUIRE(depthStencilTexture->AllocateMemory());

		auto offscreenFrameBuffer = renderer.CreateFrameBuffer();
		offscreenFrameBuffer->DefineViewportRegion(V2UInt32(0, 0), offscreenSize);
		REQUIRE(offscreenFrameBuffer->BindTexture(colorTexture.get(), IFrameBuffer::AttachmentType::Color));
		REQUIRE(offscreenFrameBuffer->BindTexture(depthStencilTexture.get(), IFrameBuffer::AttachmentType::Depth));
		REQUIRE(offscreenFrameBuffer->BindTexture(depthStencilTexture.get(), IFrameBuffer::AttachmentType::Stencil));

		auto depthStencilRenderPassNode = renderer.CreateRenderPassNode();
		depthStencilRenderPassNode->BindFrameBuffer(offscreenFrameBuffer.get());
		depthStencilRenderPassNode->SetAttachmentClearData(IFrameBuffer::AttachmentType::Color, 0, V4Float32(0.0f, 0.0f, 0.0f, 1.0f));
		depthStencilRenderPassNode->SetAttachmentClearData(IFrameBuffer::AttachmentType::Depth, 0, V4Float32(0.25f, 0.0f, 0.0f, 0.0f));
		depthStencilRenderPassNode->SetAttachmentClearData(IFrameBuffer::AttachmentType::Stencil, 0, V4UInt32(64, 0, 0, 0));
		renderer.SetRenderPasses(&depthStencilRenderPassNode, 1);

		auto depthCapture = renderer.CreateFrameBufferOutput();
		depthCapture->SetDetachAfterCapture(true);
		offscreenFrameBuffer->AddOutputCaptureTarget(depthCapture.get(), IFrameBuffer::AttachmentType::Depth);
		auto stencilCapture = renderer.CreateFrameBufferOutput();
		stencilCapture->SetDetachAfterCapture(true);
		offscreenFrameBuffer->AddOutputCaptureTarget(stencilCapture.get(), IFrameBuffer::AttachmentType::Stencil);
		DrawOneFrame();

		REQUIRE(depthCapture->HasCapturedOutput());
		REQUIRE(depthCapture->GetImageDimensions() == offscreenSize);
		REQUIRE(ITextureBuffer::ElementCountPerPixelFromFormat(depthCapture->GetOptimalImageFormat()) > 0);
		REQUIRE(ITextureBuffer::ByteSizePerElementFromFormat(depthCapture->GetOptimalDataFormat()) > 0);
		std::vector<V1Float32> depthData;
		REQUIRE(depthCapture->ReadBufferData(depthData));
		REQUIRE(depthData.size() == ((size_t)offscreenSize.X() * (size_t)offscreenSize.Y()));
		for (const auto& sample : depthData)
		{
			REQUIRE(std::abs(sample.X() - 0.25f) <= 0.01f);
		}

		REQUIRE(stencilCapture->HasCapturedOutput());
		REQUIRE(stencilCapture->GetImageDimensions() == offscreenSize);
		REQUIRE(ITextureBuffer::ElementCountPerPixelFromFormat(stencilCapture->GetOptimalImageFormat()) > 0);
		REQUIRE(ITextureBuffer::ByteSizePerElementFromFormat(stencilCapture->GetOptimalDataFormat()) > 0);
		std::vector<V1UInt8> stencilData;
		REQUIRE(stencilCapture->ReadBufferData(stencilData));
		REQUIRE(stencilData.size() == ((size_t)offscreenSize.X() * (size_t)offscreenSize.Y()));
		for (const auto& sample : stencilData)
		{
			REQUIRE(sample.X() == 64);
		}
		session.AddTestSuccess("DepthStencilOutputReadback", "Framebuffer output capture read back cleared depth and stencil attachment data with the expected values.");
	}
	renderer.RemoveAllRenderPasses();
	return true;
}

} // namespace cobalt::graphics::testing
