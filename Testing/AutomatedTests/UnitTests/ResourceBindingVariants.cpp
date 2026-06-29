// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "../UnitTestBase.h"
namespace cobalt::graphics::testing {
using namespace cobalt::graphics;

namespace {

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
const std::string ResourceArrayFragmentShader = R"(
StructuredBuffer<float4> colorArray;

float4 main() : SV_TARGET0
{
    return colorArray[0];
}
)";

struct TextureBindingResources
{
	ITextureBuffer1D::unique_ptr texture1D;
	ITextureBuffer2D::unique_ptr texture2D;
	ITextureBuffer3D::unique_ptr texture3D;
	ITextureBufferCube::unique_ptr textureCube;
	ITextureBuffer1DArray::unique_ptr texture1DArray;
	ITextureBuffer2DArray::unique_ptr texture2DArray;
	ITextureBufferCubeArray::unique_ptr textureCubeArray;
	ITextureSampler1D::unique_ptr sampler1D;
	ITextureSampler2D::unique_ptr sampler2D;
	ITextureSampler3D::unique_ptr sampler3D;
	ITextureSamplerCube::unique_ptr samplerCube;
	ITextureSampler1DArray::unique_ptr sampler1DArray;
	ITextureSampler2DArray::unique_ptr sampler2DArray;
	ITextureSamplerCubeArray::unique_ptr samplerCubeArray;
};

void BindAndReplaceCombinedTextureSampler(IStateContainer& stateContainer, TextureId textureId, TextureBindingResources& resources)
{
	stateContainer.BindTextureWithCombinedSampler(textureId, resources.texture1D.get(), resources.sampler1D.get());
	stateContainer.BindTextureWithCombinedSampler(textureId, resources.texture1D.get(), resources.sampler1D.get());
	stateContainer.UnbindTexture(textureId);
	stateContainer.BindTextureWithCombinedSampler(textureId, resources.texture2D.get(), resources.sampler2D.get());
	stateContainer.BindTextureWithCombinedSampler(textureId, resources.texture2D.get(), resources.sampler2D.get());
	stateContainer.UnbindTexture(textureId);
	stateContainer.BindTextureWithCombinedSampler(textureId, resources.texture3D.get(), resources.sampler3D.get());
	stateContainer.BindTextureWithCombinedSampler(textureId, resources.texture3D.get(), resources.sampler3D.get());
	stateContainer.UnbindTexture(textureId);
	stateContainer.BindTextureWithCombinedSampler(textureId, resources.textureCube.get(), resources.samplerCube.get());
	stateContainer.BindTextureWithCombinedSampler(textureId, resources.textureCube.get(), resources.samplerCube.get());
	stateContainer.UnbindTexture(textureId);
	stateContainer.BindTextureWithCombinedSampler(textureId, resources.texture1DArray.get(), resources.sampler1DArray.get());
	stateContainer.BindTextureWithCombinedSampler(textureId, resources.texture1DArray.get(), resources.sampler1DArray.get());
	stateContainer.UnbindTexture(textureId);
	stateContainer.BindTextureWithCombinedSampler(textureId, resources.texture2DArray.get(), resources.sampler2DArray.get());
	stateContainer.BindTextureWithCombinedSampler(textureId, resources.texture2DArray.get(), resources.sampler2DArray.get());
	stateContainer.UnbindTexture(textureId);
	if (resources.textureCubeArray && resources.samplerCubeArray)
	{
		stateContainer.BindTextureWithCombinedSampler(textureId, resources.textureCubeArray.get(), resources.samplerCubeArray.get());
		stateContainer.BindTextureWithCombinedSampler(textureId, resources.textureCubeArray.get(), resources.samplerCubeArray.get());
		stateContainer.UnbindTexture(textureId);
	}
}

void BindAndReplaceSeparateTextureSampler(IStateContainer& stateContainer, TextureId textureId, SamplerId samplerId, TextureBindingResources& resources)
{
	stateContainer.BindTexture(textureId, resources.texture1D.get());
	stateContainer.BindTexture(textureId, resources.texture1D.get());
	stateContainer.UnbindTexture(textureId);
	stateContainer.BindTexture(textureId, resources.texture2D.get());
	stateContainer.BindTexture(textureId, resources.texture2D.get());
	stateContainer.UnbindTexture(textureId);
	stateContainer.BindTexture(textureId, resources.texture3D.get());
	stateContainer.BindTexture(textureId, resources.texture3D.get());
	stateContainer.UnbindTexture(textureId);
	stateContainer.BindTexture(textureId, resources.textureCube.get());
	stateContainer.BindTexture(textureId, resources.textureCube.get());
	stateContainer.UnbindTexture(textureId);
	stateContainer.BindTexture(textureId, resources.texture1DArray.get());
	stateContainer.BindTexture(textureId, resources.texture1DArray.get());
	stateContainer.UnbindTexture(textureId);
	stateContainer.BindTexture(textureId, resources.texture2DArray.get());
	stateContainer.BindTexture(textureId, resources.texture2DArray.get());
	stateContainer.UnbindTexture(textureId);
	if (resources.textureCubeArray)
	{
		stateContainer.BindTexture(textureId, resources.textureCubeArray.get());
		stateContainer.BindTexture(textureId, resources.textureCubeArray.get());
		stateContainer.UnbindTexture(textureId);
	}

	stateContainer.BindSampler(samplerId, resources.sampler1D.get());
	stateContainer.BindSampler(samplerId, resources.sampler1D.get());
	stateContainer.UnbindSampler(samplerId);
	stateContainer.BindSampler(samplerId, resources.sampler2D.get());
	stateContainer.BindSampler(samplerId, resources.sampler2D.get());
	stateContainer.UnbindSampler(samplerId);
	stateContainer.BindSampler(samplerId, resources.sampler3D.get());
	stateContainer.BindSampler(samplerId, resources.sampler3D.get());
	stateContainer.UnbindSampler(samplerId);
	stateContainer.BindSampler(samplerId, resources.samplerCube.get());
	stateContainer.BindSampler(samplerId, resources.samplerCube.get());
	stateContainer.UnbindSampler(samplerId);
	stateContainer.BindSampler(samplerId, resources.sampler1DArray.get());
	stateContainer.BindSampler(samplerId, resources.sampler1DArray.get());
	stateContainer.UnbindSampler(samplerId);
	stateContainer.BindSampler(samplerId, resources.sampler2DArray.get());
	stateContainer.BindSampler(samplerId, resources.sampler2DArray.get());
	stateContainer.UnbindSampler(samplerId);
	if (resources.samplerCubeArray)
	{
		stateContainer.BindSampler(samplerId, resources.samplerCubeArray.get());
		stateContainer.BindSampler(samplerId, resources.samplerCubeArray.get());
		stateContainer.UnbindSampler(samplerId);
	}
}

void BindAndReplaceResourceArrays(IStateContainer& stateContainer, ResourceArrayId resourceArrayId, IDataArray* dataArray, ITexelArray* texelArray)
{
	stateContainer.BindResourceArray(resourceArrayId, dataArray, false);
	stateContainer.BindResourceArray(resourceArrayId, dataArray, true);
	stateContainer.UnbindResourceArray(resourceArrayId);
	stateContainer.BindResourceArray(resourceArrayId, texelArray);
	stateContainer.BindResourceArray(resourceArrayId, texelArray);
	stateContainer.UnbindResourceArray(resourceArrayId);
}

} // namespace

DEFINE_UNIT_TEST_WITH_BASE("Resources/StateValue/ResourceBindingVariants", UnitTestBase)
{
	// Retrieve our important objects
	auto& renderer = session.Renderer();

	// Create one resource of each texture and sampler family. The state-container contract only records the binding
	// relationships here, so these resources don't need memory allocation or shader-visible contents.
	TextureBindingResources resources;
	resources.texture1D = renderer.CreateTextureBuffer1D();
	resources.texture2D = renderer.CreateTextureBuffer2D();
	resources.texture3D = renderer.CreateTextureBuffer3D();
	resources.textureCube = renderer.CreateTextureBufferCube();
	resources.texture1DArray = renderer.CreateTextureBuffer1DArray();
	resources.texture2DArray = renderer.CreateTextureBuffer2DArray();
	resources.sampler1D = renderer.CreateTextureSampler1D();
	resources.sampler2D = renderer.CreateTextureSampler2D();
	resources.sampler3D = renderer.CreateTextureSampler3D();
	resources.samplerCube = renderer.CreateTextureSamplerCube();
	resources.sampler1DArray = renderer.CreateTextureSampler1DArray();
	resources.sampler2DArray = renderer.CreateTextureSampler2DArray();
	if (session.Device().IsFeatureSupported(IGraphicsDevice::Feature::TextureCubeArray))
	{
		resources.textureCubeArray = renderer.CreateTextureBufferCubeArray();
		resources.samplerCubeArray = renderer.CreateTextureSamplerCubeArray();
	}

	// Exercise the binding, replacement, and unbinding paths for renderable, state-group, and default-state containers.
	auto renderableNode = renderer.CreateRenderableNode();
	auto stateGroupNode = renderer.CreateStateGroupNode();
	auto defaultState = renderer.CreateDefaultState();
	auto textureId = TextureId(1);
	auto samplerId = SamplerId(2);
	BindAndReplaceCombinedTextureSampler(*renderableNode, textureId, resources);
	BindAndReplaceCombinedTextureSampler(*stateGroupNode, textureId, resources);
	BindAndReplaceCombinedTextureSampler(*defaultState, textureId, resources);
	BindAndReplaceSeparateTextureSampler(*renderableNode, textureId, samplerId, resources);
	BindAndReplaceSeparateTextureSampler(*stateGroupNode, textureId, samplerId, resources);
	BindAndReplaceSeparateTextureSampler(*defaultState, textureId, samplerId, resources);

	// Add a live image result for the texture and sampler binding variant pass, so the coverage case has a reference
	// image instead of only reporting that the API calls were accepted.
	{
		TexturedQuadSceneHelper::Scene scene;
		REQUIRE(TexturedQuad().InitializeScene(this, session, Geometry(), TextureFragmentShader, scene));

		std::vector<V4UInt8> textureData(4, V4UInt8(0, 0, 255, 255));
		auto texture = renderer.CreateTextureBuffer2D();
		texture->SetUsageFlags(ITextureBuffer::UsageFlags::ShaderInput);
		texture->SetTextureFormat(ITextureBuffer::ImageFormat::RGBA, ITextureBuffer::DataFormat::UNorm8);
		texture->SetTextureDimensions(V2UInt32(2, 2));
		REQUIRE(texture->SetInitialData(textureData));
		REQUIRE(texture->AllocateMemory());

		auto sampler = renderer.CreateTextureSampler2D();
		sampler->SetTextureFilterMode(ITextureSampler::FilterMode::Nearest, ITextureSampler::FilterMode::Nearest);
		sampler->SetTextureWrapMode(ITextureSampler::WrapMode::ClampToEdge, ITextureSampler::WrapMode::ClampToEdge);
		scene.renderableNode->BindTextureWithCombinedSampler(scene.shaderProgram->GetTextureId("colorTexture"), texture.get(), sampler.get());

		auto frameBufferCapture = renderer.CreateFrameBufferOutput();
		frameBufferCapture->SetDetachAfterCapture(true);
		scene.frameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
		DrawOneFrame();
		session.AddTestImageResult("TextureAndSamplerBindingVariants", "A fullscreen blue quad rendered after exercising texture and sampler binding replacement variants.", std::move(frameBufferCapture), IImageDiff::Algorithm::AllOfThem, 0.95);
		renderer.RemoveAllRenderPasses();
		scene = TexturedQuadSceneHelper::Scene();
		renderer.WaitForDeferredDeletionComplete();
	}

	// Resource arrays are optional, so only exercise the data-array and texel-array binding paths when supported.
	if (!session.Device().IsFeatureSupported(IGraphicsDevice::Feature::ResourceArrays))
	{
		session.AddTestSkipped("ResourceArrayBindingVariants", "Resource-array binding variant coverage was skipped, as the current renderer doesn't support resource arrays on this device.");
	}
	else
	{
		auto dataArray = renderer.CreateDataArray();
		auto texelArray = renderer.CreateTexelArray();
		auto resourceArrayId = ResourceArrayId(3);
		BindAndReplaceResourceArrays(*renderableNode, resourceArrayId, dataArray.get(), texelArray.get());
		BindAndReplaceResourceArrays(*stateGroupNode, resourceArrayId, dataArray.get(), texelArray.get());
		BindAndReplaceResourceArrays(*defaultState, resourceArrayId, dataArray.get(), texelArray.get());

		TexturedQuadSceneHelper::Scene scene;
		REQUIRE(TexturedQuad().InitializeScene(this, session, Geometry(), ResourceArrayFragmentShader, scene));
		V4Float32 color(1.0f, 0.0f, 1.0f, 1.0f);
		auto visibleDataArray = renderer.CreateDataArray();
		visibleDataArray->SetUsageFlags(IDataArray::UsageFlags::ShaderInput);
		visibleDataArray->SetBufferLayout(sizeof(V4Float32), 1);
		REQUIRE(visibleDataArray->SetInitialData(&color, sizeof(color)));
		REQUIRE(visibleDataArray->AllocateMemory());
		scene.renderableNode->BindResourceArray(scene.shaderProgram->GetResourceArrayId("colorArray"), visibleDataArray.get());

		auto frameBufferCapture = renderer.CreateFrameBufferOutput();
		frameBufferCapture->SetDetachAfterCapture(true);
		scene.frameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
		DrawOneFrame();
		session.AddTestImageResult("ResourceArrayBindingVariants", "A fullscreen magenta quad rendered after exercising resource-array binding replacement variants.", std::move(frameBufferCapture), IImageDiff::Algorithm::AllOfThem, 0.95);
		renderer.RemoveAllRenderPasses();
		scene = TexturedQuadSceneHelper::Scene();
		renderer.WaitForDeferredDeletionComplete();
	}

	// Exercise a representative texture binding with live shader-visible resources, proving that a renderable binding
	// overrides the default-state binding and that unbinding the renderable entry restores the default-state result.
	{
		TexturedQuadSceneHelper::Scene scene;
		REQUIRE(TexturedQuad().InitializeScene(this, session, Geometry(), TextureFragmentShader, scene));

		std::vector<V4UInt8> redTextureData(4, V4UInt8(255, 0, 0, 255));
		std::vector<V4UInt8> greenTextureData(4, V4UInt8(0, 255, 0, 255));
		auto redTexture = renderer.CreateTextureBuffer2D();
		redTexture->SetUsageFlags(ITextureBuffer::UsageFlags::ShaderInput);
		redTexture->SetTextureFormat(ITextureBuffer::ImageFormat::RGBA, ITextureBuffer::DataFormat::UNorm8);
		redTexture->SetTextureDimensions(V2UInt32(2, 2));
		REQUIRE(redTexture->SetInitialData(redTextureData));
		REQUIRE(redTexture->AllocateMemory());

		auto greenTexture = renderer.CreateTextureBuffer2D();
		greenTexture->SetUsageFlags(ITextureBuffer::UsageFlags::ShaderInput);
		greenTexture->SetTextureFormat(ITextureBuffer::ImageFormat::RGBA, ITextureBuffer::DataFormat::UNorm8);
		greenTexture->SetTextureDimensions(V2UInt32(2, 2));
		REQUIRE(greenTexture->SetInitialData(greenTextureData));
		REQUIRE(greenTexture->AllocateMemory());

		auto sampler = renderer.CreateTextureSampler2D();
		sampler->SetTextureFilterMode(ITextureSampler::FilterMode::Nearest, ITextureSampler::FilterMode::Nearest);
		sampler->SetTextureWrapMode(ITextureSampler::WrapMode::ClampToEdge, ITextureSampler::WrapMode::ClampToEdge);

		auto representativeDefaultState = renderer.CreateDefaultState();
		auto colorTextureId = scene.shaderProgram->GetTextureId("colorTexture");
		representativeDefaultState->BindTextureWithCombinedSampler(colorTextureId, redTexture.get(), sampler.get());
		scene.renderableNode->BindTextureWithCombinedSampler(colorTextureId, greenTexture.get(), sampler.get());
		scene.renderPassNode->RemoveAllChildNodes();
		scene.renderPassNode->AddChildNode(scene.programNode.get(), representativeDefaultState.get());

		auto frameBufferCapture = renderer.CreateFrameBufferOutput();
		frameBufferCapture->SetDetachAfterCapture(true);
		scene.frameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
		DrawOneFrame();
		session.AddTestImageResult("RepresentativeRenderableTextureBinding", "A fullscreen green quad using the renderable texture binding after replacement.", std::move(frameBufferCapture), IImageDiff::Algorithm::AllOfThem, 0.95);

		scene.renderableNode->UnbindTexture(colorTextureId);
		frameBufferCapture = renderer.CreateFrameBufferOutput();
		frameBufferCapture->SetDetachAfterCapture(true);
		scene.frameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
		DrawOneFrame();
		session.AddTestImageResult("RepresentativeDefaultTextureBindingAfterUnbind", "A fullscreen red quad using the default-state texture binding after unbinding the renderable texture.", std::move(frameBufferCapture), IImageDiff::Algorithm::AllOfThem, 0.95);
		renderer.RemoveAllRenderPasses();
	}

	renderer.WaitForDeferredDeletionComplete();
	return true;
}

} // namespace cobalt::graphics::testing
