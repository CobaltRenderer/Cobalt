// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "../UnitTestBase.h"
#include <array>
#include <vector>
namespace cobalt::graphics::testing {
using namespace cobalt::graphics;

namespace {

struct TransferBatchConfig
{
	const char* name;
	ITransferBatch::StartTiming startTiming;
	ITransferBatch::EndTiming endTiming;
};

} // namespace

DEFINE_UNIT_TEST_WITH_BASE("Resources/Batching/TransferBatchLifecycle", UnitTestBase)
{
	// Retrieve our important objects
	auto& renderer = session.Renderer();

	auto waitForBatchComplete = [&](ITransferBatch& transferBatch) {
		for (size_t frameNo = 0; (frameNo < 8) && !transferBatch.IsComplete(); ++frameNo)
		{
			DrawOneFrame();
		}
		transferBatch.WaitForComplete();
		REQUIRE(transferBatch.IsComplete());
	};

	const std::array<TransferBatchConfig, 4> transferBatchConfigs = {{
	  {"AfterCurrentFrameBeforeNextFrame", ITransferBatch::StartTiming::AfterCurrentFrame, ITransferBatch::EndTiming::BeforeNextFrame},
	  {"AfterCurrentFrameAnyFrame", ITransferBatch::StartTiming::AfterCurrentFrame, ITransferBatch::EndTiming::AnyFrame},
	  {"ImmediatelyBeforeNextFrame", ITransferBatch::StartTiming::Immediately, ITransferBatch::EndTiming::BeforeNextFrame},
	  {"ImmediatelyAnyFrame", ITransferBatch::StartTiming::Immediately, ITransferBatch::EndTiming::AnyFrame},
	}};

	for (const auto& config : transferBatchConfigs)
	{
		auto transferBatch = renderer.CreateTransferBatch(config.startTiming, config.endTiming);
		REQUIRE(!transferBatch->IsSubmitted());
		REQUIRE(transferBatch->SubmitBatch());
		REQUIRE(transferBatch->IsSubmitted());

		waitForBatchComplete(*transferBatch);
		session.AddTestSuccess(std::string("EmptyBatch") + config.name, "An empty transfer batch reported submission and completion state correctly.");
	}

	// Verify that invalid transfer batch lifecycle operations fail cleanly across geometry and texture resources.
	{
		VertexAttribute<V4Float32> zeroVertexAttribute(0, IVertexAttribute::PerformanceHint::WriteOften | IVertexAttribute::PerformanceHint::ReadNever, IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadOften);
		auto zeroVertexBuffer = renderer.CreateVertexBuffer();
		REQUIRE(!zeroVertexBuffer->BindVertexAttribute(zeroVertexAttribute));

		IndexAttribute<V1UInt16> zeroIndexAttribute(0, IIndexAttribute::PerformanceHint::WriteOften | IIndexAttribute::PerformanceHint::ReadNever, IIndexAttribute::PerformanceHint::WriteNever | IIndexAttribute::PerformanceHint::ReadOften);
		auto zeroIndexBuffer = renderer.CreateIndexBuffer();
		REQUIRE(!zeroIndexBuffer->BindIndexAttribute(zeroIndexAttribute));

		auto zeroTexture = renderer.CreateTextureBuffer2D();
		zeroTexture->SetTextureFormat(ITextureBuffer::ImageFormat::RGBA, ITextureBuffer::DataFormat::UNorm8);
		zeroTexture->SetTextureDimensions(V2UInt32(0, 1));
		REQUIRE(!zeroTexture->AllocateMemory());

		VertexAttribute<V4Float32> vertexAttribute(2, IVertexAttribute::PerformanceHint::WriteOften | IVertexAttribute::PerformanceHint::ReadNever, IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadOften);
		auto vertexBuffer = renderer.CreateVertexBuffer();
		REQUIRE(vertexBuffer->BindVertexAttribute(vertexAttribute));
		std::array<V4Float32, 2> initialValues = {V4Float32(0.0f, 0.0f, 0.0f, 0.0f), V4Float32(0.0f, 0.0f, 0.0f, 0.0f)};
		REQUIRE(vertexAttribute.SetInitialData(initialValues.data(), initialValues.size()));
		REQUIRE(vertexBuffer->AllocateMemory());

		IndexAttribute<V1UInt16> indexAttribute(3, IIndexAttribute::PerformanceHint::WriteOften | IIndexAttribute::PerformanceHint::ReadNever, IIndexAttribute::PerformanceHint::WriteNever | IIndexAttribute::PerformanceHint::ReadOften);
		auto indexBuffer = renderer.CreateIndexBuffer();
		REQUIRE(indexBuffer->BindIndexAttribute(indexAttribute));
		std::array<V1UInt16, 3> initialIndices = {V1UInt16(0), V1UInt16(1), V1UInt16(0)};
		REQUIRE(indexAttribute.SetInitialData(initialIndices.data(), initialIndices.size()));
		REQUIRE(indexBuffer->AllocateMemory());

		auto texture = renderer.CreateTextureBuffer2D();
		texture->SetUsageFlags(ITextureBuffer::UsageFlags::ShaderInput);
		texture->SetTextureFormat(ITextureBuffer::ImageFormat::RGBA, ITextureBuffer::DataFormat::UNorm8);
		texture->SetTextureDimensions(V2UInt32(2, 2));
		texture->SetPerformanceHints(ITextureBuffer::PerformanceHint::WriteOften | ITextureBuffer::PerformanceHint::ReadNever, ITextureBuffer::PerformanceHint::WriteNever | ITextureBuffer::PerformanceHint::ReadOften);
		std::vector<V4UInt8> initialTextureValues(4, V4UInt8(0, 0, 255, 255));
		REQUIRE(texture->SetInitialData(initialTextureValues));
		REQUIRE(texture->AllocateMemory());

		std::array<V4Float32, 2> values = {V4Float32(1.0f, 2.0f, 3.0f, 4.0f), V4Float32(5.0f, 6.0f, 7.0f, 8.0f)};
		std::array<V1UInt16, 3> updatedIndices = {V1UInt16(1), V1UInt16(0), V1UInt16(1)};
		std::vector<V4UInt8> updatedTextureValues(4, V4UInt8(255, 255, 0, 255));
		REQUIRE(!vertexAttribute.QueueDataUpdate(values.data(), values.size(), 1));
		REQUIRE(!indexAttribute.QueueDataUpdate(updatedIndices.data(), updatedIndices.size(), 1));
		REQUIRE(!texture->QueueDataUpdate(updatedTextureValues, 0, V2UInt32(1, 1), V2UInt32(2, 2)));

		auto completedTransferBatch = renderer.CreateTransferBatch(ITransferBatch::StartTiming::Immediately, ITransferBatch::EndTiming::AnyFrame);
		REQUIRE(completedTransferBatch->SubmitBatch());
		REQUIRE(completedTransferBatch->IsComplete());
		REQUIRE(!completedTransferBatch->SubmitBatch());
		REQUIRE(!vertexAttribute.QueueDataUpdate(values.data(), values.size(), 0, sizeof(V4Float32), completedTransferBatch.get()));
		REQUIRE(!indexAttribute.QueueDataUpdate(updatedIndices.data(), updatedIndices.size(), 0, sizeof(V1UInt16), completedTransferBatch.get()));
		REQUIRE(!texture->QueueDataUpdate(updatedTextureValues, 0, V2UInt32(0, 0), V2UInt32(2, 2), completedTransferBatch.get()));

		auto pendingTransferBatch = renderer.CreateTransferBatch(ITransferBatch::StartTiming::AfterCurrentFrame, ITransferBatch::EndTiming::AnyFrame);
		REQUIRE(vertexAttribute.QueueDataUpdate(values.data(), values.size(), 0, sizeof(V4Float32), pendingTransferBatch.get()));
		REQUIRE(indexAttribute.QueueDataUpdate(updatedIndices.data(), updatedIndices.size(), 0, sizeof(V1UInt16), pendingTransferBatch.get()));
		REQUIRE(texture->QueueDataUpdate(updatedTextureValues, 0, V2UInt32(0, 0), V2UInt32(2, 2), pendingTransferBatch.get()));
		REQUIRE(pendingTransferBatch->SubmitBatch());
		REQUIRE(!vertexAttribute.QueueDataUpdate(values.data(), values.size(), 0, sizeof(V4Float32), pendingTransferBatch.get()));
		REQUIRE(!indexAttribute.QueueDataUpdate(updatedIndices.data(), updatedIndices.size(), 0, sizeof(V1UInt16), pendingTransferBatch.get()));
		REQUIRE(!texture->QueueDataUpdate(updatedTextureValues, 0, V2UInt32(0, 0), V2UInt32(2, 2), pendingTransferBatch.get()));
		waitForBatchComplete(*pendingTransferBatch);
		session.AddTestSuccess("BatchFailureContracts", "Transfer batches rejected double submission, mutation after submit, operations after submit, zero-length geometry or texture allocations, and oversized geometry or texture updates.");
	}

	// Exercise transfer-batch texture updates across the non-2D texture families.
	{
		std::vector<V4UInt8> initial1DValues(4, V4UInt8(0, 0, 255, 255));
		std::vector<V4UInt8> updated1DValues(4, V4UInt8(255, 255, 0, 255));
		auto texture1D = renderer.CreateTextureBuffer1D();
		texture1D->SetUsageFlags(ITextureBuffer::UsageFlags::ShaderInput);
		texture1D->SetTextureFormat(ITextureBuffer::ImageFormat::RGBA, ITextureBuffer::DataFormat::UNorm8);
		texture1D->SetTextureDimensions(V1UInt32(4));
		texture1D->SetPerformanceHints(ITextureBuffer::PerformanceHint::WriteOften | ITextureBuffer::PerformanceHint::ReadNever, ITextureBuffer::PerformanceHint::WriteNever | ITextureBuffer::PerformanceHint::ReadOften);
		REQUIRE(texture1D->SetInitialData(initial1DValues));
		REQUIRE(texture1D->AllocateMemory());

		std::vector<V4UInt8> initial1DArrayValues(4, V4UInt8(0, 0, 255, 255));
		std::vector<V4UInt8> updated1DArrayValues(4, V4UInt8(255, 128, 0, 255));
		auto texture1DArray = renderer.CreateTextureBuffer1DArray();
		texture1DArray->SetUsageFlags(ITextureBuffer::UsageFlags::ShaderInput);
		texture1DArray->SetTextureFormat(ITextureBuffer::ImageFormat::RGBA, ITextureBuffer::DataFormat::UNorm8);
		texture1DArray->SetTextureDimensions(V1UInt32(4), 2);
		texture1DArray->SetPerformanceHints(ITextureBuffer::PerformanceHint::WriteOften | ITextureBuffer::PerformanceHint::ReadNever, ITextureBuffer::PerformanceHint::WriteNever | ITextureBuffer::PerformanceHint::ReadOften);
		REQUIRE(texture1DArray->SetInitialData(initial1DArrayValues, 0));
		REQUIRE(texture1DArray->SetInitialData(initial1DArrayValues, 1));
		REQUIRE(texture1DArray->AllocateMemory());

		std::vector<V4UInt8> initial2DArrayValues(4, V4UInt8(0, 0, 255, 255));
		std::vector<V4UInt8> updated2DArrayValues(4, V4UInt8(0, 255, 255, 255));
		auto texture2DArray = renderer.CreateTextureBuffer2DArray();
		texture2DArray->SetUsageFlags(ITextureBuffer::UsageFlags::ShaderInput);
		texture2DArray->SetTextureFormat(ITextureBuffer::ImageFormat::RGBA, ITextureBuffer::DataFormat::UNorm8);
		texture2DArray->SetTextureDimensions(V2UInt32(2, 2), 2);
		texture2DArray->SetPerformanceHints(ITextureBuffer::PerformanceHint::WriteOften | ITextureBuffer::PerformanceHint::ReadNever, ITextureBuffer::PerformanceHint::WriteNever | ITextureBuffer::PerformanceHint::ReadOften);
		REQUIRE(texture2DArray->SetInitialData(initial2DArrayValues, 0));
		REQUIRE(texture2DArray->SetInitialData(initial2DArrayValues, 1));
		REQUIRE(texture2DArray->AllocateMemory());

		std::vector<V4UInt8> initial3DValues(8, V4UInt8(0, 0, 255, 255));
		std::vector<V4UInt8> updated3DValues(8, V4UInt8(255, 0, 255, 255));
		auto texture3D = renderer.CreateTextureBuffer3D();
		texture3D->SetUsageFlags(ITextureBuffer::UsageFlags::ShaderInput);
		texture3D->SetTextureFormat(ITextureBuffer::ImageFormat::RGBA, ITextureBuffer::DataFormat::UNorm8);
		texture3D->SetTextureDimensions(V3UInt32(2, 2, 2));
		texture3D->SetPerformanceHints(ITextureBuffer::PerformanceHint::WriteOften | ITextureBuffer::PerformanceHint::ReadNever, ITextureBuffer::PerformanceHint::WriteNever | ITextureBuffer::PerformanceHint::ReadOften);
		REQUIRE(texture3D->SetInitialData(initial3DValues));
		REQUIRE(texture3D->AllocateMemory());

		std::vector<V4UInt8> initialCubeFaceValues(4, V4UInt8(0, 0, 255, 255));
		std::vector<V4UInt8> updatedCubeFaceValues(4, V4UInt8(0, 255, 0, 255));
		const std::array<ITextureBuffer::CubeMapFace, 6> cubeFaces = {
		  ITextureBuffer::CubeMapFace::PositiveX,
		  ITextureBuffer::CubeMapFace::NegativeX,
		  ITextureBuffer::CubeMapFace::PositiveY,
		  ITextureBuffer::CubeMapFace::NegativeY,
		  ITextureBuffer::CubeMapFace::PositiveZ,
		  ITextureBuffer::CubeMapFace::NegativeZ};
		auto textureCube = renderer.CreateTextureBufferCube();
		textureCube->SetUsageFlags(ITextureBuffer::UsageFlags::ShaderInput);
		textureCube->SetTextureFormat(ITextureBuffer::ImageFormat::RGBA, ITextureBuffer::DataFormat::UNorm8);
		textureCube->SetTextureDimensions(2);
		textureCube->SetPerformanceHints(ITextureBuffer::PerformanceHint::WriteOften | ITextureBuffer::PerformanceHint::ReadNever, ITextureBuffer::PerformanceHint::WriteNever | ITextureBuffer::PerformanceHint::ReadOften);
		for (auto cubeFace : cubeFaces)
		{
			REQUIRE(textureCube->SetInitialData(initialCubeFaceValues, cubeFace));
		}
		REQUIRE(textureCube->AllocateMemory());

		ITextureBufferCubeArray::unique_ptr textureCubeArray;
		if (session.Device().IsFeatureSupported(IGraphicsDevice::Feature::TextureCubeArray))
		{
			textureCubeArray = renderer.CreateTextureBufferCubeArray();
			textureCubeArray->SetUsageFlags(ITextureBuffer::UsageFlags::ShaderInput);
			textureCubeArray->SetTextureFormat(ITextureBuffer::ImageFormat::RGBA, ITextureBuffer::DataFormat::UNorm8);
			textureCubeArray->SetTextureDimensions(2, 2);
			textureCubeArray->SetPerformanceHints(ITextureBuffer::PerformanceHint::WriteOften | ITextureBuffer::PerformanceHint::ReadNever, ITextureBuffer::PerformanceHint::WriteNever | ITextureBuffer::PerformanceHint::ReadOften);
			for (size_t arrayIndex = 0; arrayIndex < 2; ++arrayIndex)
			{
				for (auto cubeFace : cubeFaces)
				{
					REQUIRE(textureCubeArray->SetInitialData(initialCubeFaceValues, cubeFace, arrayIndex));
				}
			}
			REQUIRE(textureCubeArray->AllocateMemory());
		}
		else
		{
			session.AddTestSkipped("TextureCubeArrayBatchLifecycle", "Cube-array texture transfer-batch coverage was skipped, as the current renderer doesn't support cube-array textures on this device.");
		}

		// Oversized regions should fail before a batch is involved.
		REQUIRE(!texture1D->QueueDataUpdate(updated1DValues, 0, V1UInt32(3), V1UInt32(2)));
		REQUIRE(!texture1DArray->QueueDataUpdate(updated1DArrayValues, (size_t)1, 0, V1UInt32(3), V1UInt32(2)));
		REQUIRE(!texture2DArray->QueueDataUpdate(updated2DArrayValues, (size_t)1, 0, V2UInt32(1, 1), V2UInt32(2, 2)));
		REQUIRE(!texture3D->QueueDataUpdate(updated3DValues, 0, V3UInt32(1, 1, 1), V3UInt32(2, 2, 2)));
		REQUIRE(!textureCube->QueueDataUpdate(updatedCubeFaceValues, ITextureBuffer::CubeMapFace::PositiveZ, 0, V2UInt32(1, 1), V2UInt32(2, 2)));
		if (textureCubeArray)
		{
			REQUIRE(!textureCubeArray->QueueDataUpdate(updatedCubeFaceValues, ITextureBuffer::CubeMapFace::PositiveZ, (size_t)1, 0, V2UInt32(1, 1), V2UInt32(2, 2)));
		}

		// Queue valid full-resource updates through a single transfer batch.
		auto textureFamilyBatch = renderer.CreateTransferBatch(ITransferBatch::StartTiming::AfterCurrentFrame, ITransferBatch::EndTiming::AnyFrame);
		REQUIRE(texture1D->QueueDataUpdate(updated1DValues, 0, V1UInt32(0), V1UInt32(0), textureFamilyBatch.get()));
		REQUIRE(texture1DArray->QueueDataUpdate(updated1DArrayValues, (size_t)1, 0, V1UInt32(0), V1UInt32(0), textureFamilyBatch.get()));
		REQUIRE(texture2DArray->QueueDataUpdate(updated2DArrayValues, (size_t)1, 0, V2UInt32(0, 0), V2UInt32(0, 0), textureFamilyBatch.get()));
		REQUIRE(texture3D->QueueDataUpdate(updated3DValues, 0, V3UInt32(0, 0, 0), V3UInt32(0, 0, 0), textureFamilyBatch.get()));
		REQUIRE(textureCube->QueueDataUpdate(updatedCubeFaceValues, ITextureBuffer::CubeMapFace::PositiveZ, 0, V2UInt32(0, 0), V2UInt32(0, 0), textureFamilyBatch.get()));
		if (textureCubeArray)
		{
			REQUIRE(textureCubeArray->QueueDataUpdate(updatedCubeFaceValues, ITextureBuffer::CubeMapFace::PositiveZ, (size_t)1, 0, V2UInt32(0, 0), V2UInt32(0, 0), textureFamilyBatch.get()));
		}
		REQUIRE(textureFamilyBatch->SubmitBatch());

		// Once submitted, the same texture resources must reject further operations against that batch.
		REQUIRE(!texture1D->QueueDataUpdate(updated1DValues, 0, V1UInt32(0), V1UInt32(0), textureFamilyBatch.get()));
		REQUIRE(!texture1DArray->QueueDataUpdate(updated1DArrayValues, (size_t)1, 0, V1UInt32(0), V1UInt32(0), textureFamilyBatch.get()));
		REQUIRE(!texture2DArray->QueueDataUpdate(updated2DArrayValues, (size_t)1, 0, V2UInt32(0, 0), V2UInt32(0, 0), textureFamilyBatch.get()));
		REQUIRE(!texture3D->QueueDataUpdate(updated3DValues, 0, V3UInt32(0, 0, 0), V3UInt32(0, 0, 0), textureFamilyBatch.get()));
		REQUIRE(!textureCube->QueueDataUpdate(updatedCubeFaceValues, ITextureBuffer::CubeMapFace::PositiveZ, 0, V2UInt32(0, 0), V2UInt32(0, 0), textureFamilyBatch.get()));
		if (textureCubeArray)
		{
			REQUIRE(!textureCubeArray->QueueDataUpdate(updatedCubeFaceValues, ITextureBuffer::CubeMapFace::PositiveZ, (size_t)1, 0, V2UInt32(0, 0), V2UInt32(0, 0), textureFamilyBatch.get()));
		}
		waitForBatchComplete(*textureFamilyBatch);
		session.AddTestSuccess("TextureFamilyBatchLifecycle", "1D, 1D-array, 2D-array, 3D, cubemap, and supported cube-array textures accepted valid transfer-batch updates and rejected oversized regions and operations after submit.");
	}

	// Resource arrays are optional, so only run their transfer-batch and boundary checks where the device supports them.
	if (!session.Device().IsFeatureSupported(IGraphicsDevice::Feature::ResourceArrays))
	{
		session.AddTestSkipped("ResourceArrayBatchFailureContracts", "This part of the test was skipped, as the current renderer doesn't support resource arrays on this device.");
	}
	else
	{
		std::array<V4Float32, 2> initialValues = {V4Float32(1.0f, 0.0f, 0.0f, 1.0f), V4Float32(0.0f, 1.0f, 0.0f, 1.0f)};
		std::array<V4Float32, 2> updatedValues = {V4Float32(0.0f, 0.0f, 1.0f, 1.0f), V4Float32(1.0f, 1.0f, 0.0f, 1.0f)};
		std::array<V4Float32, 3> oversizedValues = {V4Float32(1.0f, 0.0f, 0.0f, 1.0f), V4Float32(0.0f, 1.0f, 0.0f, 1.0f), V4Float32(0.0f, 0.0f, 1.0f, 1.0f)};

		auto zeroDataArray = renderer.CreateDataArray();
		zeroDataArray->SetBufferLayout(sizeof(V4Float32), 0);
		REQUIRE(!zeroDataArray->AllocateMemory());
		auto zeroTexelArray = renderer.CreateTexelArray();
		zeroTexelArray->SetBufferLayout(ITexelArray::ImageFormat::RGBA, ITexelArray::DataFormat::Float32, 0);
		REQUIRE(!zeroTexelArray->AllocateMemory());

		auto dataArray = renderer.CreateDataArray();
		dataArray->SetUsageFlags(IDataArray::UsageFlags::TransferSource | IDataArray::UsageFlags::TransferDestination);
		dataArray->SetBufferLayout(sizeof(V4Float32), initialValues.size());
		REQUIRE(dataArray->SetInitialData(initialValues.data(), initialValues.size() * sizeof(V4Float32)));
		REQUIRE(dataArray->AllocateMemory());
		auto targetDataArray = renderer.CreateDataArray();
		targetDataArray->SetUsageFlags(IDataArray::UsageFlags::TransferDestination);
		targetDataArray->SetBufferLayout(sizeof(V4Float32), initialValues.size());
		REQUIRE(targetDataArray->SetInitialData(initialValues.data(), initialValues.size() * sizeof(V4Float32)));
		REQUIRE(targetDataArray->AllocateMemory());

		auto texelArray = renderer.CreateTexelArray();
		texelArray->SetUsageFlags(ITexelArray::UsageFlags::TransferSource | ITexelArray::UsageFlags::TransferDestination);
		texelArray->SetBufferLayout(ITexelArray::ImageFormat::RGBA, ITexelArray::DataFormat::Float32, initialValues.size());
		REQUIRE(texelArray->SetInitialData(initialValues.data(), initialValues.size()));
		REQUIRE(texelArray->AllocateMemory());
		auto targetTexelArray = renderer.CreateTexelArray();
		targetTexelArray->SetUsageFlags(ITexelArray::UsageFlags::TransferDestination);
		targetTexelArray->SetBufferLayout(ITexelArray::ImageFormat::RGBA, ITexelArray::DataFormat::Float32, initialValues.size());
		REQUIRE(targetTexelArray->SetInitialData(initialValues.data(), initialValues.size()));
		REQUIRE(targetTexelArray->AllocateMemory());
		auto mismatchedTexelArray = renderer.CreateTexelArray();
		mismatchedTexelArray->SetUsageFlags(ITexelArray::UsageFlags::TransferDestination);
		mismatchedTexelArray->SetBufferLayout(ITexelArray::ImageFormat::RG, ITexelArray::DataFormat::Float32, initialValues.size());
		std::array<V2Float32, 2> mismatchedInitialValues = {V2Float32(1.0f, 0.0f), V2Float32(0.0f, 1.0f)};
		REQUIRE(mismatchedTexelArray->SetInitialData(mismatchedInitialValues.data(), mismatchedInitialValues.size()));
		REQUIRE(mismatchedTexelArray->AllocateMemory());

		REQUIRE(!dataArray->QueueDataUpdate(oversizedValues.data(), oversizedValues.size() * sizeof(V4Float32)));
		REQUIRE(!texelArray->QueueDataUpdate(oversizedValues.data(), oversizedValues.size()));
		REQUIRE(!texelArray->QueueDataTransfer(mismatchedTexelArray.get(), 1));

		auto completedTransferBatch = renderer.CreateTransferBatch(ITransferBatch::StartTiming::Immediately, ITransferBatch::EndTiming::AnyFrame);
		REQUIRE(completedTransferBatch->SubmitBatch());
		REQUIRE(completedTransferBatch->IsComplete());
		REQUIRE(!dataArray->QueueDataUpdate(updatedValues.data(), updatedValues.size() * sizeof(V4Float32), 0, completedTransferBatch.get()));
		REQUIRE(!dataArray->QueueDataTransfer(targetDataArray.get(), updatedValues.size() * sizeof(V4Float32), 0, 0, completedTransferBatch.get()));
		REQUIRE(!texelArray->QueueDataUpdate(updatedValues.data(), updatedValues.size(), 0, completedTransferBatch.get()));
		REQUIRE(!texelArray->QueueDataTransfer(targetTexelArray.get(), updatedValues.size(), 0, 0, completedTransferBatch.get()));

		auto pendingTransferBatch = renderer.CreateTransferBatch(ITransferBatch::StartTiming::AfterCurrentFrame, ITransferBatch::EndTiming::AnyFrame);
		REQUIRE(dataArray->QueueDataUpdate(updatedValues.data(), updatedValues.size() * sizeof(V4Float32), 0, pendingTransferBatch.get()));
		REQUIRE(dataArray->QueueDataTransfer(targetDataArray.get(), updatedValues.size() * sizeof(V4Float32), 0, 0, pendingTransferBatch.get()));
		REQUIRE(texelArray->QueueDataUpdate(updatedValues.data(), updatedValues.size(), 0, pendingTransferBatch.get()));
		REQUIRE(texelArray->QueueDataTransfer(targetTexelArray.get(), updatedValues.size(), 0, 0, pendingTransferBatch.get()));
		REQUIRE(pendingTransferBatch->SubmitBatch());
		REQUIRE(!dataArray->QueueDataUpdate(updatedValues.data(), updatedValues.size() * sizeof(V4Float32), 0, pendingTransferBatch.get()));
		REQUIRE(!dataArray->QueueDataTransfer(targetDataArray.get(), updatedValues.size() * sizeof(V4Float32), 0, 0, pendingTransferBatch.get()));
		REQUIRE(!texelArray->QueueDataUpdate(updatedValues.data(), updatedValues.size(), 0, pendingTransferBatch.get()));
		REQUIRE(!texelArray->QueueDataTransfer(targetTexelArray.get(), updatedValues.size(), 0, 0, pendingTransferBatch.get()));
		waitForBatchComplete(*pendingTransferBatch);
		session.AddTestSuccess("ResourceArrayBatchFailureContracts", "Data arrays and texel arrays rejected zero-length allocations, oversized updates, incompatible texel-array transfers, operations after submit, and mutation after submit.");
	}
	return true;
}

} // namespace cobalt::graphics::testing
