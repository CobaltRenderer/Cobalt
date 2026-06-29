// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "../UnitTestBase.h"
#include <array>
namespace cobalt::graphics::testing {
using namespace cobalt::graphics;

namespace {

// Define our shader programs
const std::string FullscreenVertexShader = R"(
struct VSInput {
    float4 position : position;
    float2 texCoord : texCoord;
};

struct VSOutput {
    float4 position : SV_POSITION;
    float2 texCoord : texCoord;
};

VSOutput main(VSInput IN)
{
    VSOutput OUT;
    OUT.position = IN.position;
    OUT.texCoord = IN.texCoord;
    return OUT;
}
)";
const std::string PositionOnlyVertexShader = R"(
struct VSInput {
    float4 position : position;
};

float4 main(VSInput IN) : SV_POSITION
{
    return IN.position;
}
)";
const std::string TextureFragmentShader = R"(
uniform Texture2D colorTexture;
uniform SamplerState colorTexture_CombinedSampler;

struct VSOutput {
    float4 position : SV_POSITION;
    float2 texCoord : texCoord;
};

float4 main(VSOutput IN) : SV_TARGET0
{
    return colorTexture.Sample(colorTexture_CombinedSampler, IN.texCoord);
}
)";
const std::string CubeTextureFragmentShader = R"(
uniform TextureCube colorTexture;
uniform SamplerState colorTexture_CombinedSampler;

float4 main() : SV_TARGET0
{
    return colorTexture.Sample(colorTexture_CombinedSampler, float3(0.0f, 0.0f, 1.0f));
}
)";
const std::string SeparateSamplerFragmentShader = R"(
uniform Texture2D colorTexture;
uniform SamplerState colorSampler;

struct VSOutput {
    float4 position : SV_POSITION;
    float2 texCoord : texCoord;
};

float4 main(VSOutput IN) : SV_TARGET0
{
    return colorTexture.Sample(colorSampler, IN.texCoord);
}
)";
const std::string ArraySeparateSamplerFragmentShader = R"(
uniform Texture2DArray colorTextureArray;
uniform SamplerState colorSamplerArray;

float4 main() : SV_TARGET0
{
    return colorTextureArray.Sample(colorSamplerArray, float3(1.25f, 0.5f, 0.0f));
}
)";
const std::string StateBufferFragmentShader = R"(
struct ColorData {
    float4 color;
};

cbuffer ColorCBuffer
{
    ColorData colorData;
};

float4 main() : SV_TARGET0
{
    return colorData.color;
}
)";
const std::string ResourceArrayFragmentShader = R"(
StructuredBuffer<float4> colorArray;

float4 main() : SV_TARGET0
{
    return colorArray[0];
}
)";
const std::string SolidColorFragmentShader = R"(
uniform float4 drawColor;

float4 main() : SV_TARGET0
{
    return drawColor;
}
)";

IFrameBufferOutput::unique_ptr CreateColorCapture(IRenderer& renderer, IFrameBuffer& frameBuffer)
{
	auto frameBufferCapture = renderer.CreateFrameBufferOutput();
	frameBufferCapture->SetDetachAfterCapture(true);
	frameBuffer.AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
	return frameBufferCapture;
}

} // namespace

DEFINE_UNIT_TEST_WITH_BASE("Resources/StateValue/ResourceBindingLifecycle", UnitTestBase)
{
	// Retrieve our important objects
	auto& renderer = session.Renderer();
	auto& uiThread = session.UIThread();
	auto& testWindowInfo = *session.TestWindowPlatformInfo();

	// Create the shared fullscreen quad geometry used by each binding test.
	std::vector<V4Float32> positionVertexData;
	Geometry().CreateFullscreenQuad(0.5f, positionVertexData);
	std::vector<V2Float32> texCoordData = {{0.0f, 1.0f}, {0.0f, 0.0f}, {1.0f, 0.0f}, {0.0f, 1.0f}, {1.0f, 0.0f}, {1.0f, 1.0f}};

	// Invalid resource IDs are ignored before the resource itself is used, including unallocated resources.
	{
		auto invalidBindingNode = renderer.CreateRenderableNode();
		auto unallocatedTexture = renderer.CreateTextureBuffer2D();
		unallocatedTexture->SetUsageFlags(ITextureBuffer::UsageFlags::ShaderInput);
		unallocatedTexture->SetTextureFormat(ITextureBuffer::ImageFormat::RGBA, ITextureBuffer::DataFormat::UNorm8);
		unallocatedTexture->SetTextureDimensions(V2UInt32(1, 1));
		auto sampler = renderer.CreateTextureSampler2D();
		auto unallocatedStateBuffer = renderer.CreateStateBuffer();
		invalidBindingNode->BindTextureWithCombinedSampler(TextureId::Null, unallocatedTexture.get(), sampler.get());
		invalidBindingNode->BindTexture(TextureId::Null, unallocatedTexture.get());
		invalidBindingNode->BindSampler(SamplerId::Null, sampler.get());
		invalidBindingNode->BindStateBuffer(StateBufferId::Null, unallocatedStateBuffer.get());
		invalidBindingNode->UnbindTexture(TextureId::Null);
		invalidBindingNode->UnbindSampler(SamplerId::Null);
		invalidBindingNode->UnbindStateBuffer(StateBufferId::Null);
		if (session.Device().IsFeatureSupported(IGraphicsDevice::Feature::ResourceArrays))
		{
			auto unallocatedDataArray = renderer.CreateDataArray();
			auto unallocatedTexelArray = renderer.CreateTexelArray();
			invalidBindingNode->BindResourceArray(ResourceArrayId::Null, unallocatedDataArray.get());
			invalidBindingNode->BindResourceArray(ResourceArrayId::Null, unallocatedTexelArray.get());
			invalidBindingNode->UnbindResourceArray(ResourceArrayId::Null);
		}

		// Render a visible frame after the invalid binding checks so this contract case has a reference image.
		auto frameBuffer = renderer.CreateFrameBuffer();
		frameBuffer->DefineViewportRegion(V2UInt32(0, 0), session.TestWindowSize());
		REQUIRE(uiThread.InvokeSync([&] { return frameBuffer->BindWindow(testWindowInfo, IFrameBuffer::WindowDepthStencilMode::None); }));

		auto shaderProgram = renderer.CreateShaderProgram();
		REQUIRE(shaderProgram->LoadShaderStage(IShaderProgram::ShaderStage::Vertex, ShaderSourceInfoHLSL(PositionOnlyVertexShader)));
		REQUIRE(shaderProgram->LoadShaderStage(IShaderProgram::ShaderStage::Fragment, ShaderSourceInfoHLSL(SolidColorFragmentShader)));
		REQUIRE(shaderProgram->CompileProgram());
		auto positionAttributeId = shaderProgram->GetVertexAttributeId("position");
		auto drawColorStateId = shaderProgram->GetStateValueId("drawColor");

		VertexAttribute<V4Float32> vertexAttributePosition(positionVertexData.size(), IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadNever, IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadOften);
		auto vertexBuffer = renderer.CreateVertexBuffer();
		REQUIRE(vertexBuffer->BindVertexAttribute(vertexAttributePosition));
		REQUIRE(vertexAttributePosition.SetInitialData(positionVertexData));
		REQUIRE(vertexBuffer->AllocateMemory());

		auto renderableNode = renderer.CreateRenderableNode();
		REQUIRE(renderableNode->BindVertexAttribute(vertexAttributePosition, positionAttributeId));
		REQUIRE(renderableNode->SetPrimitiveMode(IRenderableNode::PrimitiveMode::Triangles));
		renderableNode->SetStateValue(drawColorStateId, V4Float32(0.75f, 0.25f, 0.75f, 1.0f));

		auto groupNode = renderer.CreateStateGroupNode();
		groupNode->AddChildNode(renderableNode.get());

		auto programNode = renderer.CreateProgramNode();
		REQUIRE(programNode->BindShaderProgram(shaderProgram.get()));
		programNode->AddChildNode(groupNode.get());

		auto renderPassNode = renderer.CreateRenderPassNode();
		renderPassNode->BindFrameBuffer(frameBuffer.get());
		renderPassNode->SetAttachmentClearData(IFrameBuffer::AttachmentType::Color, 0, V4Float32(0.0f, 0.0f, 0.0f, 1.0f));
		renderPassNode->AddChildNode(programNode.get());
		renderer.SetRenderPasses(&renderPassNode, 1);

		auto frameBufferCapture = CreateColorCapture(renderer, *frameBuffer);
		DrawOneFrame();
		session.AddTestImageResult("InvalidResourceBindingContracts", "A fullscreen purple quad rendered after invalid resource binding contracts ignored null resource IDs.", std::move(frameBufferCapture), IImageDiff::Algorithm::NaiveDiff, 0.98);
		renderer.RemoveAllRenderPasses();
	}
	renderer.WaitForDeferredDeletionComplete();

	// Test texture unbinding by falling back to a default-state texture binding.
	{
		// Create the framebuffer used by this binding case.
		auto frameBuffer = renderer.CreateFrameBuffer();
		frameBuffer->DefineViewportRegion(V2UInt32(0, 0), session.TestWindowSize());
		REQUIRE(uiThread.InvokeSync([&] { return frameBuffer->BindWindow(testWindowInfo, IFrameBuffer::WindowDepthStencilMode::None); }));

		// Create and compile the shader program, then retrieve the resource IDs it exposes.
		auto shaderProgram = renderer.CreateShaderProgram();
		REQUIRE(shaderProgram->LoadShaderStage(IShaderProgram::ShaderStage::Vertex, ShaderSourceInfoHLSL(FullscreenVertexShader)));
		REQUIRE(shaderProgram->LoadShaderStage(IShaderProgram::ShaderStage::Fragment, ShaderSourceInfoHLSL(TextureFragmentShader)));
		REQUIRE(shaderProgram->CompileProgram());
		auto positionAttributeId = shaderProgram->GetVertexAttributeId("position");
		auto texCoordAttributeId = shaderProgram->GetVertexAttributeId("texCoord");
		auto colorTextureId = shaderProgram->GetTextureId("colorTexture");

		// Create the vertex buffer used to draw the fullscreen textured quad.
		VertexAttribute<V4Float32> vertexAttributePosition(positionVertexData.size(), IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadNever, IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadOften);
		VertexAttribute<V2Float32> vertexAttributeTexCoord(texCoordData.size(), IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadNever, IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadOften);
		auto vertexBuffer = renderer.CreateVertexBuffer();
		REQUIRE(vertexBuffer->BindVertexAttribute(vertexAttributePosition));
		REQUIRE(vertexAttributePosition.SetInitialData(positionVertexData));
		REQUIRE(vertexBuffer->BindVertexAttribute(vertexAttributeTexCoord));
		REQUIRE(vertexAttributeTexCoord.SetInitialData(texCoordData));
		REQUIRE(vertexBuffer->AllocateMemory());

		// Create two single-pixel textures so the active binding source is visible in the captured image.
		auto redTexture = renderer.CreateTextureBuffer2D();
		redTexture->SetUsageFlags(ITextureBuffer::UsageFlags::ShaderInput);
		redTexture->SetTextureFormat(ITextureBuffer::ImageFormat::RGBA, ITextureBuffer::DataFormat::UNorm8);
		redTexture->SetTextureDimensions(V2UInt32(1, 1));
		std::vector<V4UInt8> redTextureData = {V4UInt8(255, 0, 0, 255)};
		REQUIRE(redTexture->SetInitialData(redTextureData));
		REQUIRE(redTexture->AllocateMemory());

		auto greenTexture = renderer.CreateTextureBuffer2D();
		greenTexture->SetUsageFlags(ITextureBuffer::UsageFlags::ShaderInput);
		greenTexture->SetTextureFormat(ITextureBuffer::ImageFormat::RGBA, ITextureBuffer::DataFormat::UNorm8);
		greenTexture->SetTextureDimensions(V2UInt32(1, 1));
		std::vector<V4UInt8> greenTextureData = {V4UInt8(0, 255, 0, 255)};
		REQUIRE(greenTexture->SetInitialData(greenTextureData));
		REQUIRE(greenTexture->AllocateMemory());

		// Create a sampler for the combined texture/sampler binding.
		auto sampler = renderer.CreateTextureSampler2D();
		sampler->SetTextureFilterMode(ITextureSampler::FilterMode::Nearest, ITextureSampler::FilterMode::Nearest);
		sampler->SetTextureWrapMode(ITextureSampler::WrapMode::ClampToEdge, ITextureSampler::WrapMode::ClampToEdge);

		// Create a renderable binding to the green texture.
		auto renderableNode = renderer.CreateRenderableNode();
		REQUIRE(renderableNode->BindVertexAttribute(vertexAttributePosition, positionAttributeId));
		REQUIRE(renderableNode->BindVertexAttribute(vertexAttributeTexCoord, texCoordAttributeId));
		REQUIRE(renderableNode->SetPrimitiveMode(IRenderableNode::PrimitiveMode::Triangles));
		renderableNode->BindTextureWithCombinedSampler(colorTextureId, greenTexture.get(), sampler.get());

		auto groupNode = renderer.CreateStateGroupNode();
		groupNode->AddChildNode(renderableNode.get());

		auto programNode = renderer.CreateProgramNode();
		REQUIRE(programNode->BindShaderProgram(shaderProgram.get()));
		programNode->AddChildNode(groupNode.get());

		// Create a default state binding to the red texture for fallback after the renderable binding is removed.
		auto defaultState = renderer.CreateDefaultState();
		defaultState->BindTextureWithCombinedSampler(colorTextureId, redTexture.get(), sampler.get());

		// Bind the render tree to the renderer.
		auto renderPassNode = renderer.CreateRenderPassNode();
		renderPassNode->BindFrameBuffer(frameBuffer.get());
		renderPassNode->SetAttachmentClearData(IFrameBuffer::AttachmentType::Color, 0, V4Float32(0.0f, 0.0f, 0.0f, 1.0f));
		renderPassNode->AddChildNode(programNode.get(), defaultState.get());
		renderer.SetRenderPasses(&renderPassNode, 1);

		// The renderable texture binding should override the default-state binding.
		auto frameBufferCapture = renderer.CreateFrameBufferOutput();
		frameBufferCapture->SetDetachAfterCapture(true);
		frameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
		DrawOneFrame();
		session.AddTestImageResult("TextureRenderableBinding", "A fullscreen green quad sampled from the renderable texture binding.", std::move(frameBufferCapture), IImageDiff::Algorithm::AllOfThem, 0.95);

		// Unbinding the renderable texture should reveal the default-state binding.
		renderableNode->UnbindTexture(colorTextureId);
		frameBufferCapture = renderer.CreateFrameBufferOutput();
		frameBufferCapture->SetDetachAfterCapture(true);
		frameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
		DrawOneFrame();
		session.AddTestImageResult("TextureDefaultBindingAfterUnbind", "A fullscreen red quad sampled from the default-state texture binding after unbinding the renderable texture.", std::move(frameBufferCapture), IImageDiff::Algorithm::AllOfThem, 0.95);
		renderer.RemoveAllRenderPasses();
	}
	renderer.WaitForDeferredDeletionComplete();

	// Test cubemap combined texture/sampler unbinding through the same default-state fallback path.
	{
		// Create the framebuffer used by this binding case.
		auto frameBuffer = renderer.CreateFrameBuffer();
		frameBuffer->DefineViewportRegion(V2UInt32(0, 0), session.TestWindowSize());
		REQUIRE(uiThread.InvokeSync([&] { return frameBuffer->BindWindow(testWindowInfo, IFrameBuffer::WindowDepthStencilMode::None); }));

		// Create and compile the shader program, then retrieve the resource IDs it exposes.
		auto shaderProgram = renderer.CreateShaderProgram();
		REQUIRE(shaderProgram->LoadShaderStage(IShaderProgram::ShaderStage::Vertex, ShaderSourceInfoHLSL(PositionOnlyVertexShader)));
		REQUIRE(shaderProgram->LoadShaderStage(IShaderProgram::ShaderStage::Fragment, ShaderSourceInfoHLSL(CubeTextureFragmentShader)));
		REQUIRE(shaderProgram->CompileProgram());
		auto positionAttributeId = shaderProgram->GetVertexAttributeId("position");
		auto colorTextureId = shaderProgram->GetTextureId("colorTexture");

		// Create the vertex buffer used to draw the fullscreen quad.
		VertexAttribute<V4Float32> vertexAttributePosition(positionVertexData.size(), IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadNever, IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadOften);
		auto vertexBuffer = renderer.CreateVertexBuffer();
		REQUIRE(vertexBuffer->BindVertexAttribute(vertexAttributePosition));
		REQUIRE(vertexAttributePosition.SetInitialData(positionVertexData));
		REQUIRE(vertexBuffer->AllocateMemory());

		// Create two complete single-pixel cubemaps so the combined binding has a valid resource on every face.
		const std::array<ITextureBuffer::CubeMapFace, 6> cubeFaces = {
		  ITextureBuffer::CubeMapFace::PositiveX,
		  ITextureBuffer::CubeMapFace::NegativeX,
		  ITextureBuffer::CubeMapFace::PositiveY,
		  ITextureBuffer::CubeMapFace::NegativeY,
		  ITextureBuffer::CubeMapFace::PositiveZ,
		  ITextureBuffer::CubeMapFace::NegativeZ};
		std::vector<V4UInt8> redFaceData = {V4UInt8(255, 0, 0, 255)};
		std::vector<V4UInt8> greenFaceData = {V4UInt8(0, 255, 0, 255)};
		auto redTexture = renderer.CreateTextureBufferCube();
		redTexture->SetUsageFlags(ITextureBuffer::UsageFlags::ShaderInput);
		redTexture->SetTextureFormat(ITextureBuffer::ImageFormat::RGBA, ITextureBuffer::DataFormat::UNorm8);
		redTexture->SetTextureDimensions(1);
		auto greenTexture = renderer.CreateTextureBufferCube();
		greenTexture->SetUsageFlags(ITextureBuffer::UsageFlags::ShaderInput);
		greenTexture->SetTextureFormat(ITextureBuffer::ImageFormat::RGBA, ITextureBuffer::DataFormat::UNorm8);
		greenTexture->SetTextureDimensions(1);
		for (auto cubeFace : cubeFaces)
		{
			REQUIRE(redTexture->SetInitialData(redFaceData, cubeFace));
			REQUIRE(greenTexture->SetInitialData(greenFaceData, cubeFace));
		}
		REQUIRE(redTexture->AllocateMemory());
		REQUIRE(greenTexture->AllocateMemory());

		// Create a sampler for the combined cubemap binding.
		auto sampler = renderer.CreateTextureSamplerCube();
		sampler->SetTextureFilterMode(ITextureSampler::FilterMode::Nearest, ITextureSampler::FilterMode::Nearest);
		sampler->SetTextureWrapMode(ITextureSampler::WrapMode::ClampToEdge, ITextureSampler::WrapMode::ClampToEdge);

		// Create a renderable binding to the green cubemap.
		auto renderableNode = renderer.CreateRenderableNode();
		REQUIRE(renderableNode->BindVertexAttribute(vertexAttributePosition, positionAttributeId));
		REQUIRE(renderableNode->SetPrimitiveMode(IRenderableNode::PrimitiveMode::Triangles));
		renderableNode->BindTextureWithCombinedSampler(colorTextureId, greenTexture.get(), sampler.get());

		auto groupNode = renderer.CreateStateGroupNode();
		groupNode->AddChildNode(renderableNode.get());

		auto programNode = renderer.CreateProgramNode();
		REQUIRE(programNode->BindShaderProgram(shaderProgram.get()));
		programNode->AddChildNode(groupNode.get());

		// Create a default state binding to the red cubemap for fallback after the renderable binding is removed.
		auto defaultState = renderer.CreateDefaultState();
		defaultState->BindTextureWithCombinedSampler(colorTextureId, redTexture.get(), sampler.get());

		// Bind the render tree to the renderer.
		auto renderPassNode = renderer.CreateRenderPassNode();
		renderPassNode->BindFrameBuffer(frameBuffer.get());
		renderPassNode->SetAttachmentClearData(IFrameBuffer::AttachmentType::Color, 0, V4Float32(0.0f, 0.0f, 0.0f, 1.0f));
		renderPassNode->AddChildNode(programNode.get(), defaultState.get());
		renderer.SetRenderPasses(&renderPassNode, 1);

		// The renderable cubemap binding should override the default-state binding.
		auto frameBufferCapture = CreateColorCapture(renderer, *frameBuffer);
		DrawOneFrame();
		session.AddTestImageResult("CubeTextureRenderableBinding", "A fullscreen green quad sampled from the renderable cubemap binding.", std::move(frameBufferCapture), IImageDiff::Algorithm::AllOfThem, 0.95);

		// Unbinding the renderable cubemap should reveal the default-state binding.
		renderableNode->UnbindTexture(colorTextureId);
		frameBufferCapture = CreateColorCapture(renderer, *frameBuffer);
		DrawOneFrame();
		session.AddTestImageResult("CubeTextureDefaultBindingAfterUnbind", "A fullscreen red quad sampled from the default-state cubemap binding after unbinding the renderable cubemap.", std::move(frameBufferCapture), IImageDiff::Algorithm::AllOfThem, 0.95);
		renderer.RemoveAllRenderPasses();
	}
	renderer.WaitForDeferredDeletionComplete();

	// Test sampler unbinding by falling back to a default-state sampler binding.
	if (!session.Device().IsFeatureSupported(IGraphicsDevice::Feature::SeparateTextureSamplers))
	{
		session.AddTestSkipped("SamplerUnbindFallback", "This part of the test was skipped, as the current renderer doesn't support separate texture samplers on this device.");
	}
	else
	{
		// Create the framebuffer used by this binding case.
		auto frameBuffer = renderer.CreateFrameBuffer();
		frameBuffer->DefineViewportRegion(V2UInt32(0, 0), session.TestWindowSize());
		REQUIRE(uiThread.InvokeSync([&] { return frameBuffer->BindWindow(testWindowInfo, IFrameBuffer::WindowDepthStencilMode::None); }));

		// Create and compile the shader program, then retrieve the separate texture and sampler IDs it exposes.
		auto shaderProgram = renderer.CreateShaderProgram();
		REQUIRE(shaderProgram->LoadShaderStage(IShaderProgram::ShaderStage::Vertex, ShaderSourceInfoHLSL(FullscreenVertexShader)));
		REQUIRE(shaderProgram->LoadShaderStage(IShaderProgram::ShaderStage::Fragment, ShaderSourceInfoHLSL(SeparateSamplerFragmentShader)));
		REQUIRE(shaderProgram->CompileProgram());
		auto positionAttributeId = shaderProgram->GetVertexAttributeId("position");
		auto texCoordAttributeId = shaderProgram->GetVertexAttributeId("texCoord");
		auto colorTextureId = shaderProgram->GetTextureId("colorTexture");
		auto colorSamplerId = shaderProgram->GetSamplerId("colorSampler");
		REQUIRE(colorSamplerId != SamplerId::Null);

		// Use out-of-range texture coordinates so repeat and clamp samplers produce different colors.
		std::vector<V2Float32> repeatProbeTexCoordData(positionVertexData.size(), V2Float32(1.25f, 0.5f));
		VertexAttribute<V4Float32> vertexAttributePosition(positionVertexData.size(), IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadNever, IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadOften);
		VertexAttribute<V2Float32> vertexAttributeTexCoord(repeatProbeTexCoordData.size(), IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadNever, IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadOften);
		auto vertexBuffer = renderer.CreateVertexBuffer();
		REQUIRE(vertexBuffer->BindVertexAttribute(vertexAttributePosition));
		REQUIRE(vertexAttributePosition.SetInitialData(positionVertexData));
		REQUIRE(vertexBuffer->BindVertexAttribute(vertexAttributeTexCoord));
		REQUIRE(vertexAttributeTexCoord.SetInitialData(repeatProbeTexCoordData));
		REQUIRE(vertexBuffer->AllocateMemory());

		// Create a two-pixel texture where repeat and clamp sampling at the probe coordinate diverge.
		auto texture = renderer.CreateTextureBuffer2D();
		texture->SetUsageFlags(ITextureBuffer::UsageFlags::ShaderInput);
		texture->SetTextureFormat(ITextureBuffer::ImageFormat::RGBA, ITextureBuffer::DataFormat::UNorm8);
		texture->SetTextureDimensions(V2UInt32(2, 1));
		std::vector<V4UInt8> textureData = {V4UInt8(255, 0, 0, 255), V4UInt8(0, 255, 0, 255)};
		REQUIRE(texture->SetInitialData(textureData));
		REQUIRE(texture->AllocateMemory());

		// Create the samplers used by the renderable binding and the default-state fallback.
		auto repeatSampler = renderer.CreateTextureSampler2D();
		repeatSampler->SetTextureFilterMode(ITextureSampler::FilterMode::Nearest, ITextureSampler::FilterMode::Nearest);
		repeatSampler->SetTextureWrapMode(ITextureSampler::WrapMode::Repeat, ITextureSampler::WrapMode::ClampToEdge);
		auto clampSampler = renderer.CreateTextureSampler2D();
		clampSampler->SetTextureFilterMode(ITextureSampler::FilterMode::Nearest, ITextureSampler::FilterMode::Nearest);
		clampSampler->SetTextureWrapMode(ITextureSampler::WrapMode::ClampToEdge, ITextureSampler::WrapMode::ClampToEdge);

		// Create a renderable binding to the repeat sampler.
		auto renderableNode = renderer.CreateRenderableNode();
		REQUIRE(renderableNode->BindVertexAttribute(vertexAttributePosition, positionAttributeId));
		REQUIRE(renderableNode->BindVertexAttribute(vertexAttributeTexCoord, texCoordAttributeId));
		REQUIRE(renderableNode->SetPrimitiveMode(IRenderableNode::PrimitiveMode::Triangles));
		renderableNode->BindTexture(colorTextureId, texture.get());
		renderableNode->BindSampler(colorSamplerId, repeatSampler.get());

		auto groupNode = renderer.CreateStateGroupNode();
		groupNode->AddChildNode(renderableNode.get());

		auto programNode = renderer.CreateProgramNode();
		REQUIRE(programNode->BindShaderProgram(shaderProgram.get()));
		programNode->AddChildNode(groupNode.get());

		// Create a default state binding to the clamp sampler for fallback after the renderable sampler is removed.
		auto defaultState = renderer.CreateDefaultState();
		defaultState->BindSampler(colorSamplerId, clampSampler.get());

		// Bind the render tree to the renderer.
		auto renderPassNode = renderer.CreateRenderPassNode();
		renderPassNode->BindFrameBuffer(frameBuffer.get());
		renderPassNode->SetAttachmentClearData(IFrameBuffer::AttachmentType::Color, 0, V4Float32(0.0f, 0.0f, 0.0f, 1.0f));
		renderPassNode->AddChildNode(programNode.get(), defaultState.get());
		renderer.SetRenderPasses(&renderPassNode, 1);

		// The renderable sampler binding should override the default-state sampler.
		auto frameBufferCapture = renderer.CreateFrameBufferOutput();
		frameBufferCapture->SetDetachAfterCapture(true);
		frameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
		DrawOneFrame();
		session.AddTestImageResult("SamplerRenderableBinding", "A fullscreen red quad sampled using the renderable repeat sampler binding.", std::move(frameBufferCapture), IImageDiff::Algorithm::AllOfThem, 0.95);

		// Unbinding the renderable sampler should reveal the default-state sampler.
		renderableNode->UnbindSampler(colorSamplerId);
		frameBufferCapture = renderer.CreateFrameBufferOutput();
		frameBufferCapture->SetDetachAfterCapture(true);
		frameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
		DrawOneFrame();
		session.AddTestImageResult("SamplerDefaultBindingAfterUnbind", "A fullscreen green quad sampled using the default-state clamp sampler after unbinding the renderable sampler.", std::move(frameBufferCapture), IImageDiff::Algorithm::AllOfThem, 0.95);
		renderer.RemoveAllRenderPasses();
	}
	renderer.WaitForDeferredDeletionComplete();

	// Test 2D texture-array sampler unbinding for the separate sampler binding path.
	if (!session.Device().IsFeatureSupported(IGraphicsDevice::Feature::SeparateTextureSamplers))
	{
		session.AddTestSkipped("ArraySamplerUnbindFallback", "This part of the test was skipped, as the current renderer doesn't support separate texture samplers on this device.");
	}
	else
	{
		// Create the framebuffer used by this binding case.
		auto frameBuffer = renderer.CreateFrameBuffer();
		frameBuffer->DefineViewportRegion(V2UInt32(0, 0), session.TestWindowSize());
		REQUIRE(uiThread.InvokeSync([&] { return frameBuffer->BindWindow(testWindowInfo, IFrameBuffer::WindowDepthStencilMode::None); }));

		// Create and compile the shader program, then retrieve the separate texture-array and sampler IDs it exposes.
		auto shaderProgram = renderer.CreateShaderProgram();
		REQUIRE(shaderProgram->LoadShaderStage(IShaderProgram::ShaderStage::Vertex, ShaderSourceInfoHLSL(PositionOnlyVertexShader)));
		REQUIRE(shaderProgram->LoadShaderStage(IShaderProgram::ShaderStage::Fragment, ShaderSourceInfoHLSL(ArraySeparateSamplerFragmentShader)));
		REQUIRE(shaderProgram->CompileProgram());
		auto positionAttributeId = shaderProgram->GetVertexAttributeId("position");
		auto colorTextureId = shaderProgram->GetTextureId("colorTextureArray");
		auto colorSamplerId = shaderProgram->GetSamplerId("colorSamplerArray");
		REQUIRE(colorSamplerId != SamplerId::Null);

		// Create the vertex buffer used to draw the fullscreen quad.
		VertexAttribute<V4Float32> vertexAttributePosition(positionVertexData.size(), IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadNever, IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadOften);
		auto vertexBuffer = renderer.CreateVertexBuffer();
		REQUIRE(vertexBuffer->BindVertexAttribute(vertexAttributePosition));
		REQUIRE(vertexAttributePosition.SetInitialData(positionVertexData));
		REQUIRE(vertexBuffer->AllocateMemory());

		// Create a single-layer texture array where repeat and clamp sampling at the probe coordinate diverge.
		auto texture = renderer.CreateTextureBuffer2DArray();
		texture->SetUsageFlags(ITextureBuffer::UsageFlags::ShaderInput);
		texture->SetTextureFormat(ITextureBuffer::ImageFormat::RGBA, ITextureBuffer::DataFormat::UNorm8);
		texture->SetTextureDimensions(V2UInt32(2, 1), 1);
		std::vector<V4UInt8> textureData = {V4UInt8(255, 0, 0, 255), V4UInt8(0, 255, 0, 255)};
		REQUIRE(texture->SetInitialData(textureData, 0));
		REQUIRE(texture->AllocateMemory());

		// Create the samplers used by the renderable binding and the default-state fallback.
		auto repeatSampler = renderer.CreateTextureSampler2DArray();
		repeatSampler->SetTextureFilterMode(ITextureSampler::FilterMode::Nearest, ITextureSampler::FilterMode::Nearest);
		repeatSampler->SetTextureWrapMode(ITextureSampler::WrapMode::Repeat, ITextureSampler::WrapMode::ClampToEdge);
		auto clampSampler = renderer.CreateTextureSampler2DArray();
		clampSampler->SetTextureFilterMode(ITextureSampler::FilterMode::Nearest, ITextureSampler::FilterMode::Nearest);
		clampSampler->SetTextureWrapMode(ITextureSampler::WrapMode::ClampToEdge, ITextureSampler::WrapMode::ClampToEdge);

		// Create a renderable binding to the repeat sampler.
		auto renderableNode = renderer.CreateRenderableNode();
		REQUIRE(renderableNode->BindVertexAttribute(vertexAttributePosition, positionAttributeId));
		REQUIRE(renderableNode->SetPrimitiveMode(IRenderableNode::PrimitiveMode::Triangles));
		renderableNode->BindTexture(colorTextureId, texture.get());
		renderableNode->BindSampler(colorSamplerId, repeatSampler.get());

		auto groupNode = renderer.CreateStateGroupNode();
		groupNode->AddChildNode(renderableNode.get());

		auto programNode = renderer.CreateProgramNode();
		REQUIRE(programNode->BindShaderProgram(shaderProgram.get()));
		programNode->AddChildNode(groupNode.get());

		// Create a default state binding to the clamp sampler for fallback after the renderable sampler is removed.
		auto defaultState = renderer.CreateDefaultState();
		defaultState->BindSampler(colorSamplerId, clampSampler.get());

		// Bind the render tree to the renderer.
		auto renderPassNode = renderer.CreateRenderPassNode();
		renderPassNode->BindFrameBuffer(frameBuffer.get());
		renderPassNode->SetAttachmentClearData(IFrameBuffer::AttachmentType::Color, 0, V4Float32(0.0f, 0.0f, 0.0f, 1.0f));
		renderPassNode->AddChildNode(programNode.get(), defaultState.get());
		renderer.SetRenderPasses(&renderPassNode, 1);

		// The renderable array sampler should override the default-state sampler.
		auto frameBufferCapture = CreateColorCapture(renderer, *frameBuffer);
		DrawOneFrame();
		session.AddTestImageResult("ArraySamplerRenderableBinding", "A fullscreen red quad sampled using the renderable repeat sampler bound to a texture array.", std::move(frameBufferCapture), IImageDiff::Algorithm::AllOfThem, 0.95);

		// Unbinding the renderable array sampler should reveal the default-state sampler.
		renderableNode->UnbindSampler(colorSamplerId);
		frameBufferCapture = CreateColorCapture(renderer, *frameBuffer);
		DrawOneFrame();
		session.AddTestImageResult("ArraySamplerDefaultBindingAfterUnbind", "A fullscreen green quad sampled using the default-state clamp sampler after unbinding the renderable texture-array sampler.", std::move(frameBufferCapture), IImageDiff::Algorithm::AllOfThem, 0.95);
		renderer.RemoveAllRenderPasses();
	}
	renderer.WaitForDeferredDeletionComplete();

	// Test state-buffer unbinding by falling back to a default-state state buffer.
	{
		// Create the framebuffer used by this binding case.
		auto frameBuffer = renderer.CreateFrameBuffer();
		frameBuffer->DefineViewportRegion(V2UInt32(0, 0), session.TestWindowSize());
		REQUIRE(uiThread.InvokeSync([&] { return frameBuffer->BindWindow(testWindowInfo, IFrameBuffer::WindowDepthStencilMode::None); }));

		// Create and compile the shader program, then retrieve the state-buffer ID it exposes.
		auto shaderProgram = renderer.CreateShaderProgram();
		REQUIRE(shaderProgram->LoadShaderStage(IShaderProgram::ShaderStage::Vertex, ShaderSourceInfoHLSL(PositionOnlyVertexShader)));
		REQUIRE(shaderProgram->LoadShaderStage(IShaderProgram::ShaderStage::Fragment, ShaderSourceInfoHLSL(StateBufferFragmentShader)));
		REQUIRE(shaderProgram->CompileProgram());
		auto positionAttributeId = shaderProgram->GetVertexAttributeId("position");
		auto colorBufferId = shaderProgram->GetStateBufferId("ColorCBuffer");

		// Create the vertex buffer used to draw the fullscreen quad.
		VertexAttribute<V4Float32> vertexAttributePosition(positionVertexData.size(), IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadNever, IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadOften);
		VertexAttribute<V2Float32> vertexAttributeTexCoord(texCoordData.size(), IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadNever, IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadOften);
		auto vertexBuffer = renderer.CreateVertexBuffer();
		REQUIRE(vertexBuffer->BindVertexAttribute(vertexAttributePosition));
		REQUIRE(vertexAttributePosition.SetInitialData(positionVertexData));
		REQUIRE(vertexBuffer->BindVertexAttribute(vertexAttributeTexCoord));
		REQUIRE(vertexAttributeTexCoord.SetInitialData(texCoordData));
		REQUIRE(vertexBuffer->AllocateMemory());

		// Create the state-buffer layout from shader reflection.
		auto stateBufferLayout = renderer.CreateStateBufferLayout();
		REQUIRE(shaderProgram->LoadStateBufferLayoutFromShader(colorBufferId, stateBufferLayout.get()));

		// Create two state buffers so the active binding source is visible in the captured image.
		auto redStateBuffer = renderer.CreateStateBuffer();
		REQUIRE(redStateBuffer->BindBufferLayout(stateBufferLayout.get()));
		redStateBuffer->SetPageSettings(1);
		redStateBuffer->SetPerformanceHints(IStateBuffer::PerformanceHint::WriteOften | IStateBuffer::PerformanceHint::ReadNever, IStateBuffer::PerformanceHint::WriteNever | IStateBuffer::PerformanceHint::ReadOften);
		REQUIRE(redStateBuffer->AllocateMemory());
		redStateBuffer->SetStateValue(redStateBuffer->GetStateValueId("colorData.color"), V4Float32(1.0f, 0.0f, 0.0f, 1.0f));

		auto greenStateBuffer = renderer.CreateStateBuffer();
		REQUIRE(greenStateBuffer->BindBufferLayout(stateBufferLayout.get()));
		greenStateBuffer->SetPageSettings(1);
		greenStateBuffer->SetPerformanceHints(IStateBuffer::PerformanceHint::WriteOften | IStateBuffer::PerformanceHint::ReadNever, IStateBuffer::PerformanceHint::WriteNever | IStateBuffer::PerformanceHint::ReadOften);
		REQUIRE(greenStateBuffer->AllocateMemory());
		greenStateBuffer->SetStateValue(greenStateBuffer->GetStateValueId("colorData.color"), V4Float32(0.0f, 1.0f, 0.0f, 1.0f));

		// Create a renderable binding to the green state buffer.
		auto renderableNode = renderer.CreateRenderableNode();
		REQUIRE(renderableNode->BindVertexAttribute(vertexAttributePosition, positionAttributeId));
		REQUIRE(renderableNode->SetPrimitiveMode(IRenderableNode::PrimitiveMode::Triangles));
		renderableNode->BindStateBuffer(colorBufferId, greenStateBuffer.get());

		auto groupNode = renderer.CreateStateGroupNode();
		groupNode->AddChildNode(renderableNode.get());

		auto programNode = renderer.CreateProgramNode();
		REQUIRE(programNode->BindShaderProgram(shaderProgram.get()));
		programNode->AddChildNode(groupNode.get());

		// Create a default state binding to the red state buffer for fallback after the renderable binding is removed.
		auto defaultState = renderer.CreateDefaultState();
		defaultState->BindStateBuffer(colorBufferId, redStateBuffer.get());

		// Bind the render tree to the renderer.
		auto renderPassNode = renderer.CreateRenderPassNode();
		renderPassNode->BindFrameBuffer(frameBuffer.get());
		renderPassNode->SetAttachmentClearData(IFrameBuffer::AttachmentType::Color, 0, V4Float32(0.0f, 0.0f, 0.0f, 1.0f));
		renderPassNode->AddChildNode(programNode.get(), defaultState.get());
		renderer.SetRenderPasses(&renderPassNode, 1);

		// The renderable state-buffer binding should override the default-state binding.
		auto frameBufferCapture = renderer.CreateFrameBufferOutput();
		frameBufferCapture->SetDetachAfterCapture(true);
		frameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
		DrawOneFrame();
		session.AddTestImageResult("StateBufferRenderableBinding", "A fullscreen green quad using the renderable state-buffer binding.", std::move(frameBufferCapture), IImageDiff::Algorithm::AllOfThem, 0.95);

		// Unbinding the renderable state buffer should reveal the default-state binding.
		renderableNode->UnbindStateBuffer(colorBufferId);
		frameBufferCapture = renderer.CreateFrameBufferOutput();
		frameBufferCapture->SetDetachAfterCapture(true);
		frameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
		DrawOneFrame();
		session.AddTestImageResult("StateBufferDefaultBindingAfterUnbind", "A fullscreen red quad using the default-state buffer binding after unbinding the renderable state buffer.", std::move(frameBufferCapture), IImageDiff::Algorithm::AllOfThem, 0.95);
		renderer.RemoveAllRenderPasses();
	}
	renderer.WaitForDeferredDeletionComplete();

	// Test resource-array unbinding by falling back to a default-state resource array.
	if (!session.Device().IsFeatureSupported(IGraphicsDevice::Feature::ResourceArrays))
	{
		session.AddTestSkipped("ResourceArrayUnbindFallback", "This part of the test was skipped, as the current renderer doesn't support resource arrays on this device.");
		return true;
	}
	{
		// Create the framebuffer used by this binding case.
		auto frameBuffer = renderer.CreateFrameBuffer();
		frameBuffer->DefineViewportRegion(V2UInt32(0, 0), session.TestWindowSize());
		REQUIRE(uiThread.InvokeSync([&] { return frameBuffer->BindWindow(testWindowInfo, IFrameBuffer::WindowDepthStencilMode::None); }));

		// Create and compile the shader program, then retrieve the resource-array ID it exposes.
		auto shaderProgram = renderer.CreateShaderProgram();
		REQUIRE(shaderProgram->LoadShaderStage(IShaderProgram::ShaderStage::Vertex, ShaderSourceInfoHLSL(PositionOnlyVertexShader)));
		REQUIRE(shaderProgram->LoadShaderStage(IShaderProgram::ShaderStage::Fragment, ShaderSourceInfoHLSL(ResourceArrayFragmentShader)));
		REQUIRE(shaderProgram->CompileProgram());
		auto positionAttributeId = shaderProgram->GetVertexAttributeId("position");
		auto colorArrayId = shaderProgram->GetResourceArrayId("colorArray");

		// Create the vertex buffer used to draw the fullscreen quad.
		VertexAttribute<V4Float32> vertexAttributePosition(positionVertexData.size(), IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadNever, IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadOften);
		VertexAttribute<V2Float32> vertexAttributeTexCoord(texCoordData.size(), IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadNever, IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadOften);
		auto vertexBuffer = renderer.CreateVertexBuffer();
		REQUIRE(vertexBuffer->BindVertexAttribute(vertexAttributePosition));
		REQUIRE(vertexAttributePosition.SetInitialData(positionVertexData));
		REQUIRE(vertexBuffer->BindVertexAttribute(vertexAttributeTexCoord));
		REQUIRE(vertexAttributeTexCoord.SetInitialData(texCoordData));
		REQUIRE(vertexBuffer->AllocateMemory());

		// Create two resource arrays so the active binding source is visible in the captured image.
		V4Float32 redColor(1.0f, 0.0f, 0.0f, 1.0f);
		auto redDataArray = renderer.CreateDataArray();
		redDataArray->SetUsageFlags(IDataArray::UsageFlags::ShaderInput);
		redDataArray->SetBufferLayout(sizeof(V4Float32), 1);
		REQUIRE(redDataArray->SetInitialData(&redColor, sizeof(redColor)));
		REQUIRE(redDataArray->AllocateMemory());

		V4Float32 greenColor(0.0f, 1.0f, 0.0f, 1.0f);
		auto greenDataArray = renderer.CreateDataArray();
		greenDataArray->SetUsageFlags(IDataArray::UsageFlags::ShaderInput);
		greenDataArray->SetBufferLayout(sizeof(V4Float32), 1);
		REQUIRE(greenDataArray->SetInitialData(&greenColor, sizeof(greenColor)));
		REQUIRE(greenDataArray->AllocateMemory());

		// Create a renderable binding to the green resource array.
		auto renderableNode = renderer.CreateRenderableNode();
		REQUIRE(renderableNode->BindVertexAttribute(vertexAttributePosition, positionAttributeId));
		REQUIRE(renderableNode->SetPrimitiveMode(IRenderableNode::PrimitiveMode::Triangles));
		renderableNode->BindResourceArray(colorArrayId, greenDataArray.get());

		auto groupNode = renderer.CreateStateGroupNode();
		groupNode->AddChildNode(renderableNode.get());

		auto programNode = renderer.CreateProgramNode();
		REQUIRE(programNode->BindShaderProgram(shaderProgram.get()));
		programNode->AddChildNode(groupNode.get());

		// Create a default state binding to the red resource array for fallback after the renderable binding is removed.
		auto defaultState = renderer.CreateDefaultState();
		defaultState->BindResourceArray(colorArrayId, redDataArray.get());

		// Bind the render tree to the renderer.
		auto renderPassNode = renderer.CreateRenderPassNode();
		renderPassNode->BindFrameBuffer(frameBuffer.get());
		renderPassNode->SetAttachmentClearData(IFrameBuffer::AttachmentType::Color, 0, V4Float32(0.0f, 0.0f, 0.0f, 1.0f));
		renderPassNode->AddChildNode(programNode.get(), defaultState.get());
		renderer.SetRenderPasses(&renderPassNode, 1);

		// The renderable resource-array binding should override the default-state binding.
		auto frameBufferCapture = renderer.CreateFrameBufferOutput();
		frameBufferCapture->SetDetachAfterCapture(true);
		frameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
		DrawOneFrame();
		session.AddTestImageResult("ResourceArrayRenderableBinding", "A fullscreen green quad using the renderable resource-array binding.", std::move(frameBufferCapture), IImageDiff::Algorithm::AllOfThem, 0.95);

		// Unbinding the renderable resource array should reveal the default-state binding.
		renderableNode->UnbindResourceArray(colorArrayId);
		frameBufferCapture = renderer.CreateFrameBufferOutput();
		frameBufferCapture->SetDetachAfterCapture(true);
		frameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
		DrawOneFrame();
		session.AddTestImageResult("ResourceArrayDefaultBindingAfterUnbind", "A fullscreen red quad using the default-state resource-array binding after unbinding the renderable resource array.", std::move(frameBufferCapture), IImageDiff::Algorithm::AllOfThem, 0.95);
		renderer.RemoveAllRenderPasses();
	}
	renderer.WaitForDeferredDeletionComplete();
	return true;
}

} // namespace cobalt::graphics::testing
