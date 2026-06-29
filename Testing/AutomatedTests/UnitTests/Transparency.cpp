// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "../GeometryHelper.h"
#include "../UnitTestBase.h"

namespace cobalt::graphics::testing {
using namespace cobalt::graphics;

// Define our shader programs
const std::string VertexShader = R"(
struct VSInput {
    float3 position : position;
    float3 normal : normal;
    float4 color : color;
};

struct VSOutput {
    float4 position : SV_POSITION;
    float3 normal : normal;
    float4 color : color;
};

uniform row_major float4x4 viewProj;

VSOutput main(VSInput IN)
{
    VSOutput OUT;

    OUT.position = mul(viewProj, float4(IN.position, 1.0f));
    OUT.position /= OUT.position.w;
    OUT.normal = IN.normal;
    OUT.color = IN.color;

    return OUT;
}
)";

const std::string OpaqueFragmentShader = R"(
struct VSOutput {
    float4 position : SV_POSITION;
    float3 normal : normal;
    float4 color : color;
};

float4 main(VSOutput IN) : SV_TARGET0
{
    float4 color = float4(IN.color.rgb, 1.0f);
    color.rgb *= abs(dot(IN.normal, normalize(float3(3.0f,-4.0f, 5.0f))));

    return color;
}
)";

const std::string TransparentFragmentShader = R"(
struct VSOutput {
    float4 position : SV_POSITION;
    float3 normal : normal;
    float4 color : color;
};

struct PSOutput {
    float4 colorAndMagnitude : SV_TARGET0;
    float blendFactor : SV_TARGET1;
};

uniform bool UsePerRenderTargetBlending;

// Take the depth and color/transparency data to calculate the color, weight and blend factor for composition
void SetBlendInfo(in float depth, in float4 colorAndOpacity, out float4 colorAndMagnitude, out float blendFactor)
{
    float opacity = colorAndOpacity.a;
    float3 color = colorAndOpacity.rgb;
    float weight = clamp(10 / (1e-5 + pow(depth / 10, 2) + pow(depth / 200, 6)), 1e-2, 3e3);

    if (UsePerRenderTargetBlending)
    {
        colorAndMagnitude = float4(color * opacity, opacity) * weight;
        blendFactor = opacity;
    }
    else
    {
        // ##TODO## Test
        colorAndMagnitude = float4(color * opacity * weight, opacity); // verify!!!
        blendFactor = opacity * weight;
    }
}

PSOutput main(VSOutput IN) : SV_TARGET0
{
    PSOutput OUT;

    float4 color = IN.color;
    color.rgb *= abs(dot(IN.normal, normalize(float3(3.0f,-4.0f, 5.0f))));

    SetBlendInfo(IN.position.z, color, OUT.colorAndMagnitude, OUT.blendFactor);

    return OUT;
}
)";

const std::string CompositingVertexShader = R"(
float4 main(float3 position : position) : SV_POSITION
{
    return float4(position, 1.0);
}
)";

const std::string CompositingFragmentShader = R"(
uniform Texture2D ColorTex0;
uniform SamplerState ColorTex0_CombinedSampler;
uniform Texture2D ColorTex1;
uniform SamplerState ColorTex1_CombinedSampler;
uniform Texture2D OpaqueColor;
uniform SamplerState OpaqueColor_CombinedSampler;
uniform bool UsePerRenderTargetBlending;

float4 main(float4 position : SV_POSITION) : SV_TARGET0
{
	float4 outColor;
	// Switch for per-render target blending support
	float transmittance;
	float4 backgroundColor;
	float3 averageColor;
	float width;
	float height;
	if (UsePerRenderTargetBlending)
	{
		ColorTex0.GetDimensions(width, height);
		float4 sumColor = ColorTex0.Sample(ColorTex0_CombinedSampler, position.xy / float2(width, height));
		ColorTex1.GetDimensions(width, height);
		transmittance = ColorTex1.Sample(ColorTex1_CombinedSampler, position.xy / float2(width, height)).r;
		OpaqueColor.GetDimensions(width, height);
		backgroundColor = OpaqueColor.Sample(OpaqueColor_CombinedSampler, position.xy / float2(width, height));
		averageColor = sumColor.rgb / max(sumColor.a, 0.00001);
	}
	else
	{
		ColorTex0.GetDimensions(width, height);
		float4 sumColor = ColorTex0.Sample(ColorTex0_CombinedSampler, position.xy / float2(width, height));
		transmittance = sumColor.a;
		ColorTex1.GetDimensions(width, height);
		sumColor.a = ColorTex1.Sample(ColorTex1_CombinedSampler, position.xy / float2(width, height)).r;
		OpaqueColor.GetDimensions(width, height);
		backgroundColor = OpaqueColor.Sample(OpaqueColor_CombinedSampler, position.xy / float2(width, height));
		averageColor = sumColor.rgb / max(sumColor.a, 0.00001);
	}

	outColor.rgb = (averageColor * (1 - transmittance)) + (backgroundColor.rgb * transmittance);
	outColor.a = 1.0;

	return outColor;
}

)";

DEFINE_UNIT_TEST_WITH_BASE("Scene/Transparency", UnitTestBase)
{
	// Retrieve our important objects
	auto& renderer = session.Renderer();
	auto& uiThread = session.UIThread();
	auto& testWindowInfo = *session.TestWindowPlatformInfo();

	bool separateBlendModePerTargetSupported = session.Device().IsFeatureSupported(IGraphicsDevice::Feature::SeparateBlendModePerTarget);

	// Define the textures
	auto opaqueColorTexture = renderer.CreateTextureBuffer2D();
	opaqueColorTexture->SetUsageFlags(ITextureBuffer::UsageFlags::FrameBufferOutput | ITextureBuffer::UsageFlags::ShaderInput);
	opaqueColorTexture->SetTextureFormat(ITextureBuffer2D::ImageFormat::RGBA, ITextureBuffer2D::DataFormat::UNorm8);
	opaqueColorTexture->SetTextureDimensions(V2UInt32(session.TestWindowSize()));
	REQUIRE(opaqueColorTexture->AllocateMemory());

	auto transparentColorTexture = renderer.CreateTextureBuffer2D();
	transparentColorTexture->SetUsageFlags(ITextureBuffer::UsageFlags::FrameBufferOutput | ITextureBuffer::UsageFlags::ShaderInput);
	transparentColorTexture->SetTextureFormat(ITextureBuffer2D::ImageFormat::RGBA, ITextureBuffer2D::DataFormat::Float32);
	transparentColorTexture->SetTextureDimensions(V2UInt32(session.TestWindowSize()));
	REQUIRE(transparentColorTexture->AllocateMemory());

	auto transparentColorFactorTexture = renderer.CreateTextureBuffer2D();
	transparentColorFactorTexture->SetUsageFlags(ITextureBuffer::UsageFlags::FrameBufferOutput | ITextureBuffer::UsageFlags::ShaderInput);
	transparentColorFactorTexture->SetTextureFormat(ITextureBuffer2D::ImageFormat::R, ITextureBuffer2D::DataFormat::Float32);
	transparentColorFactorTexture->SetTextureDimensions(V2UInt32(session.TestWindowSize()));
	REQUIRE(transparentColorFactorTexture->AllocateMemory());

	auto depthTexture = renderer.CreateTextureBuffer2D();
	depthTexture->SetUsageFlags(ITextureBuffer::UsageFlags::FrameBufferOutput);
	depthTexture->SetTextureFormat(ITextureBuffer2D::ImageFormat::Depth, ITextureBuffer2D::DataFormat::DepthUNorm24);
	depthTexture->SetTextureDimensions(V2UInt32(session.TestWindowSize()));
	REQUIRE(depthTexture->AllocateMemory());

	// Define the framebuffers
	auto mainWindowFrameBuffer = renderer.CreateFrameBuffer();
	mainWindowFrameBuffer->DefineViewportRegion(V2UInt32(0, 0), session.TestWindowSize());
	REQUIRE(uiThread.InvokeSync([&] { return mainWindowFrameBuffer->BindWindow(testWindowInfo, IFrameBuffer::WindowDepthStencilMode::None); }));

	auto opaqueFrameBuffer = renderer.CreateFrameBuffer();
	opaqueFrameBuffer->DefineViewportRegion(V2UInt32(0, 0), session.TestWindowSize());
	REQUIRE(opaqueFrameBuffer->BindTexture(opaqueColorTexture.get(), IFrameBuffer::AttachmentType::Color));
	REQUIRE(opaqueFrameBuffer->BindTexture(depthTexture.get(), IFrameBuffer::AttachmentType::Depth));

	auto transparentFrameBuffer = renderer.CreateFrameBuffer();
	transparentFrameBuffer->DefineViewportRegion(V2UInt32(0, 0), session.TestWindowSize());
	REQUIRE(transparentFrameBuffer->BindTexture(transparentColorTexture.get(), IFrameBuffer::AttachmentType::Color, 0));
	REQUIRE(transparentFrameBuffer->BindTexture(transparentColorFactorTexture.get(), IFrameBuffer::AttachmentType::Color, 1));
	REQUIRE(transparentFrameBuffer->BindTexture(depthTexture.get(), IFrameBuffer::AttachmentType::Depth));

	// Create and compile our shader programs
	auto opaqueShaderProgram = renderer.CreateShaderProgram();
	REQUIRE(opaqueShaderProgram->LoadShaderStage(IShaderProgram::ShaderStage::Vertex, ShaderSourceInfoHLSL(VertexShader)));
	REQUIRE(opaqueShaderProgram->LoadShaderStage(IShaderProgram::ShaderStage::Fragment, ShaderSourceInfoHLSL(OpaqueFragmentShader)));
	REQUIRE(opaqueShaderProgram->CompileProgram());

	auto transparentShaderProgram = renderer.CreateShaderProgram();
	REQUIRE(transparentShaderProgram->LoadShaderStage(IShaderProgram::ShaderStage::Vertex, ShaderSourceInfoHLSL(VertexShader)));
	REQUIRE(transparentShaderProgram->LoadShaderStage(IShaderProgram::ShaderStage::Fragment, ShaderSourceInfoHLSL(TransparentFragmentShader)));
	REQUIRE(transparentShaderProgram->CompileProgram());

	auto compositingShaderProgram = renderer.CreateShaderProgram();
	REQUIRE(compositingShaderProgram->LoadShaderStage(IShaderProgram::ShaderStage::Vertex, ShaderSourceInfoHLSL(CompositingVertexShader)));
	REQUIRE(compositingShaderProgram->LoadShaderStage(IShaderProgram::ShaderStage::Fragment, ShaderSourceInfoHLSL(CompositingFragmentShader)));
	REQUIRE(compositingShaderProgram->CompileProgram());

	// Create our state group nodes
	auto opaqueGroupNode = renderer.CreateStateGroupNode();

	auto transparentGroupNode = renderer.CreateStateGroupNode();
	transparentGroupNode->SetBlendEnabled(true);
	if (separateBlendModePerTargetSupported)
	{
		transparentGroupNode->SetBlendMode(IFrameBuffer::AttachmentType::Color, 0, IStateGroupNode::BlendOperation::Add, IStateGroupNode::BlendFactor::One, IStateGroupNode::BlendFactor::One, IStateGroupNode::BlendOperation::Add, IStateGroupNode::BlendFactor::One, IStateGroupNode::BlendFactor::One);
		transparentGroupNode->SetBlendMode(IFrameBuffer::AttachmentType::Color, 1, IStateGroupNode::BlendOperation::Add, IStateGroupNode::BlendFactor::Zero, IStateGroupNode::BlendFactor::OneMinusSourceColor, IStateGroupNode::BlendOperation::Add, IStateGroupNode::BlendFactor::Zero, IStateGroupNode::BlendFactor::OneMinusSourceAlpha);
	}
	else
	{
		transparentGroupNode->SetBlendMode(IStateGroupNode::BlendOperation::Add, IStateGroupNode::BlendFactor::One, IStateGroupNode::BlendFactor::One, IStateGroupNode::BlendOperation::Add, IStateGroupNode::BlendFactor::Zero, IStateGroupNode::BlendFactor::OneMinusSourceAlpha);
	}
	transparentGroupNode->SetDepthWriteEnabled(false);
	transparentGroupNode->SetDepthTestEnabled(true);
	transparentGroupNode->SetStateValue(transparentShaderProgram->GetStateValueId("UsePerRenderTargetBlending"), separateBlendModePerTargetSupported);

	auto sampler = renderer.CreateTextureSampler2D();
	sampler->SetTextureFilterMode(ITextureSampler::FilterMode::Nearest, ITextureSampler::FilterMode::Nearest);
	sampler->SetTextureWrapMode(ITextureSampler::WrapMode::ClampToEdge, ITextureSampler::WrapMode::ClampToEdge);

	auto compositingGroupNode = renderer.CreateStateGroupNode();
	compositingGroupNode->SetStateValue(compositingShaderProgram->GetStateValueId("UsePerRenderTargetBlending"), separateBlendModePerTargetSupported);
	compositingGroupNode->BindTextureWithCombinedSampler(compositingShaderProgram->GetTextureId("ColorTex0"), transparentColorTexture.get(), sampler.get());
	compositingGroupNode->BindTextureWithCombinedSampler(compositingShaderProgram->GetTextureId("ColorTex1"), transparentColorFactorTexture.get(), sampler.get());
	compositingGroupNode->BindTextureWithCombinedSampler(compositingShaderProgram->GetTextureId("OpaqueColor"), opaqueColorTexture.get(), sampler.get());

	// Create our program nodes
	auto opaqueProgramNode = renderer.CreateProgramNode();
	REQUIRE(opaqueProgramNode->BindShaderProgram(opaqueShaderProgram.get()));
	opaqueProgramNode->AddChildNode(opaqueGroupNode.get());

	auto transparentProgramNode = renderer.CreateProgramNode();
	REQUIRE(transparentProgramNode->BindShaderProgram(transparentShaderProgram.get()));
	transparentProgramNode->AddChildNode(transparentGroupNode.get());

	auto compositingProgramNode = renderer.CreateProgramNode();
	REQUIRE(compositingProgramNode->BindShaderProgram(compositingShaderProgram.get()));
	compositingProgramNode->AddChildNode(compositingGroupNode.get());

	// Create our render pass nodes
	auto opaqueRenderPassNode = renderer.CreateRenderPassNode();
	opaqueRenderPassNode->BindFrameBuffer(opaqueFrameBuffer.get());
	opaqueRenderPassNode->SetAttachmentClearData(IFrameBuffer::AttachmentType::Color, 0, V4Float32(0.0f, 0.0f, 0.0f, 1.0f));
	opaqueRenderPassNode->SetAttachmentClearData(IFrameBuffer::AttachmentType::Depth, 0, V4Float32(1.0f, 1.0f, 1.0f, 1.0f));
	opaqueRenderPassNode->AddChildNode(opaqueProgramNode.get());

	auto transparentRenderPassNode = renderer.CreateRenderPassNode();
	transparentRenderPassNode->BindFrameBuffer(transparentFrameBuffer.get());
	transparentRenderPassNode->SetAttachmentClearData(IFrameBuffer::AttachmentType::Color, 0, V4Float32(0.0f, 0.0f, 0.0f, 0.0f));
	transparentRenderPassNode->SetAttachmentClearData(IFrameBuffer::AttachmentType::Color, 1, V4Float32(1.0f, 1.0f, 1.0f, 1.0f));
	transparentRenderPassNode->AddChildNode(transparentProgramNode.get());

	auto compositingRenderPassNode = renderer.CreateRenderPassNode();
	compositingRenderPassNode->BindFrameBuffer(mainWindowFrameBuffer.get());
	compositingRenderPassNode->SetAttachmentClearData(IFrameBuffer::AttachmentType::Color, 0, V4Float32(0.0f, 0.0f, 0.0f, 1.0f));
	compositingRenderPassNode->AddChildNode(compositingProgramNode.get());

	// Bind our render tree to the renderer
	IRenderPassNode* passes[3] = {opaqueRenderPassNode.get(), transparentRenderPassNode.get(), compositingRenderPassNode.get()};
	renderer.SetRenderPasses(&passes[0], 3);

	// Create our vertex buffer and populate it with data
	std::vector<V3Float32> spherePositionVertexData;
	std::vector<V3Float32> sphereNormalVertexData;
	std::vector<V2Float32> sphereTextureVertexData;
	std::vector<V4Float32> sphereColorVertexData;
	std::vector<V1UInt32> sphereIndexData;
	Geometry().CreatePrimitiveSphereAsTriangles(64, 64, spherePositionVertexData, sphereNormalVertexData, sphereTextureVertexData, sphereIndexData);

	sphereColorVertexData.resize(spherePositionVertexData.size());
	for (size_t i = 0; i < spherePositionVertexData.size(); ++i)
	{
		sphereColorVertexData[i] = V4Float32(0.25f, 1.0f, 1.0f, (spherePositionVertexData[i].Z() + 1.0f) / 2.0f);
	}

	std::vector<V3Float32> cubePositionVertexData;
	std::vector<V3Float32> cubeNormalVertexData;
	std::vector<V2Float32> cubeTextureVertexData;
	std::vector<V4Float32> cubeColorVertexData;
	std::vector<V1UInt32> cubeIndexData;
	Geometry().CreatePrimitiveCubeAsTriangles(cubePositionVertexData, cubeNormalVertexData, cubeTextureVertexData, cubeIndexData);

	cubeColorVertexData.resize(cubePositionVertexData.size());
	for (size_t i = 0; i < cubePositionVertexData.size(); ++i)
	{
		cubeColorVertexData[i] = V4Float32(1.0f, 1.0f, 0.5f, 0.5f);
	}

	std::vector<V3Float32> screenQuadPositions({{-1, -1, 0}, {-1, 1, 0}, {1, 1, 0}, {-1, -1, 0}, {1, 1, 0}, {1, -1, 0}});

	VertexAttribute<V3Float32> sphereVertexAttributePosition(spherePositionVertexData.size(), IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadNever, IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadOften);
	VertexAttribute<V3Float32> sphereVertexAttributeNormal(sphereNormalVertexData.size(), IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadNever, IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadOften);
	VertexAttribute<V4Float32> sphereVertexAttributeColor(sphereColorVertexData.size(), IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadNever, IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadOften);

	VertexAttribute<V3Float32> cubeVertexAttributePosition(cubePositionVertexData.size(), IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadNever, IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadOften);
	VertexAttribute<V3Float32> cubeVertexAttributeNormal(cubeNormalVertexData.size(), IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadNever, IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadOften);
	VertexAttribute<V4Float32> cubeVertexAttributeColor(cubeColorVertexData.size(), IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadNever, IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadOften);

	VertexAttribute<V3Float32> screenQuadPositionsAttribute(screenQuadPositions.size(), IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadNever, IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadOften);

	auto vertexBuffer = renderer.CreateVertexBuffer();
	REQUIRE(vertexBuffer->BindVertexAttribute(sphereVertexAttributePosition));
	REQUIRE(sphereVertexAttributePosition.SetInitialData(spherePositionVertexData));
	REQUIRE(vertexBuffer->BindVertexAttribute(sphereVertexAttributeNormal));
	REQUIRE(sphereVertexAttributeNormal.SetInitialData(sphereNormalVertexData));
	REQUIRE(vertexBuffer->BindVertexAttribute(sphereVertexAttributeColor));
	REQUIRE(sphereVertexAttributeColor.SetInitialData(sphereColorVertexData));
	REQUIRE(vertexBuffer->BindVertexAttribute(cubeVertexAttributePosition));
	REQUIRE(cubeVertexAttributePosition.SetInitialData(cubePositionVertexData));
	REQUIRE(vertexBuffer->BindVertexAttribute(cubeVertexAttributeNormal));
	REQUIRE(cubeVertexAttributeNormal.SetInitialData(cubeNormalVertexData));
	REQUIRE(vertexBuffer->BindVertexAttribute(cubeVertexAttributeColor));
	REQUIRE(cubeVertexAttributeColor.SetInitialData(cubeColorVertexData));
	REQUIRE(vertexBuffer->BindVertexAttribute(screenQuadPositionsAttribute));
	REQUIRE(screenQuadPositionsAttribute.SetInitialData(screenQuadPositions));
	REQUIRE(vertexBuffer->AllocateMemory());

	// Create our index buffer and populate it with data
	IndexAttribute<V1UInt32> sphereIndexAttribute(sphereIndexData.size(), IIndexAttribute::PerformanceHint::WriteNever | IIndexAttribute::PerformanceHint::ReadNever, IIndexAttribute::PerformanceHint::WriteNever | IIndexAttribute::PerformanceHint::ReadOften);
	auto sphereIndexBuffer = renderer.CreateIndexBuffer();
	REQUIRE(sphereIndexBuffer->BindIndexAttribute(sphereIndexAttribute));
	REQUIRE(sphereIndexAttribute.SetInitialData(sphereIndexData));
	REQUIRE(sphereIndexBuffer->AllocateMemory());

	IndexAttribute<V1UInt32> cubeIndexAttribute(cubeIndexData.size(), IIndexAttribute::PerformanceHint::WriteNever | IIndexAttribute::PerformanceHint::ReadNever, IIndexAttribute::PerformanceHint::WriteNever | IIndexAttribute::PerformanceHint::ReadOften);
	auto cubeIndexBuffer = renderer.CreateIndexBuffer();
	REQUIRE(cubeIndexBuffer->BindIndexAttribute(cubeIndexAttribute));
	REQUIRE(cubeIndexAttribute.SetInitialData(cubeIndexData));
	REQUIRE(cubeIndexBuffer->AllocateMemory());

	// Create our renderable nodes
	auto screenQuadRenderableNode = renderer.CreateRenderableNode();
	REQUIRE(screenQuadRenderableNode->BindVertexAttribute(screenQuadPositionsAttribute, opaqueShaderProgram->GetVertexAttributeId("position")));
	REQUIRE(screenQuadRenderableNode->SetPrimitiveMode(IRenderableNode::PrimitiveMode::Triangles));
	compositingGroupNode->AddChildNode(screenQuadRenderableNode.get());

	auto sphereRenderableNode = renderer.CreateRenderableNode();
	REQUIRE(sphereRenderableNode->BindVertexAttribute(sphereVertexAttributePosition, opaqueShaderProgram->GetVertexAttributeId("position")));
	REQUIRE(sphereRenderableNode->BindVertexAttribute(sphereVertexAttributeNormal, opaqueShaderProgram->GetVertexAttributeId("normal")));
	REQUIRE(sphereRenderableNode->BindVertexAttribute(sphereVertexAttributeColor, opaqueShaderProgram->GetVertexAttributeId("color")));
	REQUIRE(sphereRenderableNode->BindIndexAttribute(sphereIndexAttribute));
	REQUIRE(sphereRenderableNode->SetPrimitiveMode(IRenderableNode::PrimitiveMode::Triangles));

	auto cubeRenderableNode = renderer.CreateRenderableNode();
	REQUIRE(cubeRenderableNode->BindVertexAttribute(cubeVertexAttributePosition, opaqueShaderProgram->GetVertexAttributeId("position")));
	REQUIRE(cubeRenderableNode->BindVertexAttribute(cubeVertexAttributeNormal, opaqueShaderProgram->GetVertexAttributeId("normal")));
	REQUIRE(cubeRenderableNode->BindVertexAttribute(cubeVertexAttributeColor, opaqueShaderProgram->GetVertexAttributeId("color")));
	REQUIRE(cubeRenderableNode->BindIndexAttribute(cubeIndexAttribute));
	REQUIRE(cubeRenderableNode->SetPrimitiveMode(IRenderableNode::PrimitiveMode::Triangles));

	{
		opaqueGroupNode->AddChildNode(sphereRenderableNode.get());
		transparentGroupNode->AddChildNode(cubeRenderableNode.get());

		// Attach a framebuffer output capture target for our screenshot
		auto frameBufferCapture = renderer.CreateFrameBufferOutput();
		frameBufferCapture->SetDetachAfterCapture(true);
		mainWindowFrameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
		auto viewProj = Transform().LookAtCenterPerspective(session.TestWindowSizeAsFloat(), V3Float32(2.0f, 4.0f, 3.0f), 50);
		opaqueGroupNode->SetStateValue(opaqueShaderProgram->GetStateValueId("viewProj"), viewProj);
		transparentGroupNode->SetStateValue(transparentShaderProgram->GetStateValueId("viewProj"), viewProj);
		DrawOneFrame();
		session.AddTestImageResult("Transparency1", "Opaque sphere inside transparent cube", std::move(frameBufferCapture), IImageDiff::Algorithm::AllOfThem, 0.95);
	}

	{
		opaqueGroupNode->RemoveChildNode(sphereRenderableNode.get());
		transparentGroupNode->AddChildNode(sphereRenderableNode.get());

		// Attach a framebuffer output capture target for our screenshot
		auto frameBufferCapture = renderer.CreateFrameBufferOutput();
		frameBufferCapture->SetDetachAfterCapture(true);
		mainWindowFrameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
		auto viewProj = Transform().LookAtCenterPerspective(session.TestWindowSizeAsFloat(), V3Float32(2.0f, -4.0f, 3.0f), 50);
		opaqueGroupNode->SetStateValue(opaqueShaderProgram->GetStateValueId("viewProj"), viewProj);
		transparentGroupNode->SetStateValue(transparentShaderProgram->GetStateValueId("viewProj"), viewProj);
		DrawOneFrame();
		session.AddTestImageResult("Transparency2", "Transparent sphere inside transparent cube", std::move(frameBufferCapture), IImageDiff::Algorithm::AllOfThem, 0.95);
	}

	{
		// Attach a framebuffer output capture target for our screenshot
		auto frameBufferCapture = renderer.CreateFrameBufferOutput();
		frameBufferCapture->SetDetachAfterCapture(true);
		mainWindowFrameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
		auto viewProj = Transform().LookAtCenterPerspective(session.TestWindowSizeAsFloat(), V3Float32(2.0f, 0.0f, 0.0f), 70);
		opaqueGroupNode->SetStateValue(opaqueShaderProgram->GetStateValueId("viewProj"), viewProj);
		transparentGroupNode->SetStateValue(transparentShaderProgram->GetStateValueId("viewProj"), viewProj);
		DrawOneFrame();
		session.AddTestImageResult("Transparency3", "Sphere front on with opacity gradient", std::move(frameBufferCapture), IImageDiff::Algorithm::AllOfThem, 0.95);
	}

	{
		transparentGroupNode->RemoveChildNode(cubeRenderableNode.get());
		opaqueGroupNode->AddChildNode(cubeRenderableNode.get());

		// Attach a framebuffer output capture target for our screenshot
		auto frameBufferCapture = renderer.CreateFrameBufferOutput();
		frameBufferCapture->SetDetachAfterCapture(true);
		mainWindowFrameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
		auto viewProj = Transform().LookAtCenterPerspective(session.TestWindowSizeAsFloat(), V3Float32(2.0f, 4.0f, 3.0f), 50);
		opaqueGroupNode->SetStateValue(opaqueShaderProgram->GetStateValueId("viewProj"), viewProj);
		transparentGroupNode->SetStateValue(transparentShaderProgram->GetStateValueId("viewProj"), viewProj);
		DrawOneFrame();
		session.AddTestImageResult("Transparency4", "Opaque cube occluding transparent sphere", std::move(frameBufferCapture), IImageDiff::Algorithm::AllOfThem, 0.95);
	}

	// Remove all our defined render passes
	renderer.RemoveAllRenderPasses();
	return true;
}

} // namespace cobalt::graphics::testing
