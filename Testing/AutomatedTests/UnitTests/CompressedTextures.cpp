// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "../GeometryHelper.h"
#include "../UnitTestBase.h"
#include <array>
#include <memory>
#include <string>
#include <vector>

namespace cobalt::graphics::testing {
using namespace cobalt::graphics;

// Define our shader programs
const std::string CompressedTextureVertexShader = R"(
struct VSInput {
    float3 position : position;
    float2 texCoord : texCoord;
};

struct VSOutput {
    float4 position : SV_POSITION;
    float2 texCoord : texCoord;
};

VSOutput main(VSInput IN)
{
    VSOutput OUT;

    OUT.position = float4(IN.position, 1.0f);
    OUT.texCoord = IN.texCoord;

    return OUT;
}
)";
const std::string CompressedTextureFragmentShader = R"(
uniform Texture2D colorTexture;
uniform SamplerState colorTexture_CombinedSampler;

struct VSOutput {
    float4 position : SV_POSITION;
    float2 texCoord : texCoord;
};

float4 main(VSOutput IN) : SV_TARGET0
{
    return colorTexture.Sample(colorTexture_CombinedSampler, IN.texCoord.xy);
}
)";

namespace {
enum class CompressedTextureFileType
{
	Dds,
	Ktx
};

struct CompressedTextureCase
{
	const char* formatName;
	ITextureBuffer::ImageFormat imageFormat;
	ITextureBuffer::DataFormat dataFormat;
	CompressedTextureFileType fileType;
};
} // namespace

DEFINE_UNIT_TEST_WITH_BASE("Resources/Images/CompressedTextures", UnitTestBase)
{
	static constexpr uint32_t AssumedTextureSize = 512;
	static constexpr std::array<CompressedTextureCase, 9> TestCases{{
	  {"DXT1", ITextureBuffer::ImageFormat::RGBA, ITextureBuffer::DataFormat::DXT1, CompressedTextureFileType::Dds},
	  {"DXT3", ITextureBuffer::ImageFormat::RGBA, ITextureBuffer::DataFormat::DXT3, CompressedTextureFileType::Dds},
	  {"DXT5", ITextureBuffer::ImageFormat::RGBA, ITextureBuffer::DataFormat::DXT5, CompressedTextureFileType::Dds},
	  {"BPTC", ITextureBuffer::ImageFormat::RGBA, ITextureBuffer::DataFormat::BPTC, CompressedTextureFileType::Dds},
	  {"ETC2", ITextureBuffer::ImageFormat::RGBA, ITextureBuffer::DataFormat::ETC2, CompressedTextureFileType::Ktx},
	  {"ASTC4x4", ITextureBuffer::ImageFormat::RGBA, ITextureBuffer::DataFormat::ASTC4x4, CompressedTextureFileType::Ktx},
	  {"ASTC5x5", ITextureBuffer::ImageFormat::RGBA, ITextureBuffer::DataFormat::ASTC5x5, CompressedTextureFileType::Ktx},
	  {"ASTC6x6", ITextureBuffer::ImageFormat::RGBA, ITextureBuffer::DataFormat::ASTC6x6, CompressedTextureFileType::Ktx},
	  {"ASTC8x8", ITextureBuffer::ImageFormat::RGBA, ITextureBuffer::DataFormat::ASTC8x8, CompressedTextureFileType::Ktx},
	}};

	// Retrieve our important objects
	auto& renderer = session.Renderer();
	auto& uiThread = session.UIThread();
	auto& testWindowInfo = *session.TestWindowPlatformInfo();

	{
		// Define the framebuffer
		auto mainWindowFrameBuffer = renderer.CreateFrameBuffer();
		mainWindowFrameBuffer->DefineViewportRegion(V2UInt32(0, 0), session.TestWindowSize());
		REQUIRE(uiThread.InvokeSync([&] { return mainWindowFrameBuffer->BindWindow(testWindowInfo, IFrameBuffer::WindowDepthStencilMode::DepthUNorm24); }));

		// Create and compile our shader program
		auto shaderProgram = renderer.CreateShaderProgram();
		REQUIRE(shaderProgram->LoadShaderStage(IShaderProgram::ShaderStage::Vertex, ShaderSourceInfoHLSL(CompressedTextureVertexShader)));
		REQUIRE(shaderProgram->LoadShaderStage(IShaderProgram::ShaderStage::Fragment, ShaderSourceInfoHLSL(CompressedTextureFragmentShader)));
		REQUIRE(shaderProgram->CompileProgram());

		// Retrieve our shader attribute IDs
		auto positionAttributeId = shaderProgram->GetVertexAttributeId("position");
		auto texCoordAttributeId = shaderProgram->GetVertexAttributeId("texCoord");

		// Generate data for the object we want to render
		std::vector<V3Float32> positionVertexData;
		std::vector<V3Float32> normalVertexData;
		std::vector<V2Float32> texCoordData;
		std::vector<V1UInt32> indexData;
		Geometry().CreatePrimitiveSquareAsTriangles(10, positionVertexData, normalVertexData, texCoordData, indexData);

		// Create our vertex buffer and populate it with data
		VertexAttribute<V3Float32> vertexAttributePosition(positionVertexData.size(), IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadNever, IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadOften);
		VertexAttribute<V2Float32> vertexAttributeTexCoord(texCoordData.size(), IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadNever, IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadOften);
		auto vertexBuffer = renderer.CreateVertexBuffer();
		REQUIRE(vertexBuffer->BindVertexAttribute(vertexAttributePosition));
		REQUIRE(vertexAttributePosition.SetInitialData(positionVertexData));
		REQUIRE(vertexBuffer->BindVertexAttribute(vertexAttributeTexCoord));
		REQUIRE(vertexAttributeTexCoord.SetInitialData(texCoordData));
		REQUIRE(vertexBuffer->AllocateMemory());

		// Create our index buffer and populate it with data
		IndexAttribute<V1UInt32> indexAttribute(indexData.size(), IIndexAttribute::PerformanceHint::WriteNever | IIndexAttribute::PerformanceHint::ReadNever, IIndexAttribute::PerformanceHint::WriteNever | IIndexAttribute::PerformanceHint::ReadOften);
		auto indexBuffer = renderer.CreateIndexBuffer();
		REQUIRE(indexBuffer->BindIndexAttribute(indexAttribute));
		REQUIRE(indexAttribute.SetInitialData(indexData));
		REQUIRE(indexBuffer->AllocateMemory());

		// Create a texture sampler to read the texture data from the shader
		auto sampler = renderer.CreateTextureSampler2D();
		sampler->SetTextureFilterMode(ITextureSampler::FilterMode::Nearest, ITextureSampler::FilterMode::Nearest);
		sampler->SetTextureWrapMode(ITextureSampler::WrapMode::ClampToEdge, ITextureSampler::WrapMode::ClampToEdge);

		// Create our renderable node
		auto renderableNode = renderer.CreateRenderableNode();
		REQUIRE(renderableNode->BindVertexAttribute(vertexAttributePosition, positionAttributeId));
		REQUIRE(renderableNode->BindVertexAttribute(vertexAttributeTexCoord, texCoordAttributeId));
		REQUIRE(renderableNode->BindIndexAttribute(indexAttribute));
		REQUIRE(renderableNode->SetPrimitiveMode(IRenderableNode::PrimitiveMode::Triangles));

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

		auto colorTextureId = shaderProgram->GetTextureId("colorTexture");
		for (const auto& testCase : TestCases)
		{
			std::string testName = std::string("CompressedTexture") + testCase.formatName;
			if (!session.Device().IsTextureFormatSupported(testCase.imageFormat, testCase.dataFormat))
			{
				session.AddTestSkipped(testName, "This test was skipped, as the current graphics device does not support this compressed texture format.");
				continue;
			}

			std::unique_ptr<TextureHelper::CompressedTextureInfo> compressedImage;
			switch (testCase.fileType)
			{
			case CompressedTextureFileType::Dds:
				compressedImage = Texture().LoadImageFromDdsFile(testName + ".dds");
				break;
			case CompressedTextureFileType::Ktx:
				compressedImage = Texture().LoadImageFromKtxFile(testName + ".ktx");
				break;
			}
			REQUIRE(compressedImage != nullptr);
			REQUIRE(compressedImage->size == V2UInt32(AssumedTextureSize, AssumedTextureSize));
			REQUIRE(compressedImage->dataFormat == testCase.dataFormat);
			REQUIRE(static_cast<size_t>(compressedImage->mipmapLevelCount) == compressedImage->mipmapTextureData.size());
			REQUIRE(compressedImage->mipmapLevelCount > 0);

			auto texture = renderer.CreateTextureBuffer2D();
			texture->SetTextureDimensions(compressedImage->size, (int)compressedImage->mipmapLevelCount);
			texture->SetTextureFormat(compressedImage->imageFormat, compressedImage->dataFormat);
			texture->SetUsageFlags(ITextureBuffer::UsageFlags::ShaderInput);
			texture->SetPerformanceHints(ITextureBuffer::PerformanceHint::WriteRarely | ITextureBuffer::PerformanceHint::ReadNever, ITextureBuffer::PerformanceHint::WriteNever | ITextureBuffer::PerformanceHint::ReadOften);
			for (uint32_t mipmapLevel = 0; mipmapLevel < compressedImage->mipmapLevelCount; ++mipmapLevel)
			{
				const auto& mipmapData = compressedImage->mipmapTextureData[mipmapLevel];
				REQUIRE(texture->SetInitialData(mipmapData.data(), mipmapData.size(), compressedImage->sourceImageFormat, compressedImage->sourceDataFormat, (int)mipmapLevel));
			}
			REQUIRE(texture->AllocateMemory());

			renderableNode->BindTextureWithCombinedSampler(colorTextureId, texture.get(), sampler.get());

			auto frameBufferCapture = renderer.CreateFrameBufferOutput();
			frameBufferCapture->SetDetachAfterCapture(true);
			mainWindowFrameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
			DrawOneFrame();
			session.AddTestImageResult(testName, std::string("A 512x512 compressed ") + testCase.formatName + " texture.", std::move(frameBufferCapture), IImageDiff::Algorithm::NaiveDiff, 0.98f);
		}

		// Remove all our defined render passes so we can re-bind the same window below.
		renderer.RemoveAllRenderPasses();
	}
	renderer.WaitForDeferredDeletionComplete();

	// Define a perspective scene so the mip chain is visible along a plane receding into the distance.
	TexturedQuadSceneHelper::Scene perspectiveScene;
	REQUIRE(TexturedQuad().InitializeScene(this, session, Geometry(), TexturedQuad().PerspectiveVertexShader(), CompressedTextureFragmentShader, perspectiveScene));

	MipmappingHelper::PerspectivePlaneSettings perspectiveSettings;
	perspectiveSettings.texCoordScale = V2Float32(8.0f, 32.0f);
	REQUIRE(Mipmapping().ConfigurePerspectivePlaneScene(perspectiveScene, Transform(), session, perspectiveSettings, TexturedQuad()));

	auto sampler = renderer.CreateTextureSampler2D();
	sampler->SetTextureWrapMode(ITextureSampler::WrapMode::Repeat, ITextureSampler::WrapMode::Repeat);
	sampler->SetTextureFilterMode(ITextureSampler::FilterMode::Nearest, ITextureSampler::FilterMode::Nearest);
	sampler->SetAnisotropicFilterMode(false);

	auto colorTextureId = perspectiveScene.shaderProgram->GetTextureId("colorTexture");
	for (const auto& testCase : TestCases)
	{
		std::string testName = std::string("CompressedTexture") + testCase.formatName;
		if (!session.Device().IsTextureFormatSupported(testCase.imageFormat, testCase.dataFormat))
		{
			session.AddTestSkipped(testName + "Perspective", "This test was skipped, as the current graphics device does not support this compressed texture format.");
			continue;
		}

		std::unique_ptr<TextureHelper::CompressedTextureInfo> compressedImage;
		switch (testCase.fileType)
		{
		case CompressedTextureFileType::Dds:
			compressedImage = Texture().LoadImageFromDdsFile(testName + ".dds");
			break;
		case CompressedTextureFileType::Ktx:
			compressedImage = Texture().LoadImageFromKtxFile(testName + ".ktx");
			break;
		}
		REQUIRE(compressedImage != nullptr);
		REQUIRE(compressedImage->size == V2UInt32(AssumedTextureSize, AssumedTextureSize));
		REQUIRE(compressedImage->dataFormat == testCase.dataFormat);
		REQUIRE(static_cast<size_t>(compressedImage->mipmapLevelCount) == compressedImage->mipmapTextureData.size());
		REQUIRE(compressedImage->mipmapLevelCount > 0);
		REQUIRE(compressedImage->mipmapLevelCount > 1);

		auto texture = renderer.CreateTextureBuffer2D();
		texture->SetTextureDimensions(compressedImage->size, (int)compressedImage->mipmapLevelCount);
		texture->SetTextureFormat(compressedImage->imageFormat, compressedImage->dataFormat);
		texture->SetUsageFlags(ITextureBuffer::UsageFlags::ShaderInput);
		texture->SetPerformanceHints(ITextureBuffer::PerformanceHint::WriteRarely | ITextureBuffer::PerformanceHint::ReadNever, ITextureBuffer::PerformanceHint::WriteNever | ITextureBuffer::PerformanceHint::ReadOften);
		for (uint32_t mipmapLevel = 0; mipmapLevel < compressedImage->mipmapLevelCount; ++mipmapLevel)
		{
			const auto& mipmapData = compressedImage->mipmapTextureData[mipmapLevel];
			REQUIRE(texture->SetInitialData(mipmapData.data(), mipmapData.size(), compressedImage->sourceImageFormat, compressedImage->sourceDataFormat, (int)mipmapLevel));
		}
		REQUIRE(texture->AllocateMemory());
		perspectiveScene.renderableNode->BindTextureWithCombinedSampler(colorTextureId, texture.get(), sampler.get());

		const auto maxMipmapLevel = (float)(compressedImage->mipmapLevelCount - 1);
		sampler->SetTextureMipmapMode(ITextureSampler::MipmapMode::Linear);
		sampler->SetMipmapLevelMapping(0.0f, maxMipmapLevel, 0.0f);
		auto frameBufferCapture = renderer.CreateFrameBufferOutput();
		frameBufferCapture->SetDetachAfterCapture(true);
		perspectiveScene.frameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
		DrawOneFrame();
		session.AddTestImageResult(testName + "Perspective", std::string("A compressed ") + testCase.formatName + " texture sampled on a perspective floor plane with linear mipmap blending.", std::move(frameBufferCapture), IImageDiff::Algorithm::NaiveDiff, 0.98f);
	}

	// Remove all our defined render passes
	renderer.RemoveAllRenderPasses();
	return true;
}

} // namespace cobalt::graphics::testing
