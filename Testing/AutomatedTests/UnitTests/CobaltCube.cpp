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
    float2 texCoord : texCoord;
    float3 normal : normal;
};

struct VSOutput {
    float4 position : SV_POSITION;
    float2 texCoord : texCoord;
    float3 normal : normal;
};

uniform row_major float4x4 viewProj;

VSOutput main(VSInput IN)
{
    VSOutput OUT;

    OUT.position = mul(viewProj, float4(IN.position, 1.0f));
    OUT.texCoord = IN.texCoord;
    OUT.normal = IN.normal;

    return OUT;
}
)";
const std::string FragmentShader = R"(
uniform Texture2D colorTexture;
uniform SamplerState colorTexture_CombinedSampler;

struct VSOutput {
    float4 position : SV_POSITION;
    float2 texCoord : texCoord;
    float3 normal : normal;
};

float4 main(VSOutput IN) : SV_TARGET0
{
	float4 color = colorTexture.Sample(colorTexture_CombinedSampler, IN.texCoord.xy);
    color.rgb *= abs(dot(IN.normal, normalize(float3(3.0f,-4.0f, 5.0f))));
    return color;
}
)";

DEFINE_UNIT_TEST_WITH_BASE("Scene/CobaltCube", UnitTestBase)
{
	// Retrieve our important objects
	auto& renderer = session.Renderer();
	auto& uiThread = session.UIThread();
	auto& testWindowInfo = *session.TestWindowPlatformInfo();

	// Define the framebuffer
	auto mainWindowFrameBuffer = renderer.CreateFrameBuffer();
	mainWindowFrameBuffer->DefineViewportRegion(V2UInt32(0, 0), session.TestWindowSize());
	REQUIRE(uiThread.InvokeSync([&] { return mainWindowFrameBuffer->BindWindow(testWindowInfo, IFrameBuffer::WindowDepthStencilMode::DepthUNorm24); }));

	// Create and compile our shader program
	auto shaderProgram = renderer.CreateShaderProgram();
	REQUIRE(shaderProgram->LoadShaderStage(IShaderProgram::ShaderStage::Vertex, ShaderSourceInfoHLSL(VertexShader)));
	REQUIRE(shaderProgram->LoadShaderStage(IShaderProgram::ShaderStage::Fragment, ShaderSourceInfoHLSL(FragmentShader)));
	REQUIRE(shaderProgram->CompileProgram());

	// Retrieve our shader attribute IDs
	auto positionAttributeId = shaderProgram->GetVertexAttributeId("position");
	auto texCoordAttributeId = shaderProgram->GetVertexAttributeId("texCoord");
	auto normalAttributeId = shaderProgram->GetVertexAttributeId("normal");

	// Generate data for the object we want to render
	std::vector<V3Float32> positionVertexData;
	std::vector<V3Float32> normalVertexData;
	std::vector<V2Float32> texCoordData;
	std::vector<V1UInt32> indexData;
	Geometry().CreatePrimitiveCubeAsTriangles(positionVertexData, normalVertexData, texCoordData, indexData);

	// Create our vertex buffer and populate it with data
	VertexAttribute<V3Float32> vertexAttributePosition(positionVertexData.size(), IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadNever, IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadOften);
	VertexAttribute<V3Float32> vertexAttributeNormal(normalVertexData.size(), IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadNever, IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadOften);
	VertexAttribute<V2Float32> vertexAttributeTexCoord(texCoordData.size(), IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadNever, IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadOften);
	auto vertexBuffer = renderer.CreateVertexBuffer();
	REQUIRE(vertexBuffer->BindVertexAttribute(vertexAttributePosition));
	REQUIRE(vertexAttributePosition.SetInitialData(positionVertexData));
	REQUIRE(vertexBuffer->BindVertexAttribute(vertexAttributeNormal));
	REQUIRE(vertexAttributeNormal.SetInitialData(normalVertexData));
	REQUIRE(vertexBuffer->BindVertexAttribute(vertexAttributeTexCoord));
	REQUIRE(vertexAttributeTexCoord.SetInitialData(texCoordData));
	REQUIRE(vertexBuffer->AllocateMemory());

	// Create our index buffer and populate it with data
	IndexAttribute<V1UInt32> indexAttribute(indexData.size(), IIndexAttribute::PerformanceHint::WriteNever | IIndexAttribute::PerformanceHint::ReadNever, IIndexAttribute::PerformanceHint::WriteNever | IIndexAttribute::PerformanceHint::ReadOften);
	auto indexBuffer = renderer.CreateIndexBuffer();
	REQUIRE(indexBuffer->BindIndexAttribute(indexAttribute));
	REQUIRE(indexAttribute.SetInitialData(indexData));
	REQUIRE(indexBuffer->AllocateMemory());

	// Create our texture buffer and load our image data into it
	auto image = Texture().LoadImageFromPngFile("CobaltLogoBlackWithWhiteBackground.png");
	REQUIRE(image != nullptr);
	auto texture = renderer.CreateTextureBuffer2D();
	texture->SetTextureDimensions(image->size);
	texture->SetTextureFormat(ITextureBuffer::ImageFormat::RGBA, ITextureBuffer::DataFormat::UNorm8);
	texture->SetUsageFlags(ITextureBuffer::UsageFlags::ShaderInput);
	texture->SetPerformanceHints(ITextureBuffer::PerformanceHint::WriteNever | ITextureBuffer::PerformanceHint::ReadNever, ITextureBuffer::PerformanceHint::WriteNever | ITextureBuffer::PerformanceHint::ReadOften);
	REQUIRE(texture->SetInitialData(image->mipmapTextureData[0].data(), image->mipmapTextureData[0].size()));
	REQUIRE(texture->AllocateMemory());

	// Create a texture sampler to read the texture data from the shader
	auto sampler = renderer.CreateTextureSampler2D();
	sampler->SetTextureFilterMode(ITextureSampler::FilterMode::Nearest, ITextureSampler::FilterMode::Nearest);
	sampler->SetTextureWrapMode(ITextureSampler::WrapMode::ClampToEdge, ITextureSampler::WrapMode::ClampToEdge);

	// Create our renderable node
	auto renderableNode = renderer.CreateRenderableNode();
	REQUIRE(renderableNode->BindVertexAttribute(vertexAttributePosition, positionAttributeId));
	REQUIRE(renderableNode->BindVertexAttribute(vertexAttributeNormal, normalAttributeId));
	REQUIRE(renderableNode->BindVertexAttribute(vertexAttributeTexCoord, texCoordAttributeId));
	REQUIRE(renderableNode->BindIndexAttribute(indexAttribute));
	REQUIRE(renderableNode->SetPrimitiveMode(IRenderableNode::PrimitiveMode::Triangles));

	// Bind our texture to the renderable
	renderableNode->BindTextureWithCombinedSampler(shaderProgram->GetTextureId("colorTexture"), texture.get(), sampler.get());

	// Create our state group node
	auto groupNode = renderer.CreateStateGroupNode();
	groupNode->AddChildNode(renderableNode.get());

	// Create our program node
	auto programNode = renderer.CreateProgramNode();
	REQUIRE(programNode->BindShaderProgram(shaderProgram.get()));
	programNode->AddChildNode(groupNode.get());

	// Create our render pass node
	auto renderPassNode = renderer.CreateRenderPassNode();
	renderPassNode->BindFrameBuffer(mainWindowFrameBuffer.get());
	renderPassNode->SetAttachmentClearData(IFrameBuffer::AttachmentType::Color, 0, V4Float32(0.0f, 0.0f, 0.0f, 1.0f));
	renderPassNode->SetAttachmentClearData(IFrameBuffer::AttachmentType::Depth, 0, V4Float32(1.0f, 0.0f, 0.0f, 0.0f));
	renderPassNode->AddChildNode(programNode.get());

	// Bind our render tree to the renderer
	renderer.SetRenderPasses(&renderPassNode, 1);

	// Capture an image of the scene
	auto viewProj = Transform().LookAtCenterPerspective(session.TestWindowSizeAsFloat(), V3Float32(1.5f, 2.0f, 2.5f));
	renderableNode->SetStateValue(shaderProgram->GetStateValueId("viewProj"), viewProj);
	auto frameBufferCapture = renderer.CreateFrameBufferOutput();
	frameBufferCapture->SetDetachAfterCapture(true);
	mainWindowFrameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
	DrawOneFrame();
	session.AddTestImageResult("CobaltCube1", "Cube with Cobalt logo on top", std::move(frameBufferCapture), IImageDiff::Algorithm::AllOfThem, 0.95);

	// Capture another image of the scene with the camera repositioned
	viewProj = Transform().LookAtCenterPerspective(session.TestWindowSizeAsFloat(), V3Float32(-3.0f, 0.0f, -2.5f));
	renderableNode->SetStateValue(shaderProgram->GetStateValueId("viewProj"), viewProj);
	frameBufferCapture = renderer.CreateFrameBufferOutput();
	frameBufferCapture->SetDetachAfterCapture(true);
	mainWindowFrameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
	DrawOneFrame();
	session.AddTestImageResult("CobaltCube2", "Cube with Cobalt logo on bottom", std::move(frameBufferCapture), IImageDiff::Algorithm::AllOfThem, 0.95);

	// Remove all our defined render passes
	renderer.RemoveAllRenderPasses();
	return true;
}

} // namespace cobalt::graphics::testing
