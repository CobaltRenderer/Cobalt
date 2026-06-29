// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "../UnitTestBase.h"
#include <array>
namespace cobalt::graphics::testing {
using namespace cobalt::graphics;

namespace {

const char* WindowDepthStencilModeName(IFrameBuffer::WindowDepthStencilMode mode)
{
	switch (mode)
	{
	case IFrameBuffer::WindowDepthStencilMode::DepthUNorm24StencilUInt8:
		return "DepthUNorm24StencilUInt8";
	case IFrameBuffer::WindowDepthStencilMode::DepthFloat32StencilUInt8:
		return "DepthFloat32StencilUInt8";
	default:
		return "Unknown";
	}
}

} // namespace

const std::string VertexShader = R"(
struct VSInput {
    float4 position : position;
};

struct VSOutput {
    float4 position : SV_POSITION;
};

VSOutput main(VSInput IN)
{
    VSOutput OUT;
    OUT.position = IN.position;
    return OUT;
}
)";
const std::string FragmentShader = R"(
float4 main() : SV_TARGET0
{
    return float4(1.0f, 1.0f, 1.0f, 1.0f);
}
)";

DEFINE_UNIT_TEST_WITH_BASE("Framebuffer/FramebufferClearStencil", UnitTestBase)
{
	// Retrieve our important objects
	auto& renderer = session.Renderer();
	auto& uiThread = session.UIThread();
	auto& testWindowInfo = *session.TestWindowPlatformInfo();

	// Create and compile our shader program
	auto shaderProgram = renderer.CreateShaderProgram();
	REQUIRE(shaderProgram->LoadShaderStage(IShaderProgram::ShaderStage::Vertex, ShaderSourceInfoHLSL(VertexShader)));
	REQUIRE(shaderProgram->LoadShaderStage(IShaderProgram::ShaderStage::Fragment, ShaderSourceInfoHLSL(FragmentShader)));
	REQUIRE(shaderProgram->CompileProgram());

	// Retrieve our shader attribute IDs
	auto positionAttributeId = shaderProgram->GetVertexAttributeId("position");

	// Create the triangle geometry used to stamp distinct stencil values into three quadrants
	std::vector<V4Float32> upperLeftVertexData;
	std::vector<V4Float32> upperRightVertexData;
	std::vector<V4Float32> lowerLeftVertexData;
	Geometry().CreateUpperLeftTriangle(0.5f, upperLeftVertexData);
	Geometry().CreateUpperRightTriangle(0.5f, upperRightVertexData);
	Geometry().CreateLowerLeftTriangle(0.5f, lowerLeftVertexData);

	// Create our vertex attributes and populate them with geometry data
	VertexAttribute<V4Float32> upperLeftVertexAttribute(upperLeftVertexData.size(), IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadNever, IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadOften);
	VertexAttribute<V4Float32> upperRightVertexAttribute(upperRightVertexData.size(), IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadNever, IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadOften);
	VertexAttribute<V4Float32> lowerLeftVertexAttribute(lowerLeftVertexData.size(), IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadNever, IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadOften);

	auto upperLeftVertexBuffer = renderer.CreateVertexBuffer();
	REQUIRE(upperLeftVertexBuffer->BindVertexAttribute(upperLeftVertexAttribute));
	REQUIRE(upperLeftVertexAttribute.SetInitialData(upperLeftVertexData));
	REQUIRE(upperLeftVertexBuffer->AllocateMemory());

	auto upperRightVertexBuffer = renderer.CreateVertexBuffer();
	REQUIRE(upperRightVertexBuffer->BindVertexAttribute(upperRightVertexAttribute));
	REQUIRE(upperRightVertexAttribute.SetInitialData(upperRightVertexData));
	REQUIRE(upperRightVertexBuffer->AllocateMemory());

	auto lowerLeftVertexBuffer = renderer.CreateVertexBuffer();
	REQUIRE(lowerLeftVertexBuffer->BindVertexAttribute(lowerLeftVertexAttribute));
	REQUIRE(lowerLeftVertexAttribute.SetInitialData(lowerLeftVertexData));
	REQUIRE(lowerLeftVertexBuffer->AllocateMemory());

	// Create the program node that writes one stencil value into the upper left quadrant
	auto upperLeftRenderableNode = renderer.CreateRenderableNode();
	REQUIRE(upperLeftRenderableNode->BindVertexAttribute(upperLeftVertexAttribute, positionAttributeId));
	REQUIRE(upperLeftRenderableNode->SetPrimitiveMode(IRenderableNode::PrimitiveMode::Triangles));

	auto upperLeftGroupNode = renderer.CreateStateGroupNode();
	upperLeftGroupNode->SetDepthTestEnabled(false);
	upperLeftGroupNode->SetDepthWriteEnabled(false);
	upperLeftGroupNode->SetPolygonCullMode(IStateGroupNode::PolygonCullMode::None);
	upperLeftGroupNode->SetStencilTestEnabled(true);
	upperLeftGroupNode->SetStencilReferenceValue(192);
	upperLeftGroupNode->SetStencilOperation(IStateGroupNode::StencilTargetFace::FrontAndBackFace, IStateGroupNode::StencilComparisonFunction::Always, IStateGroupNode::StencilOperation::Replace, IStateGroupNode::StencilOperation::Keep, IStateGroupNode::StencilOperation::Keep);
	upperLeftGroupNode->AddChildNode(upperLeftRenderableNode.get());

	auto upperLeftProgramNode = renderer.CreateProgramNode();
	REQUIRE(upperLeftProgramNode->BindShaderProgram(shaderProgram.get()));
	upperLeftProgramNode->AddChildNode(upperLeftGroupNode.get());

	// Create the program node that writes one stencil value into the upper right quadrant
	auto upperRightRenderableNode = renderer.CreateRenderableNode();
	REQUIRE(upperRightRenderableNode->BindVertexAttribute(upperRightVertexAttribute, positionAttributeId));
	REQUIRE(upperRightRenderableNode->SetPrimitiveMode(IRenderableNode::PrimitiveMode::Triangles));

	auto upperRightGroupNode = renderer.CreateStateGroupNode();
	upperRightGroupNode->SetDepthTestEnabled(false);
	upperRightGroupNode->SetDepthWriteEnabled(false);
	upperRightGroupNode->SetPolygonCullMode(IStateGroupNode::PolygonCullMode::None);
	upperRightGroupNode->SetStencilTestEnabled(true);
	upperRightGroupNode->SetStencilReferenceValue(96);
	upperRightGroupNode->SetStencilOperation(IStateGroupNode::StencilTargetFace::FrontAndBackFace, IStateGroupNode::StencilComparisonFunction::Always, IStateGroupNode::StencilOperation::Replace, IStateGroupNode::StencilOperation::Keep, IStateGroupNode::StencilOperation::Keep);
	upperRightGroupNode->AddChildNode(upperRightRenderableNode.get());

	auto upperRightProgramNode = renderer.CreateProgramNode();
	REQUIRE(upperRightProgramNode->BindShaderProgram(shaderProgram.get()));
	upperRightProgramNode->AddChildNode(upperRightGroupNode.get());

	// Create the program node that writes one stencil value into the lower left quadrant
	auto lowerLeftRenderableNode = renderer.CreateRenderableNode();
	REQUIRE(lowerLeftRenderableNode->BindVertexAttribute(lowerLeftVertexAttribute, positionAttributeId));
	REQUIRE(lowerLeftRenderableNode->SetPrimitiveMode(IRenderableNode::PrimitiveMode::Triangles));

	auto lowerLeftGroupNode = renderer.CreateStateGroupNode();
	lowerLeftGroupNode->SetDepthTestEnabled(false);
	lowerLeftGroupNode->SetDepthWriteEnabled(false);
	lowerLeftGroupNode->SetPolygonCullMode(IStateGroupNode::PolygonCullMode::None);
	lowerLeftGroupNode->SetStencilTestEnabled(true);
	lowerLeftGroupNode->SetStencilReferenceValue(224);
	lowerLeftGroupNode->SetStencilOperation(IStateGroupNode::StencilTargetFace::FrontAndBackFace, IStateGroupNode::StencilComparisonFunction::Always, IStateGroupNode::StencilOperation::Replace, IStateGroupNode::StencilOperation::Keep, IStateGroupNode::StencilOperation::Keep);
	lowerLeftGroupNode->AddChildNode(lowerLeftRenderableNode.get());

	auto lowerLeftProgramNode = renderer.CreateProgramNode();
	REQUIRE(lowerLeftProgramNode->BindShaderProgram(shaderProgram.get()));
	lowerLeftProgramNode->AddChildNode(lowerLeftGroupNode.get());

	// Run the clear and draw sequence against each supported window stencil format first
	const std::array<IFrameBuffer::WindowDepthStencilMode, 2> windowModes = {{
	  IFrameBuffer::WindowDepthStencilMode::DepthUNorm24StencilUInt8,
	  IFrameBuffer::WindowDepthStencilMode::DepthFloat32StencilUInt8,
	}};
	for (auto mode : windowModes)
	{
		renderer.WaitForDeferredDeletionComplete();

		auto frameBuffer = renderer.CreateFrameBuffer();
		frameBuffer->DefineViewportRegion(V2UInt32(0, 0), session.TestWindowSize());
		REQUIRE(uiThread.InvokeSync([&] { return frameBuffer->BindWindow(testWindowInfo, mode); }));
		auto renderPassNode = renderer.CreateRenderPassNode();
		renderPassNode->BindFrameBuffer(frameBuffer.get());
		renderer.SetRenderPasses(&renderPassNode, 1);

		// Clear the window bound stencil attachment with no renderable content
		renderPassNode->SetAttachmentClearData(IFrameBuffer::AttachmentType::Color, 0, V4Float32(0.0f, 0.0f, 0.0f, 1.0f));
		renderPassNode->SetAttachmentClearData(IFrameBuffer::AttachmentType::Depth, 0, V4Float32(1.0f, 0.0f, 0.0f, 0.0f));
		renderPassNode->SetAttachmentClearData(IFrameBuffer::AttachmentType::Stencil, 0, V4UInt32(64, 0, 0, 0));
		auto frameBufferCapture = renderer.CreateFrameBufferOutput();
		frameBufferCapture->SetDetachAfterCapture(true);
		frameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Stencil);
		DrawOneFrame();
		session.AddTestImageResult(std::string("Window") + WindowDepthStencilModeName(mode) + "ClearOnly", "A uniformly shaded stencil image after clearing the window-bound " + std::string(WindowDepthStencilModeName(mode)) + " stencil attachment to one constant value with no renderable content.", std::move(frameBufferCapture), IImageDiff::Algorithm::AllOfThem, 0.95);

		// Clear the window bound stencil attachment and overwrite the upper left quadrant
		renderPassNode->SetAttachmentClearData(IFrameBuffer::AttachmentType::Stencil, 0, V4UInt32(32, 0, 0, 0));
		renderPassNode->AddChildNode(upperLeftProgramNode.get());
		frameBufferCapture = renderer.CreateFrameBufferOutput();
		frameBufferCapture->SetDetachAfterCapture(true);
		frameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Stencil);
		DrawOneFrame();
		session.AddTestImageResult(std::string("Window") + WindowDepthStencilModeName(mode) + "ClearAndUpperLeft", "A stencil image with one tone in most of the frame and a different tone in the upper-left triangle where new stencil values were written after clearing the window-bound " + std::string(WindowDepthStencilModeName(mode)) + " stencil attachment.", std::move(frameBufferCapture), IImageDiff::Algorithm::AllOfThem, 0.95);

		// Clear the window bound stencil attachment again and overwrite the lower left quadrant
		renderPassNode->RemoveAllChildNodes();
		renderPassNode->SetAttachmentClearData(IFrameBuffer::AttachmentType::Stencil, 0, V4UInt32(16, 0, 0, 0));
		renderPassNode->AddChildNode(lowerLeftProgramNode.get());
		frameBufferCapture = renderer.CreateFrameBufferOutput();
		frameBufferCapture->SetDetachAfterCapture(true);
		frameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Stencil);
		DrawOneFrame();
		session.AddTestImageResult(std::string("Window") + WindowDepthStencilModeName(mode) + "ClearAndLowerLeft", "A stencil image reset to one uniform tone except for a different tone in the lower-left triangle after clearing the window-bound " + std::string(WindowDepthStencilModeName(mode)) + " stencil attachment again.", std::move(frameBufferCapture), IImageDiff::Algorithm::AllOfThem, 0.95);

		renderPassNode->RemoveAllChildNodes();
	}
	renderer.WaitForDeferredDeletionComplete();

	// Repeat the same logical sequence using an offscreen color and depth stencil target pair
	V2UInt32 offscreenSize = session.TestWindowSize();
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
	auto renderPassNode = renderer.CreateRenderPassNode();
	renderPassNode->BindFrameBuffer(offscreenFrameBuffer.get());
	renderer.SetRenderPasses(&renderPassNode, 1);

	// Clear the offscreen stencil attachment with no renderable content
	renderPassNode->RemoveAllChildNodes();
	renderPassNode->SetAttachmentClearData(IFrameBuffer::AttachmentType::Color, 0, V4Float32(0.0f, 0.0f, 0.0f, 1.0f));
	renderPassNode->SetAttachmentClearData(IFrameBuffer::AttachmentType::Depth, 0, V4Float32(1.0f, 0.0f, 0.0f, 0.0f));
	renderPassNode->SetAttachmentClearData(IFrameBuffer::AttachmentType::Stencil, 0, V4UInt32(64, 0, 0, 0));
	auto frameBufferCapture = renderer.CreateFrameBufferOutput();
	frameBufferCapture->SetDetachAfterCapture(true);
	offscreenFrameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Stencil);
	DrawOneFrame();
	session.AddTestImageResult("OffscreenClearOnly", "A uniformly shaded stencil image after clearing the offscreen stencil attachment to one constant value with no renderable content.", std::move(frameBufferCapture), IImageDiff::Algorithm::AllOfThem, 0.95);

	// Load the offscreen stencil attachment without drawing anything new
	renderPassNode->SetAttachmentLoadStoreBehavior(IFrameBuffer::AttachmentType::Color, 0, IRenderPassNode::AttachmentLoadBehavior::LoadExistingData, IRenderPassNode::AttachmentStoreBehavior::StoreFinalData);
	renderPassNode->SetAttachmentLoadStoreBehavior(IFrameBuffer::AttachmentType::Depth, 0, IRenderPassNode::AttachmentLoadBehavior::LoadExistingData, IRenderPassNode::AttachmentStoreBehavior::StoreFinalData);
	renderPassNode->SetAttachmentLoadStoreBehavior(IFrameBuffer::AttachmentType::Stencil, 0, IRenderPassNode::AttachmentLoadBehavior::LoadExistingData, IRenderPassNode::AttachmentStoreBehavior::StoreFinalData);
	renderPassNode->RemoveAttachmentClearData(IFrameBuffer::AttachmentType::Color, 0);
	renderPassNode->RemoveAttachmentClearData(IFrameBuffer::AttachmentType::Depth, 0);
	renderPassNode->RemoveAttachmentClearData(IFrameBuffer::AttachmentType::Stencil, 0);
	frameBufferCapture = renderer.CreateFrameBufferOutput();
	frameBufferCapture->SetDetachAfterCapture(true);
	offscreenFrameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Stencil);
	DrawOneFrame();
	session.AddTestImageResult("OffscreenLoadOnly", "The same uniformly shaded stencil image preserved after loading the offscreen stencil attachment without any renderable content.", std::move(frameBufferCapture), IImageDiff::Algorithm::AllOfThem, 0.95);

	// Clear the offscreen stencil attachment and overwrite the upper left quadrant
	renderPassNode->SetAttachmentClearData(IFrameBuffer::AttachmentType::Stencil, 0, V4UInt32(32, 0, 0, 0));
	renderPassNode->AddChildNode(upperLeftProgramNode.get());
	frameBufferCapture = renderer.CreateFrameBufferOutput();
	frameBufferCapture->SetDetachAfterCapture(true);
	offscreenFrameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Stencil);
	DrawOneFrame();
	session.AddTestImageResult("OffscreenClearAndUpperLeft", "A stencil image with one tone in most of the frame and a different tone in the upper-left triangle where new stencil values were written after clearing the offscreen stencil attachment.", std::move(frameBufferCapture), IImageDiff::Algorithm::AllOfThem, 0.95);

	// Load the offscreen stencil attachment again with no new renderable content
	renderPassNode->RemoveAllChildNodes();
	renderPassNode->SetAttachmentLoadStoreBehavior(IFrameBuffer::AttachmentType::Color, 0, IRenderPassNode::AttachmentLoadBehavior::LoadExistingData, IRenderPassNode::AttachmentStoreBehavior::StoreFinalData);
	renderPassNode->SetAttachmentLoadStoreBehavior(IFrameBuffer::AttachmentType::Depth, 0, IRenderPassNode::AttachmentLoadBehavior::LoadExistingData, IRenderPassNode::AttachmentStoreBehavior::StoreFinalData);
	renderPassNode->SetAttachmentLoadStoreBehavior(IFrameBuffer::AttachmentType::Stencil, 0, IRenderPassNode::AttachmentLoadBehavior::LoadExistingData, IRenderPassNode::AttachmentStoreBehavior::StoreFinalData);
	renderPassNode->RemoveAttachmentClearData(IFrameBuffer::AttachmentType::Color, 0);
	renderPassNode->RemoveAttachmentClearData(IFrameBuffer::AttachmentType::Depth, 0);
	renderPassNode->RemoveAttachmentClearData(IFrameBuffer::AttachmentType::Stencil, 0);
	frameBufferCapture = renderer.CreateFrameBufferOutput();
	frameBufferCapture->SetDetachAfterCapture(true);
	offscreenFrameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Stencil);
	DrawOneFrame();
	session.AddTestImageResult("OffscreenLoadAfterUpperLeft", "The same stencil image preserved after loading the offscreen stencil attachment with no new renderable content.", std::move(frameBufferCapture), IImageDiff::Algorithm::AllOfThem, 0.95);

	// Load the offscreen stencil attachment and add a second stencil writing triangle in the upper
	// right quadrant
	renderPassNode->AddChildNode(upperRightProgramNode.get());
	frameBufferCapture = renderer.CreateFrameBufferOutput();
	frameBufferCapture->SetDetachAfterCapture(true);
	offscreenFrameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Stencil);
	DrawOneFrame();
	session.AddTestImageResult("OffscreenLoadAndUpperRight", "A stencil image with distinct tones in the upper-left and upper-right triangles after loading the offscreen stencil attachment and writing a second set of stencil values.", std::move(frameBufferCapture), IImageDiff::Algorithm::AllOfThem, 0.95);

	// Clear the offscreen stencil attachment again and overwrite the lower left quadrant
	renderPassNode->RemoveAllChildNodes();
	renderPassNode->SetAttachmentClearData(IFrameBuffer::AttachmentType::Stencil, 0, V4UInt32(16, 0, 0, 0));
	renderPassNode->AddChildNode(lowerLeftProgramNode.get());
	frameBufferCapture = renderer.CreateFrameBufferOutput();
	frameBufferCapture->SetDetachAfterCapture(true);
	offscreenFrameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Stencil);
	DrawOneFrame();
	session.AddTestImageResult("OffscreenClearAndLowerLeft", "A stencil image reset to one uniform tone except for a different tone in the lower-left triangle after clearing the offscreen stencil attachment again.", std::move(frameBufferCapture), IImageDiff::Algorithm::AllOfThem, 0.95);

	// Remove all our defined render passes
	renderer.RemoveAllRenderPasses();
	return true;
}

} // namespace cobalt::graphics::testing
