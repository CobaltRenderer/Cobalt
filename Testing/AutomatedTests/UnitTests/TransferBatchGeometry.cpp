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
};

struct VSOutput {
    float4 position : SV_POSITION;
};

VSOutput main(VSInput IN)
{
    VSOutput OUT;

    OUT.position = float4(IN.position, 1.0f);

    return OUT;
}
)";
const std::string FragmentShader = R"(
float4 main() : SV_TARGET0
{
    return float4(0.0f, 1.0f, 0.0f, 1.0f);
}
)";

} // namespace

DEFINE_UNIT_TEST_WITH_BASE("Resources/Batching/GeometryTransferBatch", UnitTestBase)
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

	// Define our geometry states
	std::vector<V3Float32> leftQuadPositions = {
	  {-0.95f, 0.75f, 0.0f},
	  {-0.05f, 0.75f, 0.0f},
	  {-0.95f, -0.75f, 0.0f},
	  {-0.05f, -0.75f, 0.0f},
	};
	std::vector<V3Float32> rightQuadPositions = {
	  {0.05f, 0.75f, 0.0f},
	  {0.95f, 0.75f, 0.0f},
	  {0.05f, -0.75f, 0.0f},
	  {0.95f, -0.75f, 0.0f},
	};
	std::vector<V1UInt32> visibleIndices = {{0}, {1}, {2}, {2}, {1}, {3}};
	std::vector<V1UInt32> degenerateIndices = {{0}, {0}, {0}, {0}, {0}, {0}};

	VertexAttribute<V3Float32> vertexAttributePosition(leftQuadPositions.size(), IVertexAttribute::PerformanceHint::WriteOften | IVertexAttribute::PerformanceHint::ReadNever, IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadOften);
	auto vertexBuffer = renderer.CreateVertexBuffer();
	REQUIRE(vertexBuffer->BindVertexAttribute(vertexAttributePosition));
	REQUIRE(vertexAttributePosition.SetInitialData(leftQuadPositions));
	REQUIRE(vertexBuffer->AllocateMemory());

	IndexAttribute<V1UInt32> indexAttribute(visibleIndices.size(), IIndexAttribute::PerformanceHint::WriteOften | IIndexAttribute::PerformanceHint::ReadNever, IIndexAttribute::PerformanceHint::WriteNever | IIndexAttribute::PerformanceHint::ReadOften);
	auto indexBuffer = renderer.CreateIndexBuffer();
	REQUIRE(indexBuffer->BindIndexAttribute(indexAttribute));
	REQUIRE(indexAttribute.SetInitialData(visibleIndices));
	REQUIRE(indexBuffer->AllocateMemory());

	// Create our renderable node
	auto renderableNode = renderer.CreateRenderableNode();
	REQUIRE(renderableNode->BindVertexAttribute(vertexAttributePosition, shaderProgram->GetVertexAttributeId("position")));
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

		// Start from a visible quad on the left for this configuration
		REQUIRE(vertexAttributePosition.QueueDataUpdate(leftQuadPositions.data(), leftQuadPositions.size()));
		REQUIRE(indexAttribute.QueueDataUpdate(visibleIndices.data(), visibleIndices.size()));
		auto frameBufferCapture = renderer.CreateFrameBufferOutput();
		frameBufferCapture->SetDetachAfterCapture(true);
		mainWindowFrameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
		DrawOneFrame();
		session.AddTestImageResult("Transfer" + configName + "Baseline", "A green quad rendered on the left side of the image.", std::move(frameBufferCapture), IImageDiff::Algorithm::AllOfThem, 0.95);

		// Queue a vertex update to move the rendered quad across the screen
		auto vertexTransferBatch = renderer.CreateTransferBatch(config.startTiming, config.endTiming);
		REQUIRE(vertexAttributePosition.QueueDataUpdate(rightQuadPositions.data(), rightQuadPositions.size(), 0, sizeof(V3Float32), vertexTransferBatch.get()));
		REQUIRE(!vertexTransferBatch->IsComplete());

		frameBufferCapture = renderer.CreateFrameBufferOutput();
		frameBufferCapture->SetDetachAfterCapture(true);
		mainWindowFrameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
		DrawOneFrame();
		session.AddTestImageResult("Transfer" + configName + "VertexDeferred", "The batched vertex update has not been submitted yet, so the green quad should remain on the left.", std::move(frameBufferCapture), IImageDiff::Algorithm::AllOfThem, 0.95);

		// Submit the batch
		REQUIRE(vertexTransferBatch->SubmitBatch());

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
			vertexTransferBatch->WaitForComplete();
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
		session.AddTestImageResult("Transfer" + configName + "VertexSubmitted", "The batched vertex update has completed, so the green quad should move to the right.", std::move(frameBufferCapture), IImageDiff::Algorithm::AllOfThem, 0.95);
		REQUIRE(vertexTransferBatch->IsComplete());

		// Restore the visible left-side geometry and test an index update in a separate batch.
		REQUIRE(vertexAttributePosition.QueueDataUpdate(leftQuadPositions.data(), leftQuadPositions.size()));
		REQUIRE(indexAttribute.QueueDataUpdate(visibleIndices.data(), visibleIndices.size()));
		frameBufferCapture = renderer.CreateFrameBufferOutput();
		frameBufferCapture->SetDetachAfterCapture(true);
		mainWindowFrameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
		DrawOneFrame();
		session.AddTestImageResult("Transfer" + configName + "IndexBaseline", "A green quad rendered on the left side of the image.", std::move(frameBufferCapture), IImageDiff::Algorithm::AllOfThem, 0.95);

		auto indexTransferBatch = renderer.CreateTransferBatch(config.startTiming, config.endTiming);
		REQUIRE(indexAttribute.QueueDataUpdate(degenerateIndices.data(), degenerateIndices.size(), 0, sizeof(V1UInt32), indexTransferBatch.get()));
		REQUIRE(!indexTransferBatch->IsComplete());

		frameBufferCapture = renderer.CreateFrameBufferOutput();
		frameBufferCapture->SetDetachAfterCapture(true);
		mainWindowFrameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
		DrawOneFrame();
		session.AddTestImageResult("Transfer" + configName + "IndexDeferred", "The batched index update has not been submitted yet, so the green quad should remain visible.", std::move(frameBufferCapture), IImageDiff::Algorithm::AllOfThem, 0.95);

		// Submit the batch
		REQUIRE(indexTransferBatch->SubmitBatch());

		// Keep drawing frames until a waiting thread reports completion
		waitThreadStarted = false;
		transferComplete = false;
		std::thread indexWaitThread([&]() {
			std::unique_lock<std::mutex> lock(transferMutex);
			waitThreadStarted = true;
			transferCondition.notify_all();
			lock.unlock();
			indexTransferBatch->WaitForComplete();
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
		indexWaitThread.join();

		// Capture one full frame after the transfer has completed to verify the final result
		frameBufferCapture = renderer.CreateFrameBufferOutput();
		frameBufferCapture->SetDetachAfterCapture(true);
		mainWindowFrameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
		DrawOneFrame();
		session.AddTestImageResult("Transfer" + configName + "IndexSubmitted", "The submitted index update should degenerate the geometry so the image becomes black.", std::move(frameBufferCapture), IImageDiff::Algorithm::AllOfThem, 0.95);
		REQUIRE(indexTransferBatch->IsComplete());
	}

	// Remove all our defined render passes
	renderer.RemoveAllRenderPasses();
	return true;
}

} // namespace cobalt::graphics::testing
