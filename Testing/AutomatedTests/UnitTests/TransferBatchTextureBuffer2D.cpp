// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "../UnitTestBase.h"
#include <array>
#include <condition_variable>
#include <mutex>
#include <thread>
namespace cobalt::graphics::testing {
using namespace cobalt::graphics;

namespace {
const std::string VertexShader = R"(
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
const std::string FragmentShader = R"(
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

} // namespace

DEFINE_UNIT_TEST_WITH_BASE("Resources/Batching/TextureBuffer2DTransferBatch", UnitTestBase)
{
	// Retrieve our important objects
	auto& renderer = session.Renderer();
	auto& uiThread = session.UIThread();
	auto& testWindowInfo = *session.TestWindowPlatformInfo();
	auto windowSize = session.TestWindowSize();

	// Define the framebuffer
	auto mainWindowFrameBuffer = renderer.CreateFrameBuffer();
	mainWindowFrameBuffer->DefineViewportRegion(V2UInt32(0, 0), windowSize);
	REQUIRE(uiThread.InvokeSync([&] { return mainWindowFrameBuffer->BindWindow(testWindowInfo, IFrameBuffer::WindowDepthStencilMode::DepthUNorm24); }));

	// Create and compile our shader program
	auto shaderProgram = renderer.CreateShaderProgram();
	REQUIRE(shaderProgram->LoadShaderStage(IShaderProgram::ShaderStage::Vertex, ShaderSourceInfoHLSL(VertexShader)));
	REQUIRE(shaderProgram->LoadShaderStage(IShaderProgram::ShaderStage::Fragment, ShaderSourceInfoHLSL(FragmentShader)));
	REQUIRE(shaderProgram->CompileProgram());

	// Create our fullscreen quad geometry
	std::vector<V3Float32> positionVertexData = {
	  {-1.0f, 1.0f, 0.0f},
	  {1.0f, 1.0f, 0.0f},
	  {-1.0f, -1.0f, 0.0f},
	  {1.0f, -1.0f, 0.0f},
	};
	std::vector<V2Float32> texCoordVertexData = {
	  {0.0f, 1.0f},
	  {1.0f, 1.0f},
	  {0.0f, 0.0f},
	  {1.0f, 0.0f},
	};
	std::vector<V1UInt32> indexData = {{0}, {1}, {2}, {2}, {1}, {3}};

	VertexAttribute<V3Float32> vertexAttributePosition(positionVertexData.size(), IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadNever, IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadOften);
	VertexAttribute<V2Float32> vertexAttributeTexCoord(texCoordVertexData.size(), IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadNever, IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadOften);
	auto vertexBuffer = renderer.CreateVertexBuffer();
	REQUIRE(vertexBuffer->BindVertexAttribute(vertexAttributePosition));
	REQUIRE(vertexAttributePosition.SetInitialData(positionVertexData));
	REQUIRE(vertexBuffer->BindVertexAttribute(vertexAttributeTexCoord));
	REQUIRE(vertexAttributeTexCoord.SetInitialData(texCoordVertexData));
	REQUIRE(vertexBuffer->AllocateMemory());

	IndexAttribute<V1UInt32> indexAttribute(indexData.size(), IIndexAttribute::PerformanceHint::WriteNever | IIndexAttribute::PerformanceHint::ReadNever, IIndexAttribute::PerformanceHint::WriteNever | IIndexAttribute::PerformanceHint::ReadOften);
	auto indexBuffer = renderer.CreateIndexBuffer();
	REQUIRE(indexBuffer->BindIndexAttribute(indexAttribute));
	REQUIRE(indexAttribute.SetInitialData(indexData));
	REQUIRE(indexBuffer->AllocateMemory());

	// Create our texture
	std::vector<V4UInt8> baselineTexture(4, V4UInt8(0, 0, 255, 255));
	std::vector<V4UInt8> fullImageUpdate(4, V4UInt8(255, 255, 0, 255));
	std::vector<V4UInt8> leftImageUpdate(2, V4UInt8(255, 0, 0, 255));
	std::vector<V4UInt8> rightImageUpdate(2, V4UInt8(0, 255, 0, 255));

	auto texture = renderer.CreateTextureBuffer2D();
	texture->SetTextureDimensions(V2UInt32(4, 1));
	texture->SetTextureFormat(ITextureBuffer::ImageFormat::RGBA, ITextureBuffer::DataFormat::UNorm8);
	texture->SetUsageFlags(ITextureBuffer::UsageFlags::ShaderInput);
	texture->SetPerformanceHints(ITextureBuffer::PerformanceHint::WriteOften | ITextureBuffer::PerformanceHint::ReadNever, ITextureBuffer::PerformanceHint::WriteNever | ITextureBuffer::PerformanceHint::ReadOften);
	REQUIRE(texture->SetInitialData(baselineTexture));
	REQUIRE(texture->AllocateMemory());

	auto sampler = renderer.CreateTextureSampler2D();
	sampler->SetTextureFilterMode(ITextureSampler::FilterMode::Nearest, ITextureSampler::FilterMode::Nearest);
	sampler->SetTextureWrapMode(ITextureSampler::WrapMode::ClampToEdge, ITextureSampler::WrapMode::ClampToEdge);

	// Create our renderable node
	auto positionAttributeId = shaderProgram->GetVertexAttributeId("position");
	auto texCoordAttributeId = shaderProgram->GetVertexAttributeId("texCoord");
	auto renderableNode = renderer.CreateRenderableNode();
	REQUIRE(renderableNode->BindVertexAttribute(vertexAttributePosition, positionAttributeId));
	REQUIRE(renderableNode->BindVertexAttribute(vertexAttributeTexCoord, texCoordAttributeId));
	REQUIRE(renderableNode->BindIndexAttribute(indexAttribute));
	REQUIRE(renderableNode->SetPrimitiveMode(IRenderableNode::PrimitiveMode::Triangles));
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

	struct TransferBatchConfig
	{
		ITransferBatch::StartTiming startTiming;
		ITransferBatch::EndTiming endTiming;
	};

	const std::array<TransferBatchConfig, 4> transferBatchConfigs = {{
	  {ITransferBatch::StartTiming::AfterCurrentFrame, ITransferBatch::EndTiming::BeforeNextFrame},
	  {ITransferBatch::StartTiming::AfterCurrentFrame, ITransferBatch::EndTiming::AnyFrame},
	  {ITransferBatch::StartTiming::Immediately, ITransferBatch::EndTiming::BeforeNextFrame},
	  {ITransferBatch::StartTiming::Immediately, ITransferBatch::EndTiming::AnyFrame},
	}};

	for (const auto& config : transferBatchConfigs)
	{
		std::string configName;
		if (config.startTiming == ITransferBatch::StartTiming::AfterCurrentFrame)
		{
			configName = (config.endTiming == ITransferBatch::EndTiming::BeforeNextFrame) ? "AfterCurrentFrameBeforeNextFrame" : "AfterCurrentFrameBeforeAnyFrame";
		}
		else
		{
			configName = (config.endTiming == ITransferBatch::EndTiming::BeforeNextFrame) ? "ImmediatelyBeforeNextFrame" : "ImmediatelyBeforeAnyFrame";
		}

		// Test a full-image update through a transfer batch
		REQUIRE(texture->QueueDataUpdate(baselineTexture));
		auto frameBufferCapture = renderer.CreateFrameBufferOutput();
		frameBufferCapture->SetDetachAfterCapture(true);
		mainWindowFrameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
		DrawOneFrame();
		session.AddTestImageResult("Transfer" + configName + "FullBaseline", "A fullscreen blue image sampled from a 4x1 texture.", std::move(frameBufferCapture), IImageDiff::Algorithm::AllOfThem, 0.95);

		auto fullImageBatch = renderer.CreateTransferBatch(config.startTiming, config.endTiming);
		REQUIRE(texture->QueueDataUpdate(fullImageUpdate, 0, V2UInt32(0, 0), V2UInt32(0, 0), fullImageBatch.get()));
		REQUIRE(!fullImageBatch->IsComplete());

		frameBufferCapture = renderer.CreateFrameBufferOutput();
		frameBufferCapture->SetDetachAfterCapture(true);
		mainWindowFrameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
		DrawOneFrame();
		session.AddTestImageResult("Transfer" + configName + "FullDeferred", "The full-image texture update has not been submitted yet, so the image should remain blue.", std::move(frameBufferCapture), IImageDiff::Algorithm::AllOfThem, 0.95);

		// Submit the batch
		REQUIRE(fullImageBatch->SubmitBatch());

		// Keep drawing frames until a waiting thread reports completion
		std::mutex transferMutex;
		std::condition_variable transferCondition;
		std::atomic<bool> waitThreadStarted = false;
		std::atomic<bool> transferComplete = false;
		std::thread waitThread([&]() {
			std::unique_lock<std::mutex> lock(transferMutex);
			waitThreadStarted = true;
			transferCondition.notify_all();
			lock.unlock();
			fullImageBatch->WaitForComplete();
			lock.lock();
			transferComplete = true;
			transferCondition.notify_all();
		});
		{
			std::unique_lock<std::mutex> lock(transferMutex);
			while (!waitThreadStarted)
			{
				transferCondition.wait(lock);
			}
		}
		while (!transferComplete)
		{
			DrawOneFrame();
		}
		waitThread.join();

		// Capture one full frame after the transfer has completed to verify the final result
		frameBufferCapture = renderer.CreateFrameBufferOutput();
		frameBufferCapture->SetDetachAfterCapture(true);
		mainWindowFrameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
		DrawOneFrame();
		session.AddTestImageResult("Transfer" + configName + "FullSubmitted", "The full-image texture update has completed, so the image should be yellow.", std::move(frameBufferCapture), IImageDiff::Algorithm::AllOfThem, 0.95);
		REQUIRE(fullImageBatch->IsComplete());

		// If we're not starting immediately, buffer contents can be preserved, so perform partial buffer update tests.
		if (config.startTiming != ITransferBatch::StartTiming::Immediately)
		{
			// Test partial region updates through a transfer batch
			REQUIRE(texture->QueueDataUpdate(baselineTexture));
			frameBufferCapture = renderer.CreateFrameBufferOutput();
			frameBufferCapture->SetDetachAfterCapture(true);
			mainWindowFrameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
			DrawOneFrame();
			session.AddTestImageResult("Transfer" + configName + "PartialBaseline", "A fullscreen blue image sampled from a 4x1 texture.", std::move(frameBufferCapture), IImageDiff::Algorithm::AllOfThem, 0.95);

			auto partialImageBatch = renderer.CreateTransferBatch(config.startTiming, config.endTiming);
			REQUIRE(texture->QueueDataUpdate(leftImageUpdate, 0, V2UInt32(0, 0), V2UInt32(2, 1), partialImageBatch.get()));
			REQUIRE(texture->QueueDataUpdate(rightImageUpdate, 0, V2UInt32(2, 0), V2UInt32(2, 1), partialImageBatch.get()));
			REQUIRE(!partialImageBatch->IsComplete());

			frameBufferCapture = renderer.CreateFrameBufferOutput();
			frameBufferCapture->SetDetachAfterCapture(true);
			mainWindowFrameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
			DrawOneFrame();
			session.AddTestImageResult("Transfer" + configName + "PartialDeferred", "The partial texture updates have not been submitted yet, so the image should remain blue.", std::move(frameBufferCapture), IImageDiff::Algorithm::AllOfThem, 0.95);

			// Submit the batch
			REQUIRE(partialImageBatch->SubmitBatch());

			// Keep drawing frames until a waiting thread reports completion
			waitThreadStarted = false;
			transferComplete = false;
			std::thread partialWaitThread([&]() {
				std::unique_lock<std::mutex> lock(transferMutex);
				waitThreadStarted = true;
				transferCondition.notify_all();
				lock.unlock();
				partialImageBatch->WaitForComplete();
				lock.lock();
				transferComplete = true;
				transferCondition.notify_all();
			});
			{
				std::unique_lock<std::mutex> lock(transferMutex);
				while (!waitThreadStarted)
				{
					transferCondition.wait(lock);
				}
			}
			while (!transferComplete)
			{
				DrawOneFrame();
			}
			partialWaitThread.join();

			// Capture one full frame after the transfer has completed to verify the final result
			frameBufferCapture = renderer.CreateFrameBufferOutput();
			frameBufferCapture->SetDetachAfterCapture(true);
			mainWindowFrameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
			DrawOneFrame();
			session.AddTestImageResult("Transfer" + configName + "PartialSubmitted", "The submitted partial texture updates should produce a red left half and green right half.", std::move(frameBufferCapture), IImageDiff::Algorithm::AllOfThem, 0.95);
			REQUIRE(partialImageBatch->IsComplete());
		}
	}

	// Remove all our defined render passes
	renderer.RemoveAllRenderPasses();
	return true;
}

} // namespace cobalt::graphics::testing
