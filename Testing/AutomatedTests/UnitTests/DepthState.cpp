// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "../UnitTestBase.h"
#include <array>
namespace cobalt::graphics::testing {
using namespace cobalt::graphics;

namespace {

float ClipSpaceDepthValue(IGraphicsDevice::DepthRange depthRange, float targetDepthValue)
{
	if (depthRange == IGraphicsDevice::DepthRange::NegativeOneToOne)
	{
		return (targetDepthValue * 2.0f) - 1.0f;
	}
	return targetDepthValue;
}

constexpr float SeededDepthComparisonValue = 0.5f;
constexpr float CloserDepthComparisonValue = 0.25f;
constexpr float FurtherDepthComparisonValue = 0.75f;
constexpr float DepthBiasClampProbeDepthValue = 0.50025f;
constexpr float DepthBiasClampLimit = -0.00005f;
constexpr float DepthBiasClampUnclampedBias = -10000000.0f;

const char* WindowDepthStencilModeName(IFrameBuffer::WindowDepthStencilMode mode)
{
	switch (mode)
	{
	case IFrameBuffer::WindowDepthStencilMode::DepthUNorm16:
		return "DepthUNorm16";
	case IFrameBuffer::WindowDepthStencilMode::DepthUNorm24:
		return "DepthUNorm24";
	case IFrameBuffer::WindowDepthStencilMode::DepthUNorm24StencilUInt8:
		return "DepthUNorm24StencilUInt8";
	case IFrameBuffer::WindowDepthStencilMode::DepthFloat32:
		return "DepthFloat32";
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

DEFINE_UNIT_TEST_WITH_BASE("StateGroup/DepthState", UnitTestBase)
{
	// Retrieve our important objects
	auto& renderer = session.Renderer();
	auto& uiThread = session.UIThread();
	auto& testWindowInfo = *session.TestWindowPlatformInfo();
	auto depthRange = session.Device().GetFrameBufferLimits().depthRange;
	const bool depthBiasClampSupported = session.Device().IsFeatureSupported(IGraphicsDevice::Feature::DepthBiasClamp);

	// Create and compile our shader program
	auto shaderProgram = renderer.CreateShaderProgram();
	REQUIRE(shaderProgram->LoadShaderStage(IShaderProgram::ShaderStage::Vertex, ShaderSourceInfoHLSL(VertexShader)));
	REQUIRE(shaderProgram->LoadShaderStage(IShaderProgram::ShaderStage::Fragment, ShaderSourceInfoHLSL(FragmentShader)));
	REQUIRE(shaderProgram->CompileProgram());

	// Retrieve our shader attribute and state IDs
	auto positionAttributeId = shaderProgram->GetVertexAttributeId("position");
	auto drawColorStateId = shaderProgram->GetStateValueId("drawColor");

	// Generate geometry for the fullscreen comparison quad and the layered depth write test scene
	std::vector<V4Float32> closerDepthComparisonVertexData;
	std::vector<V4Float32> furtherDepthComparisonVertexData;
	std::vector<V4Float32> fullScreenVertexData;
	std::vector<V4Float32> backgroundVertexData;
	std::vector<V4Float32> foregroundVertexData;
	std::vector<V4Float32> middleVertexData;
	std::vector<V4Float32> depthBiasClampProbeVertexData;
	Geometry().CreateFullscreenQuad(ClipSpaceDepthValue(depthRange, CloserDepthComparisonValue), closerDepthComparisonVertexData);
	Geometry().CreateFullscreenQuad(ClipSpaceDepthValue(depthRange, FurtherDepthComparisonValue), furtherDepthComparisonVertexData);
	Geometry().CreateFullscreenQuad(ClipSpaceDepthValue(depthRange, SeededDepthComparisonValue), fullScreenVertexData);
	Geometry().CreateFullscreenQuad(ClipSpaceDepthValue(depthRange, 0.6f), backgroundVertexData);
	Geometry().CreateCenteredQuad(ClipSpaceDepthValue(depthRange, 0.4f), 0.65f, foregroundVertexData);
	Geometry().CreateCenteredQuad(ClipSpaceDepthValue(depthRange, SeededDepthComparisonValue), 0.35f, middleVertexData);
	Geometry().CreateFullscreenQuad(ClipSpaceDepthValue(depthRange, DepthBiasClampProbeDepthValue), depthBiasClampProbeVertexData);

	// Create our vertex attributes
	VertexAttribute<V4Float32> closerDepthComparisonVertexAttribute(closerDepthComparisonVertexData.size(), IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadNever, IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadOften);
	VertexAttribute<V4Float32> furtherDepthComparisonVertexAttribute(furtherDepthComparisonVertexData.size(), IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadNever, IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadOften);
	VertexAttribute<V4Float32> fullScreenVertexAttribute(fullScreenVertexData.size(), IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadNever, IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadOften);
	VertexAttribute<V4Float32> backgroundVertexAttribute(backgroundVertexData.size(), IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadNever, IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadOften);
	VertexAttribute<V4Float32> foregroundVertexAttribute(foregroundVertexData.size(), IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadNever, IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadOften);
	VertexAttribute<V4Float32> middleVertexAttribute(middleVertexData.size(), IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadNever, IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadOften);
	VertexAttribute<V4Float32> depthBiasClampProbeVertexAttribute(depthBiasClampProbeVertexData.size(), IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadNever, IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadOften);

	// Create our vertex buffers and populate them with data
	auto closerDepthComparisonVertexBuffer = renderer.CreateVertexBuffer();
	REQUIRE(closerDepthComparisonVertexBuffer->BindVertexAttribute(closerDepthComparisonVertexAttribute));
	REQUIRE(closerDepthComparisonVertexAttribute.SetInitialData(closerDepthComparisonVertexData));
	REQUIRE(closerDepthComparisonVertexBuffer->AllocateMemory());

	auto furtherDepthComparisonVertexBuffer = renderer.CreateVertexBuffer();
	REQUIRE(furtherDepthComparisonVertexBuffer->BindVertexAttribute(furtherDepthComparisonVertexAttribute));
	REQUIRE(furtherDepthComparisonVertexAttribute.SetInitialData(furtherDepthComparisonVertexData));
	REQUIRE(furtherDepthComparisonVertexBuffer->AllocateMemory());

	auto fullScreenVertexBuffer = renderer.CreateVertexBuffer();
	REQUIRE(fullScreenVertexBuffer->BindVertexAttribute(fullScreenVertexAttribute));
	REQUIRE(fullScreenVertexAttribute.SetInitialData(fullScreenVertexData));
	REQUIRE(fullScreenVertexBuffer->AllocateMemory());

	auto backgroundVertexBuffer = renderer.CreateVertexBuffer();
	REQUIRE(backgroundVertexBuffer->BindVertexAttribute(backgroundVertexAttribute));
	REQUIRE(backgroundVertexAttribute.SetInitialData(backgroundVertexData));
	REQUIRE(backgroundVertexBuffer->AllocateMemory());

	auto foregroundVertexBuffer = renderer.CreateVertexBuffer();
	REQUIRE(foregroundVertexBuffer->BindVertexAttribute(foregroundVertexAttribute));
	REQUIRE(foregroundVertexAttribute.SetInitialData(foregroundVertexData));
	REQUIRE(foregroundVertexBuffer->AllocateMemory());

	auto middleVertexBuffer = renderer.CreateVertexBuffer();
	REQUIRE(middleVertexBuffer->BindVertexAttribute(middleVertexAttribute));
	REQUIRE(middleVertexAttribute.SetInitialData(middleVertexData));
	REQUIRE(middleVertexBuffer->AllocateMemory());

	auto depthBiasClampProbeVertexBuffer = renderer.CreateVertexBuffer();
	REQUIRE(depthBiasClampProbeVertexBuffer->BindVertexAttribute(depthBiasClampProbeVertexAttribute));
	REQUIRE(depthBiasClampProbeVertexAttribute.SetInitialData(depthBiasClampProbeVertexData));
	REQUIRE(depthBiasClampProbeVertexBuffer->AllocateMemory());

	// Create the fullscreen quad used to seed the depth buffer for the depth comparison function cases
	auto blackDepthSeedRenderableNode = renderer.CreateRenderableNode();
	REQUIRE(blackDepthSeedRenderableNode->BindVertexAttribute(fullScreenVertexAttribute, positionAttributeId));
	REQUIRE(blackDepthSeedRenderableNode->SetPrimitiveMode(IRenderableNode::PrimitiveMode::Triangles));
	blackDepthSeedRenderableNode->SetStateValue(drawColorStateId, V4Float32(0.0f, 0.0f, 0.0f, 1.0f));

	auto blackDepthSeedGroupNode = renderer.CreateStateGroupNode();
	blackDepthSeedGroupNode->SetPolygonCullMode(IStateGroupNode::PolygonCullMode::None);
	blackDepthSeedGroupNode->AddChildNode(blackDepthSeedRenderableNode.get());

	auto blackDepthSeedProgramNode = renderer.CreateProgramNode();
	REQUIRE(blackDepthSeedProgramNode->BindShaderProgram(shaderProgram.get()));
	blackDepthSeedProgramNode->AddChildNode(blackDepthSeedGroupNode.get());

	// Create the fullscreen quad used for the equal-depth comparison function cases
	auto equalDepthRenderableNode = renderer.CreateRenderableNode();
	REQUIRE(equalDepthRenderableNode->BindVertexAttribute(fullScreenVertexAttribute, positionAttributeId));
	REQUIRE(equalDepthRenderableNode->SetPrimitiveMode(IRenderableNode::PrimitiveMode::Triangles));
	equalDepthRenderableNode->SetStateValue(drawColorStateId, V4Float32(1.0f, 0.0f, 0.0f, 1.0f));

	auto equalDepthGroupNode = renderer.CreateStateGroupNode();
	equalDepthGroupNode->SetPolygonCullMode(IStateGroupNode::PolygonCullMode::None);
	equalDepthGroupNode->AddChildNode(equalDepthRenderableNode.get());

	auto equalDepthProgramNode = renderer.CreateProgramNode();
	REQUIRE(equalDepthProgramNode->BindShaderProgram(shaderProgram.get()));
	equalDepthProgramNode->AddChildNode(equalDepthGroupNode.get());

	auto closerDepthRenderableNode = renderer.CreateRenderableNode();
	REQUIRE(closerDepthRenderableNode->BindVertexAttribute(closerDepthComparisonVertexAttribute, positionAttributeId));
	REQUIRE(closerDepthRenderableNode->SetPrimitiveMode(IRenderableNode::PrimitiveMode::Triangles));
	closerDepthRenderableNode->SetStateValue(drawColorStateId, V4Float32(1.0f, 0.0f, 0.0f, 1.0f));

	auto closerDepthGroupNode = renderer.CreateStateGroupNode();
	closerDepthGroupNode->SetPolygonCullMode(IStateGroupNode::PolygonCullMode::None);
	closerDepthGroupNode->AddChildNode(closerDepthRenderableNode.get());

	auto closerDepthProgramNode = renderer.CreateProgramNode();
	REQUIRE(closerDepthProgramNode->BindShaderProgram(shaderProgram.get()));
	closerDepthProgramNode->AddChildNode(closerDepthGroupNode.get());

	auto furtherDepthRenderableNode = renderer.CreateRenderableNode();
	REQUIRE(furtherDepthRenderableNode->BindVertexAttribute(furtherDepthComparisonVertexAttribute, positionAttributeId));
	REQUIRE(furtherDepthRenderableNode->SetPrimitiveMode(IRenderableNode::PrimitiveMode::Triangles));
	furtherDepthRenderableNode->SetStateValue(drawColorStateId, V4Float32(1.0f, 0.0f, 0.0f, 1.0f));

	auto furtherDepthGroupNode = renderer.CreateStateGroupNode();
	furtherDepthGroupNode->SetPolygonCullMode(IStateGroupNode::PolygonCullMode::None);
	furtherDepthGroupNode->AddChildNode(furtherDepthRenderableNode.get());

	auto furtherDepthProgramNode = renderer.CreateProgramNode();
	REQUIRE(furtherDepthProgramNode->BindShaderProgram(shaderProgram.get()));
	furtherDepthProgramNode->AddChildNode(furtherDepthGroupNode.get());

	// Create the coplanar fullscreen quads used to make depth bias visible.
	auto redDepthBiasBaselineRenderableNode = renderer.CreateRenderableNode();
	REQUIRE(redDepthBiasBaselineRenderableNode->BindVertexAttribute(fullScreenVertexAttribute, positionAttributeId));
	REQUIRE(redDepthBiasBaselineRenderableNode->SetPrimitiveMode(IRenderableNode::PrimitiveMode::Triangles));
	redDepthBiasBaselineRenderableNode->SetStateValue(drawColorStateId, V4Float32(1.0f, 0.0f, 0.0f, 1.0f));

	auto redDepthBiasBaselineGroupNode = renderer.CreateStateGroupNode();
	redDepthBiasBaselineGroupNode->SetPolygonCullMode(IStateGroupNode::PolygonCullMode::None);
	redDepthBiasBaselineGroupNode->AddChildNode(redDepthBiasBaselineRenderableNode.get());

	auto redDepthBiasBaselineProgramNode = renderer.CreateProgramNode();
	REQUIRE(redDepthBiasBaselineProgramNode->BindShaderProgram(shaderProgram.get()));
	redDepthBiasBaselineProgramNode->AddChildNode(redDepthBiasBaselineGroupNode.get());

	auto greenDepthBiasProbeRenderableNode = renderer.CreateRenderableNode();
	REQUIRE(greenDepthBiasProbeRenderableNode->BindVertexAttribute(fullScreenVertexAttribute, positionAttributeId));
	REQUIRE(greenDepthBiasProbeRenderableNode->SetPrimitiveMode(IRenderableNode::PrimitiveMode::Triangles));
	greenDepthBiasProbeRenderableNode->SetStateValue(drawColorStateId, V4Float32(0.0f, 1.0f, 0.0f, 1.0f));

	auto greenDepthBiasProbeGroupNode = renderer.CreateStateGroupNode();
	greenDepthBiasProbeGroupNode->SetPolygonCullMode(IStateGroupNode::PolygonCullMode::None);
	greenDepthBiasProbeGroupNode->AddChildNode(greenDepthBiasProbeRenderableNode.get());

	auto greenDepthBiasProbeProgramNode = renderer.CreateProgramNode();
	REQUIRE(greenDepthBiasProbeProgramNode->BindShaderProgram(shaderProgram.get()));
	greenDepthBiasProbeProgramNode->AddChildNode(greenDepthBiasProbeGroupNode.get());

	auto greenDepthBiasClampProbeRenderableNode = renderer.CreateRenderableNode();
	REQUIRE(greenDepthBiasClampProbeRenderableNode->BindVertexAttribute(depthBiasClampProbeVertexAttribute, positionAttributeId));
	REQUIRE(greenDepthBiasClampProbeRenderableNode->SetPrimitiveMode(IRenderableNode::PrimitiveMode::Triangles));
	greenDepthBiasClampProbeRenderableNode->SetStateValue(drawColorStateId, V4Float32(0.0f, 1.0f, 0.0f, 1.0f));

	auto greenDepthBiasClampProbeGroupNode = renderer.CreateStateGroupNode();
	greenDepthBiasClampProbeGroupNode->SetPolygonCullMode(IStateGroupNode::PolygonCullMode::None);
	greenDepthBiasClampProbeGroupNode->AddChildNode(greenDepthBiasClampProbeRenderableNode.get());

	auto greenDepthBiasClampProbeProgramNode = renderer.CreateProgramNode();
	REQUIRE(greenDepthBiasClampProbeProgramNode->BindShaderProgram(shaderProgram.get()));
	greenDepthBiasClampProbeProgramNode->AddChildNode(greenDepthBiasClampProbeGroupNode.get());

	// Create the three quads used for the depth write control case
	auto blueBackgroundRenderableNode = renderer.CreateRenderableNode();
	REQUIRE(blueBackgroundRenderableNode->BindVertexAttribute(backgroundVertexAttribute, positionAttributeId));
	REQUIRE(blueBackgroundRenderableNode->SetPrimitiveMode(IRenderableNode::PrimitiveMode::Triangles));
	blueBackgroundRenderableNode->SetStateValue(drawColorStateId, V4Float32(0.0f, 0.0f, 1.0f, 1.0f));

	auto blueBackgroundGroupNode = renderer.CreateStateGroupNode();
	blueBackgroundGroupNode->SetPolygonCullMode(IStateGroupNode::PolygonCullMode::None);
	blueBackgroundGroupNode->AddChildNode(blueBackgroundRenderableNode.get());

	auto blueBackgroundProgramNode = renderer.CreateProgramNode();
	REQUIRE(blueBackgroundProgramNode->BindShaderProgram(shaderProgram.get()));
	blueBackgroundProgramNode->AddChildNode(blueBackgroundGroupNode.get());

	auto redForegroundRenderableNode = renderer.CreateRenderableNode();
	REQUIRE(redForegroundRenderableNode->BindVertexAttribute(foregroundVertexAttribute, positionAttributeId));
	REQUIRE(redForegroundRenderableNode->SetPrimitiveMode(IRenderableNode::PrimitiveMode::Triangles));
	redForegroundRenderableNode->SetStateValue(drawColorStateId, V4Float32(1.0f, 0.0f, 0.0f, 1.0f));

	auto redForegroundGroupNode = renderer.CreateStateGroupNode();
	redForegroundGroupNode->SetPolygonCullMode(IStateGroupNode::PolygonCullMode::None);
	redForegroundGroupNode->AddChildNode(redForegroundRenderableNode.get());

	auto redForegroundProgramNode = renderer.CreateProgramNode();
	REQUIRE(redForegroundProgramNode->BindShaderProgram(shaderProgram.get()));
	redForegroundProgramNode->AddChildNode(redForegroundGroupNode.get());

	auto greenMiddleRenderableNode = renderer.CreateRenderableNode();
	REQUIRE(greenMiddleRenderableNode->BindVertexAttribute(middleVertexAttribute, positionAttributeId));
	REQUIRE(greenMiddleRenderableNode->SetPrimitiveMode(IRenderableNode::PrimitiveMode::Triangles));
	greenMiddleRenderableNode->SetStateValue(drawColorStateId, V4Float32(0.0f, 1.0f, 0.0f, 1.0f));

	auto greenMiddleGroupNode = renderer.CreateStateGroupNode();
	greenMiddleGroupNode->SetPolygonCullMode(IStateGroupNode::PolygonCullMode::None);
	greenMiddleGroupNode->AddChildNode(greenMiddleRenderableNode.get());

	auto greenMiddleProgramNode = renderer.CreateProgramNode();
	REQUIRE(greenMiddleProgramNode->BindShaderProgram(shaderProgram.get()));
	greenMiddleProgramNode->AddChildNode(greenMiddleGroupNode.get());

	// Run the same logical set of depth state checks against the supplied framebuffer
	auto runSequence = [&](IFrameBuffer& frameBuffer, const std::string& prefix, const std::string& targetDescription) {
		// Create our render pass node for the depth comparison tests
		auto renderPassNode = renderer.CreateRenderPassNode();
		renderPassNode->BindFrameBuffer(&frameBuffer);
		renderPassNode->SetAttachmentClearData(IFrameBuffer::AttachmentType::Color, 0, V4Float32(0.0f, 0.0f, 0.0f, 1.0f));
		renderer.SetRenderPasses(&renderPassNode, 1);

		// Define the expected visual result for each depth comparison function
		struct DepthCompareCase
		{
			const char* name;
			const char* visibleResult;
			IStateGroupNode::DepthComparisonFunction comparisonFunction;
			IProgramNode* programNode;
			IStateGroupNode* groupNode;
		};
		const std::array<DepthCompareCase, 10> comparisonCases = {{
		  {"CompareNever", "a black image because the red fullscreen quad always fails depth testing", IStateGroupNode::DepthComparisonFunction::Never, equalDepthProgramNode.get(), equalDepthGroupNode.get()},
		  {"CompareEqual", "a full red image because the red fullscreen quad passes when depth equals the seeded value", IStateGroupNode::DepthComparisonFunction::Equal, equalDepthProgramNode.get(), equalDepthGroupNode.get()},
		  {"CompareNotEqual", "a black image because the red fullscreen quad fails when every pixel matches the seeded depth", IStateGroupNode::DepthComparisonFunction::NotEqual, equalDepthProgramNode.get(), equalDepthGroupNode.get()},
		  {"CompareLess", "a black image because the red fullscreen quad is not closer than the seeded depth", IStateGroupNode::DepthComparisonFunction::Less, equalDepthProgramNode.get(), equalDepthGroupNode.get()},
		  {"CompareLessPass", "a full red image because the red fullscreen quad is closer than the seeded depth", IStateGroupNode::DepthComparisonFunction::Less, closerDepthProgramNode.get(), closerDepthGroupNode.get()},
		  {"CompareLessOrEqual", "a full red image because the red fullscreen quad passes when depth is equal to the seeded value", IStateGroupNode::DepthComparisonFunction::LessOrEqual, equalDepthProgramNode.get(), equalDepthGroupNode.get()},
		  {"CompareGreater", "a black image because the red fullscreen quad is not further away than the seeded depth", IStateGroupNode::DepthComparisonFunction::Greater, equalDepthProgramNode.get(), equalDepthGroupNode.get()},
		  {"CompareGreaterPass", "a full red image because the red fullscreen quad is further away than the seeded depth", IStateGroupNode::DepthComparisonFunction::Greater, furtherDepthProgramNode.get(), furtherDepthGroupNode.get()},
		  {"CompareGreaterOrEqual", "a full red image because the red fullscreen quad passes when depth is equal to the seeded value", IStateGroupNode::DepthComparisonFunction::GreaterOrEqual, equalDepthProgramNode.get(), equalDepthGroupNode.get()},
		  {"CompareAlways", "a full red image because the red fullscreen quad always passes depth testing", IStateGroupNode::DepthComparisonFunction::Always, equalDepthProgramNode.get(), equalDepthGroupNode.get()},
		}};

		// Capture a frame for each depth comparison function
		auto frameBufferCapture = renderer.CreateFrameBufferOutput();
		for (const auto& comparisonCase : comparisonCases)
		{
			// Seed the depth buffer through the same raster path used by the equal-depth comparison draw. This keeps
			// equality tests independent of how a renderer quantizes depth clear values for normalized depth formats.
			blackDepthSeedGroupNode->SetDepthTestEnabled(true);
			blackDepthSeedGroupNode->SetDepthWriteEnabled(true);
			blackDepthSeedGroupNode->SetDepthComparisonFunction(IStateGroupNode::DepthComparisonFunction::Always);
			comparisonCase.groupNode->SetDepthTestEnabled(true);
			comparisonCase.groupNode->SetDepthWriteEnabled(true);
			comparisonCase.groupNode->SetDepthComparisonFunction(comparisonCase.comparisonFunction);
			renderPassNode->RemoveAllChildNodes();
			renderPassNode->SetAttachmentClearData(IFrameBuffer::AttachmentType::Depth, 0, V4Float32(1.0f, 0.0f, 0.0f, 0.0f));
			renderPassNode->AddChildNode(blackDepthSeedProgramNode.get());
			renderPassNode->AddChildNode(comparisonCase.programNode);
			frameBufferCapture = renderer.CreateFrameBufferOutput();
			frameBufferCapture->SetDetachAfterCapture(true);
			frameBuffer.AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
			DrawOneFrame();
			session.AddTestImageResult(prefix + comparisonCase.name, "The " + targetDescription + " image should be " + std::string(comparisonCase.visibleResult) + ".", std::move(frameBufferCapture), IImageDiff::Algorithm::AllOfThem, 0.95);
		}

		// Capture a case with depth testing disabled entirely
		equalDepthGroupNode->SetDepthTestEnabled(false);
		equalDepthGroupNode->SetDepthComparisonFunction(IStateGroupNode::DepthComparisonFunction::Never);
		renderPassNode->RemoveAllChildNodes();
		renderPassNode->SetAttachmentClearData(IFrameBuffer::AttachmentType::Depth, 0, V4Float32(1.0f, 0.0f, 0.0f, 0.0f));
		renderPassNode->AddChildNode(equalDepthProgramNode.get());
		frameBufferCapture = renderer.CreateFrameBufferOutput();
		frameBufferCapture->SetDetachAfterCapture(true);
		frameBuffer.AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
		DrawOneFrame();
		session.AddTestImageResult(prefix + "DepthTestDisabled", "The " + targetDescription + " image should be fully red because depth testing is disabled.", std::move(frameBufferCapture), IImageDiff::Algorithm::AllOfThem, 0.95);

		// Configure the three layered quads to demonstrate the effect of disabling depth writes
		blueBackgroundGroupNode->SetDepthTestEnabled(true);
		blueBackgroundGroupNode->SetDepthWriteEnabled(true);
		blueBackgroundGroupNode->SetDepthComparisonFunction(IStateGroupNode::DepthComparisonFunction::Always);
		redForegroundGroupNode->SetDepthTestEnabled(true);
		redForegroundGroupNode->SetDepthWriteEnabled(false);
		redForegroundGroupNode->SetDepthComparisonFunction(IStateGroupNode::DepthComparisonFunction::Always);
		greenMiddleGroupNode->SetDepthTestEnabled(true);
		greenMiddleGroupNode->SetDepthWriteEnabled(true);
		greenMiddleGroupNode->SetDepthComparisonFunction(IStateGroupNode::DepthComparisonFunction::Less);

		// Reconfigure the render pass for the depth write control test
		renderPassNode->RemoveAllChildNodes();
		renderPassNode->SetAttachmentClearData(IFrameBuffer::AttachmentType::Color, 0, V4Float32(0.0f, 0.0f, 0.0f, 1.0f));
		renderPassNode->SetAttachmentClearData(IFrameBuffer::AttachmentType::Depth, 0, V4Float32(1.0f, 0.0f, 0.0f, 0.0f));
		renderPassNode->AddChildNode(blueBackgroundProgramNode.get());
		renderPassNode->AddChildNode(redForegroundProgramNode.get());
		renderPassNode->AddChildNode(greenMiddleProgramNode.get());

		// Capture the layered result
		frameBufferCapture = renderer.CreateFrameBufferOutput();
		frameBufferCapture->SetDetachAfterCapture(true);
		frameBuffer.AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
		DrawOneFrame();
		session.AddTestImageResult(prefix + "DepthWriteDisabled", "The " + targetDescription + " image should show a blue background, a larger red square and a smaller green square in the center because the red layer does not update depth.", std::move(frameBufferCapture), IImageDiff::Algorithm::AllOfThem, 0.95);

		// Configure coplanar quads so the green quad only passes when depth bias pulls it closer.
		redDepthBiasBaselineGroupNode->SetDepthTestEnabled(true);
		redDepthBiasBaselineGroupNode->SetDepthWriteEnabled(true);
		redDepthBiasBaselineGroupNode->SetDepthComparisonFunction(IStateGroupNode::DepthComparisonFunction::Always);
		greenDepthBiasProbeGroupNode->SetDepthTestEnabled(true);
		greenDepthBiasProbeGroupNode->SetDepthWriteEnabled(true);
		greenDepthBiasProbeGroupNode->SetDepthComparisonFunction(IStateGroupNode::DepthComparisonFunction::Less);
		greenDepthBiasProbeGroupNode->ClearDepthBias();

		// Reconfigure the render pass for the depth bias lifecycle test.
		renderPassNode->RemoveAllChildNodes();
		renderPassNode->SetAttachmentClearData(IFrameBuffer::AttachmentType::Color, 0, V4Float32(0.0f, 0.0f, 0.0f, 1.0f));
		renderPassNode->SetAttachmentClearData(IFrameBuffer::AttachmentType::Depth, 0, V4Float32(1.0f, 0.0f, 0.0f, 0.0f));
		renderPassNode->AddChildNode(redDepthBiasBaselineProgramNode.get());
		renderPassNode->AddChildNode(greenDepthBiasProbeProgramNode.get());

		// With no bias, the coplanar green quad should fail the strict less-than depth test.
		frameBufferCapture = renderer.CreateFrameBufferOutput();
		frameBufferCapture->SetDetachAfterCapture(true);
		frameBuffer.AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
		DrawOneFrame();
		session.AddTestImageResult(prefix + "DepthBiasBaseline", "The " + targetDescription + " image should be fully red because the coplanar green quad has no depth bias and fails the strict less-than depth test.", std::move(frameBufferCapture), IImageDiff::Algorithm::AllOfThem, 0.95);

		// Applying a negative constant bias should pull the green quad closer so it visibly replaces the red baseline.
		greenDepthBiasProbeGroupNode->SetDepthBias(-10000.0f, 0.0f, 0.0f);
		frameBufferCapture = renderer.CreateFrameBufferOutput();
		frameBufferCapture->SetDetachAfterCapture(true);
		frameBuffer.AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
		DrawOneFrame();
		session.AddTestImageResult(prefix + "DepthBiasApplied", "The " + targetDescription + " image should be fully green because negative depth bias pulls the coplanar green quad closer than the red baseline.", std::move(frameBufferCapture), IImageDiff::Algorithm::AllOfThem, 0.95);

		// Clearing the bias should restore the original red baseline image.
		greenDepthBiasProbeGroupNode->ClearDepthBias();
		frameBufferCapture = renderer.CreateFrameBufferOutput();
		frameBufferCapture->SetDetachAfterCapture(true);
		frameBuffer.AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
		DrawOneFrame();
		session.AddTestImageResult(prefix + "DepthBiasCleared", "The " + targetDescription + " image should be fully red again after clearing depth bias from the green quad.", std::move(frameBufferCapture), IImageDiff::Algorithm::AllOfThem, 0.95);

		// Depth bias clamp is optional; keep the non-clamp coverage above active even when this feature is absent.
		if (!depthBiasClampSupported)
		{
			session.AddTestSkipped(prefix + "DepthBiasClamp", "The " + targetDescription + " depth-bias-clamp check was skipped because the current device does not support depth bias clamp.");
		}
		else
		{
			// Configure a green quad slightly behind the red baseline, so only a large unclamped bias makes it pass.
			// The separation is far enough above a 16-bit normalized depth step to survive fallback window formats,
			// while the clamp remains below that separation so the clamped case still fails the strict less test.
			greenDepthBiasClampProbeGroupNode->SetDepthTestEnabled(true);
			greenDepthBiasClampProbeGroupNode->SetDepthWriteEnabled(true);
			greenDepthBiasClampProbeGroupNode->SetDepthComparisonFunction(IStateGroupNode::DepthComparisonFunction::Less);
			greenDepthBiasClampProbeGroupNode->ClearDepthBias();

			renderPassNode->RemoveAllChildNodes();
			renderPassNode->SetAttachmentClearData(IFrameBuffer::AttachmentType::Color, 0, V4Float32(0.0f, 0.0f, 0.0f, 1.0f));
			renderPassNode->SetAttachmentClearData(IFrameBuffer::AttachmentType::Depth, 0, V4Float32(1.0f, 0.0f, 0.0f, 0.0f));
			renderPassNode->AddChildNode(redDepthBiasBaselineProgramNode.get());
			renderPassNode->AddChildNode(greenDepthBiasClampProbeProgramNode.get());

			// With no clamp, the large negative bias is enough to pull the slightly-behind green quad in front.
			greenDepthBiasClampProbeGroupNode->SetDepthBias(DepthBiasClampUnclampedBias, 0.0f, 0.0f);
			frameBufferCapture = renderer.CreateFrameBufferOutput();
			frameBufferCapture->SetDetachAfterCapture(true);
			frameBuffer.AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
			DrawOneFrame();
			session.AddTestImageResult(prefix + "DepthBiasClampUnclampedReference", "The " + targetDescription + " image should be fully green because the large negative bias is not clamped and pulls the slightly-behind green quad in front.", std::move(frameBufferCapture), IImageDiff::Algorithm::AllOfThem, 0.95);

			// Clamping the same negative bias to a small value should leave the green quad behind the red baseline.
			greenDepthBiasClampProbeGroupNode->SetDepthBias(DepthBiasClampUnclampedBias, 0.0f, DepthBiasClampLimit);
			frameBufferCapture = renderer.CreateFrameBufferOutput();
			frameBufferCapture->SetDetachAfterCapture(true);
			frameBuffer.AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
			DrawOneFrame();
			session.AddTestImageResult(prefix + "DepthBiasClampApplied", "The " + targetDescription + " image should be fully red because depth bias clamp limits the negative bias before it can pull the slightly-behind green quad in front.", std::move(frameBufferCapture), IImageDiff::Algorithm::AllOfThem, 0.95);

			greenDepthBiasClampProbeGroupNode->ClearDepthBias();
		}
	};

	// Run the depth state test sequence against each supported window depth/stencil mode
	const std::array<IFrameBuffer::WindowDepthStencilMode, 5> windowModes = {{
	  IFrameBuffer::WindowDepthStencilMode::DepthUNorm16,
	  IFrameBuffer::WindowDepthStencilMode::DepthUNorm24,
	  IFrameBuffer::WindowDepthStencilMode::DepthUNorm24StencilUInt8,
	  IFrameBuffer::WindowDepthStencilMode::DepthFloat32,
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

	// Repeat the same logical set of tests using an offscreen framebuffer
	V2UInt32 offscreenSize = session.TestWindowSize();
	auto colorTexture = renderer.CreateTextureBuffer2D();
	colorTexture->SetUsageFlags(ITextureBuffer::UsageFlags::FrameBufferOutput);
	colorTexture->SetTextureFormat(ITextureBuffer::ImageFormat::RGBA, ITextureBuffer::DataFormat::UNorm8);
	colorTexture->SetTextureDimensions(offscreenSize);
	REQUIRE(colorTexture->AllocateMemory());

	auto depthTexture = renderer.CreateTextureBuffer2D();
	depthTexture->SetUsageFlags(ITextureBuffer::UsageFlags::FrameBufferOutput);
	depthTexture->SetTextureFormat(ITextureBuffer::ImageFormat::Depth, ITextureBuffer::DataFormat::DepthFloat32);
	depthTexture->SetTextureDimensions(offscreenSize);
	REQUIRE(depthTexture->AllocateMemory());

	auto offscreenFrameBuffer = renderer.CreateFrameBuffer();
	offscreenFrameBuffer->DefineViewportRegion(V2UInt32(0, 0), offscreenSize);
	REQUIRE(offscreenFrameBuffer->BindTexture(colorTexture.get(), IFrameBuffer::AttachmentType::Color));
	REQUIRE(offscreenFrameBuffer->BindTexture(depthTexture.get(), IFrameBuffer::AttachmentType::Depth));
	runSequence(*offscreenFrameBuffer, "Offscreen", "offscreen");

	// Remove all our defined render passes
	renderer.RemoveAllRenderPasses();
	return true;
}

} // namespace cobalt::graphics::testing
