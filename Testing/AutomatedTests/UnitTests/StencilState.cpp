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
uniform float4 drawColor;

float4 main() : SV_TARGET0
{
    return drawColor;
}
)";

DEFINE_UNIT_TEST_WITH_BASE("StateGroup/StencilState", UnitTestBase)
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

	// Retrieve our shader attribute and state IDs
	auto positionAttributeId = shaderProgram->GetVertexAttributeId("position");
	auto drawColorStateId = shaderProgram->GetStateValueId("drawColor");

	// Generate the centred quad that seeds the stencil buffer and the fullscreen quad that reads it
	// back
	std::vector<V4Float32> centeredQuadVertexData;
	std::vector<V4Float32> fullScreenVertexData;
	Geometry().CreateCenteredQuad(0.5f, 0.55f, centeredQuadVertexData);
	Geometry().CreateFullscreenQuad(0.5f, fullScreenVertexData);

	// Create our vertex attributes and populate them with geometry data
	VertexAttribute<V4Float32> centeredQuadVertexAttribute(centeredQuadVertexData.size(), IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadNever, IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadOften);
	VertexAttribute<V4Float32> fullScreenVertexAttribute(fullScreenVertexData.size(), IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadNever, IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadOften);

	auto centeredQuadVertexBuffer = renderer.CreateVertexBuffer();
	REQUIRE(centeredQuadVertexBuffer->BindVertexAttribute(centeredQuadVertexAttribute));
	REQUIRE(centeredQuadVertexAttribute.SetInitialData(centeredQuadVertexData));
	REQUIRE(centeredQuadVertexBuffer->AllocateMemory());

	auto fullScreenVertexBuffer = renderer.CreateVertexBuffer();
	REQUIRE(fullScreenVertexBuffer->BindVertexAttribute(fullScreenVertexAttribute));
	REQUIRE(fullScreenVertexAttribute.SetInitialData(fullScreenVertexData));
	REQUIRE(fullScreenVertexBuffer->AllocateMemory());

	// Create the stencil seeding draw, which writes a known value into the centred quad.
	auto stencilSeedRenderableNode = renderer.CreateRenderableNode();
	REQUIRE(stencilSeedRenderableNode->BindVertexAttribute(centeredQuadVertexAttribute, positionAttributeId));
	REQUIRE(stencilSeedRenderableNode->SetPrimitiveMode(IRenderableNode::PrimitiveMode::Triangles));
	stencilSeedRenderableNode->SetStateValue(drawColorStateId, V4Float32(0.0f, 0.0f, 0.0f, 1.0f));

	auto stencilSeedGroupNode = renderer.CreateStateGroupNode();
	stencilSeedGroupNode->SetDepthTestEnabled(false);
	stencilSeedGroupNode->SetDepthWriteEnabled(false);
	stencilSeedGroupNode->SetPolygonCullMode(IStateGroupNode::PolygonCullMode::None);
	stencilSeedGroupNode->AddChildNode(stencilSeedRenderableNode.get());

	auto stencilSeedProgramNode = renderer.CreateProgramNode();
	REQUIRE(stencilSeedProgramNode->BindShaderProgram(shaderProgram.get()));
	stencilSeedProgramNode->AddChildNode(stencilSeedGroupNode.get());

	// Create the fullscreen draw used to verify stencil comparisons against the seeded values
	auto stencilTestRenderableNode = renderer.CreateRenderableNode();
	REQUIRE(stencilTestRenderableNode->BindVertexAttribute(fullScreenVertexAttribute, positionAttributeId));
	REQUIRE(stencilTestRenderableNode->SetPrimitiveMode(IRenderableNode::PrimitiveMode::Triangles));
	stencilTestRenderableNode->SetStateValue(drawColorStateId, V4Float32(1.0f, 0.0f, 0.0f, 1.0f));

	auto stencilTestGroupNode = renderer.CreateStateGroupNode();
	stencilTestGroupNode->SetDepthTestEnabled(false);
	stencilTestGroupNode->SetDepthWriteEnabled(false);
	stencilTestGroupNode->SetPolygonCullMode(IStateGroupNode::PolygonCullMode::None);
	stencilTestGroupNode->AddChildNode(stencilTestRenderableNode.get());

	auto stencilTestProgramNode = renderer.CreateProgramNode();
	REQUIRE(stencilTestProgramNode->BindShaderProgram(shaderProgram.get()));
	stencilTestProgramNode->AddChildNode(stencilTestGroupNode.get());

	// Run the same logical stencil comparisons against both window bound and offscreen framebuffers
	auto runSequence = [&](IFrameBuffer& frameBuffer, const std::string& prefix, const std::string& targetDescription) {
		// Configure the first draw to seed the centred quad with one stencil value
		stencilSeedGroupNode->SetStencilTestEnabled(true);
		stencilSeedGroupNode->SetStencilReferenceValue(4);
		stencilSeedGroupNode->SetStencilOperation(IStateGroupNode::StencilTargetFace::FrontAndBackFace, IStateGroupNode::StencilComparisonFunction::Always, IStateGroupNode::StencilOperation::Replace, IStateGroupNode::StencilOperation::Keep, IStateGroupNode::StencilOperation::Keep);

		// Configure the second draw to read back those values across the fullscreen quad
		stencilTestGroupNode->SetStencilTestEnabled(true);
		stencilTestGroupNode->SetStencilReferenceValue(3);
		stencilTestGroupNode->SetStencilOperation(IStateGroupNode::StencilTargetFace::FrontAndBackFace, IStateGroupNode::StencilComparisonFunction::Always, IStateGroupNode::StencilOperation::Keep, IStateGroupNode::StencilOperation::Keep, IStateGroupNode::StencilOperation::Keep);

		// Create the render pass and clear the framebuffer to known color, depth, and stencil values.
		auto renderPassNode = renderer.CreateRenderPassNode();
		renderPassNode->BindFrameBuffer(&frameBuffer);
		renderPassNode->SetAttachmentClearData(IFrameBuffer::AttachmentType::Color, 0, V4Float32(0.0f, 0.0f, 0.0f, 1.0f));
		renderPassNode->SetAttachmentClearData(IFrameBuffer::AttachmentType::Depth, 0, V4Float32(1.0f, 0.0f, 0.0f, 0.0f));
		renderPassNode->SetAttachmentClearData(IFrameBuffer::AttachmentType::Stencil, 0, V4UInt32(2, 0, 0, 0));
		renderPassNode->AddChildNode(stencilSeedProgramNode.get());
		renderPassNode->AddChildNode(stencilTestProgramNode.get());
		renderer.SetRenderPasses(&renderPassNode, 1);

		// First verify that disabling stencil testing allows the fullscreen quad to cover the whole
		// frame
		stencilTestGroupNode->SetStencilTestEnabled(false);
		auto frameBufferCapture = renderer.CreateFrameBufferOutput();
		frameBufferCapture->SetDetachAfterCapture(true);
		frameBuffer.AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
		DrawOneFrame();
		session.AddTestImageResult(prefix + "StencilTestDisabled", "The " + targetDescription + " image should be a full red frame because stencil testing is disabled after seeding the stencil buffer.", std::move(frameBufferCapture), IImageDiff::Algorithm::AllOfThem, 0.95);
		stencilTestGroupNode->SetStencilTestEnabled(true);

		struct StencilCompareCase
		{
			const char* name;
			const char* description;
			IStateGroupNode::StencilComparisonFunction comparisonFunction;
		};

		// Step through the comparison functions, keeping the seeded stencil values unchanged between
		// frames
		const std::array<StencilCompareCase, 8> comparisonCases = {{
		  {"CompareNever", "a black image because the red fullscreen quad always fails stencil testing", IStateGroupNode::StencilComparisonFunction::Never},
		  {"CompareEqual", "a black image because the reference value does not match any seeded stencil value", IStateGroupNode::StencilComparisonFunction::Equal},
		  {"CompareNotEqual", "a full red image because the reference value differs from every seeded stencil value", IStateGroupNode::StencilComparisonFunction::NotEqual},
		  {"CompareLess", "a red square in the center where the seeded stencil value is greater than the reference value", IStateGroupNode::StencilComparisonFunction::Less},
		  {"CompareLessOrEqual", "a red square in the center where the seeded stencil value is greater than or equal to the reference value", IStateGroupNode::StencilComparisonFunction::LessOrEqual},
		  {"CompareGreater", "a red border around a black center where the clear stencil value is less than the reference value", IStateGroupNode::StencilComparisonFunction::Greater},
		  {"CompareGreaterOrEqual", "a red border around a black center where the clear stencil value is less than or equal to the reference value", IStateGroupNode::StencilComparisonFunction::GreaterOrEqual},
		  {"CompareAlways", "a full red image because the red fullscreen quad always passes stencil testing", IStateGroupNode::StencilComparisonFunction::Always},
		}};
		for (const auto& comparisonCase : comparisonCases)
		{
			stencilTestGroupNode->SetStencilOperation(IStateGroupNode::StencilTargetFace::FrontAndBackFace, comparisonCase.comparisonFunction, IStateGroupNode::StencilOperation::Keep, IStateGroupNode::StencilOperation::Keep, IStateGroupNode::StencilOperation::Keep);
			frameBufferCapture = renderer.CreateFrameBufferOutput();
			frameBufferCapture->SetDetachAfterCapture(true);
			frameBuffer.AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
			DrawOneFrame();
			session.AddTestImageResult(prefix + comparisonCase.name, "The " + targetDescription + " image should show " + std::string(comparisonCase.description) + ".", std::move(frameBufferCapture), IImageDiff::Algorithm::AllOfThem, 0.95);
		}
	};

	// Run the stencil comparisons against each supported window depth stencil mode first
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
		runSequence(*frameBuffer, std::string("Window") + WindowDepthStencilModeName(mode), std::string("window-bound ") + WindowDepthStencilModeName(mode));
	}

	// Repeat the same stencil comparisons using an offscreen color and depth stencil target pair
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
	runSequence(*offscreenFrameBuffer, "Offscreen", "offscreen");

	// Remove all our defined render passes
	renderer.RemoveAllRenderPasses();
	return true;
}

} // namespace cobalt::graphics::testing
