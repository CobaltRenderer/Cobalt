// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "../UnitTestBase.h"
#include <unordered_set>
namespace cobalt::graphics::testing {
using namespace cobalt::graphics;

namespace {
// Define our shader programs
const std::string ComputeShader = R"(
struct BufferEntryOut
{
	uint4 threadId;
	uint4 checkValue;
};

RWStructuredBuffer<BufferEntryOut> bufferDataOut;

[numthreads(1, 1, 1)]
void main(uint3 threadId : SV_DispatchThreadID)
{
	uint counterValue = bufferDataOut.IncrementCounter();
	bufferDataOut[counterValue].threadId = uint4(threadId, threadId.x + threadId.y + threadId.z);
	bufferDataOut[counterValue].checkValue = uint4(1, 2, 3, 4);
}
)";

// Define our local structures
struct BufferEntryOut
{
	V4UInt32 threadId;
	V4UInt32 checkValue;
};
} // namespace

DEFINE_UNIT_TEST_WITH_BASE("Compute/BasicCompute", UnitTestBase)
{
	// Ensure compute shaders are supported by the current renderer
	if (!session.Device().IsFeatureSupported(IGraphicsDevice::Feature::ComputeShaders))
	{
		session.AddTestSkipped("Skipped test", "This test was skipped, as the current renderer doesn't support compute shaders on this device.");
		return true;
	}

	// Retrieve our important objects
	auto& renderer = session.Renderer();

	// Set our thread counts for the compute operation
	uint32_t threadCount = 50;

	// Create and compile our shader program
	auto shaderProgram = renderer.CreateShaderProgram();
	REQUIRE(shaderProgram->LoadShaderStage(IShaderProgram::ShaderStage::Compute, ShaderSourceInfoHLSL(ComputeShader)));
	REQUIRE(shaderProgram->CompileProgram());

	// Create our output data array
	auto dataArrayOut = renderer.CreateDataArray();
	size_t structureSizeOut = sizeof(BufferEntryOut);
	dataArrayOut->SetBufferLayout(structureSizeOut, threadCount, true);
	REQUIRE(dataArrayOut->AllocateMemory());

	// Create our output data array capture target
	auto dataArrayCapturedOutput = renderer.CreateDataArrayOutput();
	dataArrayOut->AddOutputCaptureTarget(dataArrayCapturedOutput.get());

	// Set up additional output targets before dispatch so the capture lifecycle can be tested without running the
	// counter-based compute task more than once.
	auto uncapturedOutput = renderer.CreateDataArrayOutput();
	std::vector<BufferEntryOut> uncapturedReadData(1);
	REQUIRE(!uncapturedOutput->ReadBufferData(uncapturedReadData.data(), uncapturedReadData.size() * sizeof(BufferEntryOut)));
	uint32_t uncapturedCounterValue = 0;
	REQUIRE(!uncapturedOutput->ReadCounterValue(uncapturedCounterValue));
	REQUIRE(uncapturedOutput->GetEntryCount() == 0);
	REQUIRE(uncapturedOutput->GetEntrySizeInBytes() == 0);

	auto removedCaptureOutput = renderer.CreateDataArrayOutput();
	dataArrayOut->AddOutputCaptureTarget(removedCaptureOutput.get());
	dataArrayOut->RemoveOutputCaptureTarget(removedCaptureOutput.get());

	auto regionCaptureOutput = renderer.CreateDataArrayOutput();
	regionCaptureOutput->SetDetachAfterCapture(true);
	regionCaptureOutput->SetArrayCaptureRegion(5, 10, false);
	dataArrayOut->AddOutputCaptureTarget(regionCaptureOutput.get());

	// Retrieve our shader attribute IDs
	auto dataArrayOutId = shaderProgram->GetResourceArrayId("bufferDataOut");

	// Create our state group node
	auto groupNode = renderer.CreateStateGroupNode();
	groupNode->BindResourceArray(dataArrayOutId, dataArrayOut.get());

	// Define our compute task
	V3UInt32 threadGroupCounts(threadCount, 1, 1);
	groupNode->SetComputeTask(threadGroupCounts);

	// Create our program node
	auto programNode = renderer.CreateProgramNode();
	REQUIRE(programNode->BindShaderProgram(shaderProgram.get()));
	programNode->AddChildNode(groupNode.get());

	// Create our render pass node
	auto renderPassNode = renderer.CreateRenderPassNode();
	renderPassNode->AddChildNode(programNode.get());

	// Bind our render tree to the renderer
	renderer.SetRenderPasses(&renderPassNode, 1);

	// Perform the compute task
	DrawOneFrame();

	// Ensure the write buffer was modified and captured correctly
	REQUIRE(dataArrayCapturedOutput->HasCapturedOutput());
	REQUIRE(dataArrayCapturedOutput->HasCapturedCounterValue());
	REQUIRE(dataArrayCapturedOutput->GetEntryCount() == threadCount);
	REQUIRE(dataArrayCapturedOutput->GetEntrySizeInBytes() == sizeof(BufferEntryOut));
	uint32_t counterValue;
	REQUIRE(dataArrayCapturedOutput->ReadCounterValue(counterValue));
	REQUIRE(counterValue == threadCount);
	std::vector<unsigned char> oversizedOutputRead((threadCount * sizeof(BufferEntryOut)) + 1);
	REQUIRE(!dataArrayCapturedOutput->ReadBufferData(oversizedOutputRead.data(), oversizedOutputRead.size()));
	std::vector<BufferEntryOut> outputData;
	outputData.resize(threadCount, BufferEntryOut{});
	REQUIRE(dataArrayCapturedOutput->ReadBufferData(outputData.data(), outputData.size() * sizeof(BufferEntryOut)));
	std::unordered_set<uint64_t> locatedThreadEntries;
	for (size_t i = 0; i < threadCount; ++i)
	{
		const auto& entry = outputData[i];
		REQUIRE(entry.checkValue.X() == 1);
		REQUIRE(entry.checkValue.Y() == 2);
		REQUIRE(entry.checkValue.Z() == 3);
		REQUIRE(entry.checkValue.W() == 4);

		uint64_t threadKey = ((uint64_t)entry.threadId.X() << 32) | ((uint64_t)entry.threadId.Y() << 16) | entry.threadId.Z();
		REQUIRE(locatedThreadEntries.find(threadKey) == locatedThreadEntries.end());
		locatedThreadEntries.insert(threadKey);

		REQUIRE(entry.threadId.X() < threadGroupCounts.X());
		REQUIRE(entry.threadId.Y() < threadGroupCounts.Y());
		REQUIRE(entry.threadId.Z() < threadGroupCounts.Z());
		REQUIRE(entry.threadId.W() == (entry.threadId.X() + entry.threadId.Y() + entry.threadId.Z()));
	}
	session.AddTestSuccess("DataValueMatch", "Testing the output array to ensure it is valid and complete.");

	// Verify the additional output targets observed the requested lifecycle behavior.
	REQUIRE(regionCaptureOutput->HasCapturedOutput());
	REQUIRE(!regionCaptureOutput->HasCapturedCounterValue());
	REQUIRE(regionCaptureOutput->GetEntryCount() == 5);
	REQUIRE(regionCaptureOutput->GetEntrySizeInBytes() == sizeof(BufferEntryOut));
	std::vector<BufferEntryOut> regionOutputData(5);
	REQUIRE(regionCaptureOutput->ReadBufferData(regionOutputData.data(), regionOutputData.size() * sizeof(BufferEntryOut)));
	regionCaptureOutput->ClearCapturedOutput();
	REQUIRE(!regionCaptureOutput->HasCapturedOutput());
	REQUIRE(!removedCaptureOutput->HasCapturedOutput());
	session.AddTestSuccess("DataArrayOutputLifecycle", "Data array output capture reported uncaptured state, capture regions, counter suppression, detach-after-capture state, readback sizing, clearing, and removal correctly.");

	// Removing the compute task should prevent subsequent dispatches.
	dataArrayCapturedOutput->ClearCapturedOutput();
	REQUIRE(!dataArrayCapturedOutput->HasCapturedOutput());
	groupNode->RemoveComputeTask();
	DrawOneFrame();
	REQUIRE(!dataArrayCapturedOutput->HasCapturedOutput());
	session.AddTestSuccess("ComputeTaskRemoved", "Removing the compute task prevented the state group from dispatching or capturing new output.");

	// Remove all our defined render passes
	renderer.RemoveAllRenderPasses();
	return true;
}

} // namespace cobalt::graphics::testing
