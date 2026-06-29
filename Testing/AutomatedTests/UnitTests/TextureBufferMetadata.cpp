// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "../UnitTestBase.h"
namespace cobalt::graphics::testing {
using namespace cobalt::graphics;

namespace {

std::vector<V4UInt8> CreateColorPixels(size_t pixelCount, const V4UInt8& color)
{
	return std::vector<V4UInt8>(pixelCount, color);
}

std::vector<V3UInt8> CreateThreeComponentPixels(size_t pixelCount, const V3UInt8& color)
{
	return std::vector<V3UInt8>(pixelCount, color);
}

} // namespace

DEFINE_UNIT_TEST_WITH_BASE("Resources/Images/TextureBufferMetadata", UnitTestBase)
{
	// Retrieve our important objects
	auto& renderer = session.Renderer();

	// Create a mipmapped 2D texture and verify that the allocation metadata is reported back correctly.
	auto texture = renderer.CreateTextureBuffer2D();
	texture->SetUsageFlags(ITextureBuffer::UsageFlags::ShaderInput);
	texture->SetPerformanceHints(ITextureBuffer::PerformanceHint::WriteRarely | ITextureBuffer::PerformanceHint::ReadNever, ITextureBuffer::PerformanceHint::WriteNever | ITextureBuffer::PerformanceHint::ReadOften);
	texture->SetDataPersistenceFlags(ITextureBuffer::DataPersistenceFlags::InvalidateExistingDataOnWrite | ITextureBuffer::DataPersistenceFlags::InvalidateExistingDataAfterDrawComplete);
	texture->SetTextureFormat(ITextureBuffer::ImageFormat::RGBA, ITextureBuffer::DataFormat::UNorm8);
	texture->SetTextureDimensions(V2UInt32(16, 8), 4);

	std::vector<V4UInt8> mip0((size_t)16 * 8, V4UInt8(255, 0, 0, 255));
	std::vector<V4UInt8> mip1((size_t)8 * 4, V4UInt8(0, 255, 0, 255));
	std::vector<V4UInt8> mip2((size_t)4 * 2, V4UInt8(0, 0, 255, 255));
	std::vector<V4UInt8> mip3((size_t)2 * 1, V4UInt8(255, 255, 0, 255));
	REQUIRE(texture->SetInitialData(mip0, 0));
	REQUIRE(texture->SetInitialData(mip1, 1));
	REQUIRE(texture->SetInitialData(mip2, 2));
	REQUIRE(texture->SetInitialData(mip3, 3));
	REQUIRE(texture->AllocateMemory());

	REQUIRE(texture->AllocatedImageFormat() == ITextureBuffer::ImageFormat::RGBA);
	REQUIRE(texture->AllocatedDataFormat() == ITextureBuffer::DataFormat::UNorm8);
	REQUIRE(texture->MipmapLevelCount() == 4);
	REQUIRE(texture->MipmapLevelDimensions(0) == V2UInt32(16, 8));
	REQUIRE(texture->MipmapLevelDimensions(1) == V2UInt32(8, 4));
	REQUIRE(texture->MipmapLevelDimensions(2) == V2UInt32(4, 2));
	REQUIRE(texture->MipmapLevelDimensions(3) == V2UInt32(2, 1));
	session.AddTestSuccess("TextureBufferMetadata", "A mipmapped 2D texture reported its allocated format and mipmap dimensions correctly.");

	// Validate texture setup and mutation contracts that should fail before the renderer attempts native allocation or
	// upload work.
	{
		auto textureWithoutFormat = renderer.CreateTextureBuffer2D();
		textureWithoutFormat->SetTextureDimensions(V2UInt32(2, 2));
		REQUIRE(!textureWithoutFormat->AllocateMemory());

		auto textureWithoutDimensions = renderer.CreateTextureBuffer2D();
		textureWithoutDimensions->SetTextureFormat(ITextureBuffer::ImageFormat::RGBA, ITextureBuffer::DataFormat::UNorm8);
		auto textureWithoutDimensionsInitialData = CreateColorPixels(4, V4UInt8(255, 0, 0, 255));
		REQUIRE(!textureWithoutDimensions->SetInitialData(textureWithoutDimensionsInitialData));
		REQUIRE(!textureWithoutDimensions->AllocateMemory());

		auto textureWithZeroDimension = renderer.CreateTextureBuffer2D();
		textureWithZeroDimension->SetTextureFormat(ITextureBuffer::ImageFormat::RGBA, ITextureBuffer::DataFormat::UNorm8);
		textureWithZeroDimension->SetTextureDimensions(V2UInt32(0, 2));
		REQUIRE(!textureWithZeroDimension->AllocateMemory());

		auto textureWithPartialInitialData = renderer.CreateTextureBuffer2D();
		textureWithPartialInitialData->SetTextureFormat(ITextureBuffer::ImageFormat::RGBA, ITextureBuffer::DataFormat::UNorm8);
		textureWithPartialInitialData->SetTextureDimensions(V2UInt32(4, 4), 2);
		auto partialInitialData = CreateColorPixels(16, V4UInt8(255, 0, 0, 255));
		REQUIRE(textureWithPartialInitialData->SetInitialData(partialInitialData, 0));
		REQUIRE(!textureWithPartialInitialData->AllocateMemory());

		auto textureWithDuplicateInitialData = renderer.CreateTextureBuffer2D();
		textureWithDuplicateInitialData->SetTextureFormat(ITextureBuffer::ImageFormat::RGBA, ITextureBuffer::DataFormat::UNorm8);
		textureWithDuplicateInitialData->SetTextureDimensions(V2UInt32(2, 2));
		auto duplicateInitialDataA = CreateColorPixels(4, V4UInt8(255, 0, 0, 255));
		auto duplicateInitialDataB = CreateColorPixels(4, V4UInt8(0, 255, 0, 255));
		REQUIRE(textureWithDuplicateInitialData->SetInitialData(duplicateInitialDataA));
		REQUIRE(!textureWithDuplicateInitialData->SetInitialData(duplicateInitialDataB));

		auto textureUpdateBeforeAllocation = renderer.CreateTextureBuffer2D();
		textureUpdateBeforeAllocation->SetTextureFormat(ITextureBuffer::ImageFormat::RGBA, ITextureBuffer::DataFormat::UNorm8);
		textureUpdateBeforeAllocation->SetTextureDimensions(V2UInt32(2, 2));
		REQUIRE(!textureUpdateBeforeAllocation->QueueDataUpdate(CreateColorPixels(4, V4UInt8(255, 255, 255, 255))));

		auto immutableTexture = renderer.CreateTextureBuffer2D();
		immutableTexture->SetTextureFormat(ITextureBuffer::ImageFormat::RGBA, ITextureBuffer::DataFormat::UNorm8);
		immutableTexture->SetTextureDimensions(V2UInt32(2, 2));
		immutableTexture->SetPerformanceHints(ITextureBuffer::PerformanceHint::WriteNever | ITextureBuffer::PerformanceHint::ReadNever, ITextureBuffer::PerformanceHint::WriteNever | ITextureBuffer::PerformanceHint::ReadOften);
		auto immutableInitialData = CreateColorPixels(4, V4UInt8(0, 0, 255, 255));
		REQUIRE(immutableTexture->SetInitialData(immutableInitialData));
		REQUIRE(immutableTexture->AllocateMemory());
		REQUIRE(!immutableTexture->QueueDataUpdate(CreateColorPixels(4, V4UInt8(255, 255, 255, 255))));

		auto allocatedTexture = renderer.CreateTextureBuffer2D();
		allocatedTexture->SetTextureFormat(ITextureBuffer::ImageFormat::RGBA, ITextureBuffer::DataFormat::UNorm8);
		allocatedTexture->SetTextureDimensions(V2UInt32(2, 2));
		allocatedTexture->SetPerformanceHints(ITextureBuffer::PerformanceHint::WriteOften | ITextureBuffer::PerformanceHint::ReadNever, ITextureBuffer::PerformanceHint::WriteNever | ITextureBuffer::PerformanceHint::ReadOften);
		auto rgbInitialData = CreateThreeComponentPixels(4, V3UInt8(32, 64, 128));
		REQUIRE(allocatedTexture->SetInitialData(rgbInitialData.data(), rgbInitialData.size() * sizeof(V3UInt8), ITextureBuffer::SourceImageFormat::RGB, ITextureBuffer::SourceDataFormat::UInt8));
		REQUIRE(allocatedTexture->AllocateMemory());
		auto postAllocationInitialData = CreateColorPixels(4, V4UInt8(255, 0, 0, 255));
		REQUIRE(!allocatedTexture->SetInitialData(postAllocationInitialData));
		REQUIRE(!allocatedTexture->QueueDataUpdate(CreateColorPixels(4, V4UInt8(255, 255, 255, 255)), 1));
		REQUIRE(!allocatedTexture->QueueDataUpdate(CreateColorPixels(4, V4UInt8(255, 255, 255, 255)), 0, V2UInt32(1, 1), V2UInt32(2, 2)));

		auto completedTransferBatch = renderer.CreateTransferBatch(ITransferBatch::StartTiming::Immediately, ITransferBatch::EndTiming::AnyFrame);
		REQUIRE(completedTransferBatch->SubmitBatch());
		REQUIRE(completedTransferBatch->IsComplete());
		REQUIRE(!allocatedTexture->QueueDataUpdate(CreateColorPixels(4, V4UInt8(255, 255, 255, 255)), 0, V2UInt32(0, 0), V2UInt32(2, 2), completedTransferBatch.get()));
	}
	session.AddTestSuccess("TextureBufferContracts2D", "2D texture buffers rejected invalid allocation, initial data, update region, immutability, and submitted transfer batch states.");

	// Exercise the 1D and 3D dimension-specific validation paths as well, so the renderer validates and reports each
	// texture coordinate shape consistently.
	{
		auto texture1D = renderer.CreateTextureBuffer1D();
		texture1D->SetTextureFormat(ITextureBuffer::ImageFormat::RGBA, ITextureBuffer::DataFormat::UNorm8);
		texture1D->SetTextureDimensions(V1UInt32(4));
		texture1D->SetPerformanceHints(ITextureBuffer::PerformanceHint::WriteOften | ITextureBuffer::PerformanceHint::ReadNever, ITextureBuffer::PerformanceHint::WriteNever | ITextureBuffer::PerformanceHint::ReadOften);
		auto texture1DInitialData = CreateColorPixels(4, V4UInt8(255, 0, 0, 255));
		REQUIRE(texture1D->SetInitialData(texture1DInitialData));
		REQUIRE(texture1D->AllocateMemory());
		REQUIRE(!texture1D->QueueDataUpdate(CreateColorPixels(2, V4UInt8(0, 255, 0, 255)), 0, V1UInt32(3), V1UInt32(2)));

		auto texture3D = renderer.CreateTextureBuffer3D();
		texture3D->SetTextureFormat(ITextureBuffer::ImageFormat::RGBA, ITextureBuffer::DataFormat::UNorm8);
		texture3D->SetTextureDimensions(V3UInt32(2, 2, 2));
		texture3D->SetPerformanceHints(ITextureBuffer::PerformanceHint::WriteOften | ITextureBuffer::PerformanceHint::ReadNever, ITextureBuffer::PerformanceHint::WriteNever | ITextureBuffer::PerformanceHint::ReadOften);
		auto texture3DInitialData = CreateColorPixels(8, V4UInt8(0, 0, 255, 255));
		REQUIRE(texture3D->SetInitialData(texture3DInitialData));
		REQUIRE(texture3D->AllocateMemory());
		REQUIRE(!texture3D->QueueDataUpdate(CreateColorPixels(2, V4UInt8(255, 255, 255, 255)), 0, V3UInt32(1, 1, 1), V3UInt32(2, 1, 1)));
	}
	session.AddTestSuccess("TextureBufferDimensionContracts", "1D and 3D texture buffers rejected update regions outside their image dimensions.");

	// Exercise array and cube texture update paths with small textures, including successful partial updates and
	// boundary checks for array indices, faces, mip levels, and update regions.
	{
		auto texture1DArray = renderer.CreateTextureBuffer1DArray();
		texture1DArray->SetTextureFormat(ITextureBuffer::ImageFormat::RGBA, ITextureBuffer::DataFormat::UNorm8);
		texture1DArray->SetTextureDimensions(V1UInt32(4), 2);
		texture1DArray->SetPerformanceHints(ITextureBuffer::PerformanceHint::WriteOften | ITextureBuffer::PerformanceHint::ReadNever, ITextureBuffer::PerformanceHint::WriteNever | ITextureBuffer::PerformanceHint::ReadOften);
		auto texture1DArrayLayer0 = CreateColorPixels(4, V4UInt8(255, 0, 0, 255));
		auto texture1DArrayLayer1 = CreateColorPixels(4, V4UInt8(0, 255, 0, 255));
		REQUIRE(texture1DArray->SetInitialData(texture1DArrayLayer0, 0));
		REQUIRE(texture1DArray->SetInitialData(texture1DArrayLayer1, 1));
		REQUIRE(texture1DArray->AllocateMemory());
		REQUIRE(texture1DArray->AllocatedImageFormat() == ITextureBuffer::ImageFormat::RGBA);
		REQUIRE(texture1DArray->AllocatedDataFormat() == ITextureBuffer::DataFormat::UNorm8);
		REQUIRE(texture1DArray->MipmapLevelCount() == 1);
		REQUIRE(texture1DArray->MipmapLevelDimensions(0) == V1UInt32(4));
		REQUIRE(texture1DArray->QueueDataUpdate(CreateColorPixels(2, V4UInt8(0, 0, 255, 255)), 1, 0, V1UInt32(1), V1UInt32(2)));
		REQUIRE(!texture1DArray->QueueDataUpdate(CreateColorPixels(1, V4UInt8(255, 255, 255, 255)), 2));
		REQUIRE(!texture1DArray->QueueDataUpdate(CreateColorPixels(2, V4UInt8(255, 255, 255, 255)), 0, 0, V1UInt32(3), V1UInt32(2)));

		auto texture2DArray = renderer.CreateTextureBuffer2DArray();
		texture2DArray->SetTextureFormat(ITextureBuffer::ImageFormat::RGBA, ITextureBuffer::DataFormat::UNorm8);
		texture2DArray->SetTextureDimensions(V2UInt32(2, 2), 2);
		texture2DArray->SetPerformanceHints(ITextureBuffer::PerformanceHint::WriteOften | ITextureBuffer::PerformanceHint::ReadNever, ITextureBuffer::PerformanceHint::WriteNever | ITextureBuffer::PerformanceHint::ReadOften);
		auto texture2DArrayLayer0 = CreateColorPixels(4, V4UInt8(255, 0, 0, 255));
		auto texture2DArrayLayer1 = CreateColorPixels(4, V4UInt8(0, 255, 0, 255));
		REQUIRE(texture2DArray->SetInitialData(texture2DArrayLayer0, 0));
		REQUIRE(texture2DArray->SetInitialData(texture2DArrayLayer1, 1));
		REQUIRE(texture2DArray->AllocateMemory());
		REQUIRE(texture2DArray->AllocatedImageFormat() == ITextureBuffer::ImageFormat::RGBA);
		REQUIRE(texture2DArray->AllocatedDataFormat() == ITextureBuffer::DataFormat::UNorm8);
		REQUIRE(texture2DArray->MipmapLevelCount() == 1);
		REQUIRE(texture2DArray->MipmapLevelDimensions(0) == V2UInt32(2, 2));
		REQUIRE(texture2DArray->QueueDataUpdate(CreateColorPixels(2, V4UInt8(0, 0, 255, 255)), 1, 0, V2UInt32(0, 1), V2UInt32(2, 1)));
		REQUIRE(!texture2DArray->QueueDataUpdate(CreateColorPixels(1, V4UInt8(255, 255, 255, 255)), 2));
		REQUIRE(!texture2DArray->QueueDataUpdate(CreateColorPixels(2, V4UInt8(255, 255, 255, 255)), 0, 0, V2UInt32(1, 1), V2UInt32(2, 1)));

		const ITextureBuffer::CubeMapFace cubeFaces[] = {
		  ITextureBuffer::CubeMapFace::PositiveX,
		  ITextureBuffer::CubeMapFace::NegativeX,
		  ITextureBuffer::CubeMapFace::PositiveY,
		  ITextureBuffer::CubeMapFace::NegativeY,
		  ITextureBuffer::CubeMapFace::PositiveZ,
		  ITextureBuffer::CubeMapFace::NegativeZ,
		};

		auto textureCube = renderer.CreateTextureBufferCube();
		textureCube->SetTextureFormat(ITextureBuffer::ImageFormat::RGBA, ITextureBuffer::DataFormat::UNorm8);
		textureCube->SetTextureDimensions(2);
		textureCube->SetPerformanceHints(ITextureBuffer::PerformanceHint::WriteOften | ITextureBuffer::PerformanceHint::ReadNever, ITextureBuffer::PerformanceHint::WriteNever | ITextureBuffer::PerformanceHint::ReadOften);
		auto textureCubeFaceData = CreateColorPixels(4, V4UInt8(255, 0, 0, 255));
		for (auto face : cubeFaces)
		{
			REQUIRE(textureCube->SetInitialData(textureCubeFaceData, face));
		}
		REQUIRE(textureCube->AllocateMemory());
		REQUIRE(textureCube->AllocatedImageFormat() == ITextureBuffer::ImageFormat::RGBA);
		REQUIRE(textureCube->AllocatedDataFormat() == ITextureBuffer::DataFormat::UNorm8);
		REQUIRE(textureCube->MipmapLevelCount() == 1);
		REQUIRE(textureCube->MipmapLevelDimensions(0) == V2UInt32(2, 2));
		REQUIRE(textureCube->QueueDataUpdate(CreateColorPixels(2, V4UInt8(0, 0, 255, 255)), ITextureBuffer::CubeMapFace::PositiveX, 0, V2UInt32(0, 1), V2UInt32(2, 1)));
		REQUIRE(!textureCube->QueueDataUpdate(CreateColorPixels(1, V4UInt8(255, 255, 255, 255)), ITextureBuffer::CubeMapFace::PositiveX, 1));
		REQUIRE(!textureCube->QueueDataUpdate(CreateColorPixels(2, V4UInt8(255, 255, 255, 255)), ITextureBuffer::CubeMapFace::PositiveX, 0, V2UInt32(1, 1), V2UInt32(2, 1)));

		if (session.Device().IsFeatureSupported(IGraphicsDevice::Feature::TextureCubeArray))
		{
			auto textureCubeArray = renderer.CreateTextureBufferCubeArray();
			textureCubeArray->SetTextureFormat(ITextureBuffer::ImageFormat::RGBA, ITextureBuffer::DataFormat::UNorm8);
			textureCubeArray->SetTextureDimensions(2, 2);
			textureCubeArray->SetPerformanceHints(ITextureBuffer::PerformanceHint::WriteOften | ITextureBuffer::PerformanceHint::ReadNever, ITextureBuffer::PerformanceHint::WriteNever | ITextureBuffer::PerformanceHint::ReadOften);
			auto textureCubeArrayFaceData = CreateColorPixels(4, V4UInt8(255, 0, 0, 255));
			for (size_t arrayIndex = 0; arrayIndex < 2; ++arrayIndex)
			{
				for (auto face : cubeFaces)
				{
					REQUIRE(textureCubeArray->SetInitialData(textureCubeArrayFaceData, face, arrayIndex));
				}
			}
			REQUIRE(textureCubeArray->AllocateMemory());
			REQUIRE(textureCubeArray->AllocatedImageFormat() == ITextureBuffer::ImageFormat::RGBA);
			REQUIRE(textureCubeArray->AllocatedDataFormat() == ITextureBuffer::DataFormat::UNorm8);
			REQUIRE(textureCubeArray->MipmapLevelCount() == 1);
			REQUIRE(textureCubeArray->MipmapLevelDimensions(0) == V2UInt32(2, 2));
			REQUIRE(textureCubeArray->QueueDataUpdate(CreateColorPixels(2, V4UInt8(0, 0, 255, 255)), ITextureBuffer::CubeMapFace::PositiveX, 1, 0, V2UInt32(0, 1), V2UInt32(2, 1)));
			REQUIRE(!textureCubeArray->QueueDataUpdate(CreateColorPixels(1, V4UInt8(255, 255, 255, 255)), ITextureBuffer::CubeMapFace::PositiveX, 2));
			REQUIRE(!textureCubeArray->QueueDataUpdate(CreateColorPixels(2, V4UInt8(255, 255, 255, 255)), ITextureBuffer::CubeMapFace::PositiveX, 0, 0, V2UInt32(1, 1), V2UInt32(2, 1)));
		}
		else
		{
			session.AddTestSkipped("TextureCubeArrayContracts", "This part of the test was skipped, as the current renderer doesn't support cube texture arrays on this device.");
		}
	}
	session.AddTestSuccess("TextureBufferArrayAndCubeContracts", "Array and cube texture buffers reported metadata correctly and validated partial updates, array indices, mip levels, and update regions.");
	return true;
}

} // namespace cobalt::graphics::testing
