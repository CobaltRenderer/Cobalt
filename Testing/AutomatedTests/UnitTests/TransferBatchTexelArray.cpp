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
Buffer<float4> colorData;

struct VSOutput {
    float4 position : SV_POSITION;
    float2 texCoord : texCoord;
};

float4 main(VSOutput IN) : SV_TARGET0
{
    uint colorIndex = min((uint)(IN.texCoord.x * 3.0f), 2u);
    return colorData[colorIndex];
}
)";

} // namespace

DEFINE_UNIT_TEST_WITH_BASE("Resources/Batching/TexelArrayTransferBatch", UnitTestBase)
{
	// Ensure texel arrays are supported by the current renderer
	if (!session.Device().IsFeatureSupported(IGraphicsDevice::Feature::ResourceArrays))
	{
		session.AddTestSkipped("Skipped test", "This test was skipped, as the current renderer doesn't support texel arrays on this device.");
		return true;
	}

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

	// Create our texel arrays
	std::vector<V4Float32> baseColors = {
	  {1.0f, 0.0f, 0.0f, 1.0f},
	  {0.0f, 1.0f, 0.0f, 1.0f},
	  {0.0f, 0.0f, 1.0f, 1.0f},
	};
	V4Float32 updatedMiddleColor(1.0f, 0.0f, 1.0f, 1.0f);
	std::vector<V4Float32> transferColors = {
	  {0.0f, 1.0f, 1.0f, 1.0f},
	  {1.0f, 1.0f, 0.0f, 1.0f},
	};

	auto colorArray = renderer.CreateTexelArray();
	colorArray->SetBufferLayout(ITexelArray::ImageFormat::RGBA, ITexelArray::DataFormat::Float32, baseColors.size());
	colorArray->SetUsageFlags(ITexelArray::UsageFlags::ShaderInput | ITexelArray::UsageFlags::TransferDestination);
	REQUIRE(colorArray->SetInitialData(baseColors));
	REQUIRE(colorArray->AllocateMemory());

	auto transferArray = renderer.CreateTexelArray();
	transferArray->SetBufferLayout(ITexelArray::ImageFormat::RGBA, ITexelArray::DataFormat::Float32, transferColors.size());
	transferArray->SetUsageFlags(ITexelArray::UsageFlags::TransferSource);
	REQUIRE(transferArray->SetInitialData(transferColors));
	REQUIRE(transferArray->AllocateMemory());

	// Retrieve our shader IDs
	auto positionAttributeId = shaderProgram->GetVertexAttributeId("position");
	auto texCoordAttributeId = shaderProgram->GetVertexAttributeId("texCoord");
	auto colorDataId = shaderProgram->GetResourceArrayId("colorData");

	// Create our renderable node
	auto renderableNode = renderer.CreateRenderableNode();
	REQUIRE(renderableNode->BindVertexAttribute(vertexAttributePosition, positionAttributeId));
	REQUIRE(renderableNode->BindVertexAttribute(vertexAttributeTexCoord, texCoordAttributeId));
	REQUIRE(renderableNode->BindIndexAttribute(indexAttribute));
	REQUIRE(renderableNode->SetPrimitiveMode(IRenderableNode::PrimitiveMode::Triangles));

	// Create our state group node
	auto groupNode = renderer.CreateStateGroupNode();
	groupNode->BindResourceArray(colorDataId, colorArray.get());
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

		// Restore the baseline data before testing the next batch configuration
		REQUIRE(colorArray->QueueDataUpdate(baseColors));
		auto frameBufferCapture = renderer.CreateFrameBufferOutput();
		frameBufferCapture->SetDetachAfterCapture(true);
		mainWindowFrameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
		DrawOneFrame();
		session.AddTestImageResult("Transfer" + configName + "Baseline", "A fullscreen image with red, green and blue vertical bands.", std::move(frameBufferCapture), IImageDiff::Algorithm::AllOfThem, 0.95);

		// Queue a CPU update and two GPU transfers into the same transfer batch
		auto transferBatch = renderer.CreateTransferBatch(config.startTiming, config.endTiming);
		REQUIRE(colorArray->QueueDataUpdate(&updatedMiddleColor, 1, 1, transferBatch.get()));
		REQUIRE(transferArray->QueueDataTransfer(colorArray.get(), 1, 0, 0, transferBatch.get()));
		REQUIRE(transferArray->QueueDataTransfer(colorArray.get(), 1, 1, 2, transferBatch.get()));
		REQUIRE(!transferBatch->IsComplete());

		// Ensure the batch does not affect rendering until it has been submitted
		frameBufferCapture = renderer.CreateFrameBufferOutput();
		frameBufferCapture->SetDetachAfterCapture(true);
		mainWindowFrameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
		DrawOneFrame();
		session.AddTestImageResult("Transfer" + configName + "Deferred", "The batched texel-array transfer has not been submitted yet, so the red, green and blue bands should be unchanged.", std::move(frameBufferCapture), IImageDiff::Algorithm::AllOfThem, 0.95);

		// Submit the batch
		REQUIRE(transferBatch->SubmitBatch());

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
			transferBatch->WaitForComplete();
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
		session.AddTestImageResult("Transfer" + configName + "Submitted", "The batched texel-array transfer has completed, producing cyan, magenta and yellow vertical bands.", std::move(frameBufferCapture), IImageDiff::Algorithm::AllOfThem, 0.95);
		REQUIRE(transferBatch->IsComplete());
	}

	// Remove all our defined render passes
	renderer.RemoveAllRenderPasses();
	return true;
}

} // namespace cobalt::graphics::testing
