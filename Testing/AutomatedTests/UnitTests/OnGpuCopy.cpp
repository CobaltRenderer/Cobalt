// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "../GeometryHelper.h"
#include "../UnitTestBase.h"
#include <numeric>
#include <random>

namespace cobalt::graphics::testing {
using namespace cobalt::graphics;

// Define our shader programs
const std::string VertexShader = R"(
struct VSInput
  {
    float3 position : position;
    float3 normal : normal;
};

struct VSOutput
  {
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
    OUT.color = float4(0,0.8f,0,1.0f);

    return OUT;
}
)";

const std::string FragmentShader = R"(
struct VSOutput
  {
    float4 position : SV_POSITION;
    float3 normal : normal;
    float4 color : color;
};

float4 main(VSOutput IN) : SV_TARGET0
{
    float4 color = float4(IN.color.rgb, 1.0f);
    color.rgb *= abs(dot(normalize(IN.normal), normalize(float3(3.0f,-4.0f, -5.0f))));

    return color;
}
)";

DEFINE_UNIT_TEST_WITH_BASE("Scene/OnGpuCopy", UnitTestBase)
{
	if (!session.Device().IsFeatureSupported(IGraphicsDevice::Feature::ResourceArrays))
	{
		session.AddTestSkipped("Skipped test", "This test was skipped, as the current renderer doesn't support data arrays on this device.");
		return true;
	}

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
	auto normalAttributeId = shaderProgram->GetVertexAttributeId("normal");

	// Generate data for the object we want to render
	std::vector<V3Float32> positionVertexData;
	std::vector<V3Float32> normalVertexData;
	std::vector<V1UInt32> indexData;
	Geometry().CreateModelStanfordBunny(positionVertexData, normalVertexData, indexData);
	auto pointCount = positionVertexData.size();

	std::vector<V3Float32> positionVertexDataGarbage(positionVertexData.size());
	std::vector<V3Float32> normalVertexDataGarbage(normalVertexData.size(), V3Float32(0.25f, 0.25f, 0.25f));
	std::vector<V1UInt32> indexDataGarbage(indexData.size());

	// Use some stable seeded noise for these buffers, so we can debug the difference between
	// "it's all 0" and "the copy didn't work"
	{
		PseudoRandomGenerator rand;
		for (auto& vertex : positionVertexDataGarbage)
		{
			vertex.X() = rand.GetNext(-0.4f, 0.4f);
			vertex.Y() = rand.GetNext(-0.4f, 0.4f);
			vertex.Z() = rand.GetNext(-0.4f, 0.4f);
		}

		for (auto& index : indexDataGarbage)
		{
			index.X() = rand.GetNext((uint32_t)pointCount);
		}
	}

	// Create our vertex buffer and set up all the aliasing stuff. We're intentionally putting all our data in one
	// buffer, leaving the other empty, and rendering from the empty buffer. All will become clear later.

	using IVAPH = IVertexAttribute::PerformanceHint;

	VertexAttribute<V3Float32> vertexAttributePositionInactive(
	  positionVertexData.size(),
	  IVAPH::WriteRarely | IVAPH::ReadNever,
	  IVAPH::WriteRarely | IVAPH::ReadRarely);
	VertexAttribute<V3Float32> vertexAttributePositionActive(
	  positionVertexData.size(),
	  IVAPH::WriteRarely | IVAPH::ReadNever,
	  IVAPH::WriteRarely | IVAPH::ReadRarely);
	VertexAttribute<V3Float32> vertexAttributeNormalInactive(
	  normalVertexData.size(),
	  IVAPH::WriteRarely | IVAPH::ReadNever,
	  IVAPH::WriteRarely | IVAPH::ReadRarely);
	VertexAttribute<V3Float32> vertexAttributeNormalActive(
	  normalVertexData.size(),
	  IVAPH::WriteRarely | IVAPH::ReadNever,
	  IVAPH::WriteRarely | IVAPH::ReadRarely);

	auto coordBufferInactive = renderer.CreateVertexBuffer();
	auto coordBufferActive = renderer.CreateVertexBuffer();
	auto normalBufferInactive = renderer.CreateVertexBuffer();
	auto normalBufferActive = renderer.CreateVertexBuffer();
	auto texelArrayInactiveNormal = renderer.CreateTexelArray();
	auto texelArrayActiveNormal = renderer.CreateTexelArray();
	auto texelArrayInactiveCoord = renderer.CreateTexelArray();
	auto texelArrayActiveCoord = renderer.CreateTexelArray();
	auto texelArrayInactiveIndex = renderer.CreateTexelArray();
	auto texelArrayActiveIndex = renderer.CreateTexelArray();

	for (auto* texelArray : {texelArrayInactiveCoord.get(), texelArrayActiveCoord.get(), texelArrayInactiveNormal.get(), texelArrayActiveNormal.get(), texelArrayInactiveIndex.get(), texelArrayActiveIndex.get()})
	{
		using F_Uf = ITexelArray::UsageFlags;
		using F_Ph = IResourceArray::PerformanceHint;

		texelArray->SetUsageFlags(F_Uf::TransferDestination |
		                          F_Uf::TransferSource);

		texelArray->SetPerformanceHints(
		  F_Ph::ReadNever | F_Ph::WriteRarely,
		  F_Ph::ReadRarely | F_Ph::WriteRarely);
	}

	texelArrayInactiveNormal->SetBufferLayout(ITexelArray::ImageFormat::R, ITexelArray::DataFormat::Float32, pointCount * 3);
	texelArrayActiveNormal->SetBufferLayout(ITexelArray::ImageFormat::R, ITexelArray::DataFormat::Float32, pointCount * 3);
	texelArrayInactiveCoord->SetBufferLayout(ITexelArray::ImageFormat::R, ITexelArray::DataFormat::Float32, pointCount * 3);
	texelArrayActiveCoord->SetBufferLayout(ITexelArray::ImageFormat::R, ITexelArray::DataFormat::Float32, pointCount * 3);
	texelArrayInactiveIndex->SetBufferLayout(ITexelArray::ImageFormat::R, ITexelArray::DataFormat::UInt32, indexData.size());
	texelArrayActiveIndex->SetBufferLayout(ITexelArray::ImageFormat::R, ITexelArray::DataFormat::UInt32, indexData.size());

	// Put the real data in the inactive buffers.
	REQUIRE(coordBufferInactive->BindVertexAttributeManualLayout(vertexAttributePositionInactive, 0, 12));
	REQUIRE(normalBufferInactive->BindVertexAttributeManualLayout(vertexAttributeNormalInactive, 0, 12));
	REQUIRE(vertexAttributePositionInactive.SetInitialData(positionVertexData));
	REQUIRE(vertexAttributeNormalInactive.SetInitialData(normalVertexData));

	REQUIRE(coordBufferInactive->AllocateMemoryWithAlias(texelArrayInactiveCoord.get()));
	REQUIRE(normalBufferInactive->AllocateMemoryWithAlias(texelArrayInactiveNormal.get()));

	// And put garbage random noise in the active one:
	REQUIRE(coordBufferActive->BindVertexAttributeManualLayout(vertexAttributePositionActive, 0, 12));
	REQUIRE(normalBufferActive->BindVertexAttributeManualLayout(vertexAttributeNormalActive, 0, 12));
	REQUIRE(vertexAttributePositionActive.SetInitialData(positionVertexDataGarbage));
	REQUIRE(vertexAttributeNormalActive.SetInitialData(normalVertexDataGarbage));

	REQUIRE(coordBufferActive->AllocateMemoryWithAlias(texelArrayActiveCoord.get()));
	REQUIRE(normalBufferActive->AllocateMemoryWithAlias(texelArrayActiveNormal.get()));

	// Create our index buffers and their aliases and populate one with data, and leave the other empty.
	IndexAttribute<V1UInt32> indexAttributeInactive(indexData.size(), IIndexAttribute::PerformanceHint::WriteNever | IIndexAttribute::PerformanceHint::ReadNever, IIndexAttribute::PerformanceHint::WriteNever | IIndexAttribute::PerformanceHint::ReadOften);
	IndexAttribute<V1UInt32> indexAttributeActive(indexData.size(), IIndexAttribute::PerformanceHint::WriteNever | IIndexAttribute::PerformanceHint::ReadNever, IIndexAttribute::PerformanceHint::WriteNever | IIndexAttribute::PerformanceHint::ReadOften);
	auto indexBufferInactive = renderer.CreateIndexBuffer();
	auto indexBufferActive = renderer.CreateIndexBuffer();
	REQUIRE(indexBufferInactive->BindIndexAttributeManualLayout(indexAttributeInactive, 0, 4));
	REQUIRE(indexBufferActive->BindIndexAttributeManualLayout(indexAttributeActive, 0, 4));
	REQUIRE(indexAttributeInactive.SetInitialData(indexData));
	REQUIRE(indexAttributeActive.SetInitialData(indexDataGarbage));
	REQUIRE(indexBufferInactive->AllocateMemoryWithAlias(texelArrayInactiveIndex.get()));
	REQUIRE(indexBufferActive->AllocateMemoryWithAlias(texelArrayActiveIndex.get()));

	// Create our renderable node
	auto renderableNode = renderer.CreateRenderableNode();
	REQUIRE(renderableNode->BindVertexAttribute(vertexAttributePositionActive, positionAttributeId));
	REQUIRE(renderableNode->BindVertexAttribute(vertexAttributeNormalActive, normalAttributeId));
	REQUIRE(renderableNode->BindIndexAttribute(indexAttributeActive));
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

	auto viewProj = Transform().LookAtCenterPerspective(session.TestWindowSizeAsFloat(), V3Float32(0.0f, -2.0f, 0.0f));
	renderableNode->SetStateValue(shaderProgram->GetStateValueId("viewProj"), viewProj);

	// Draw a control scene - should be noise, because the buffers are filled with noise.
	{
		auto frameBufferCapture = renderer.CreateFrameBufferOutput();
		frameBufferCapture->SetDetachAfterCapture(true);
		mainWindowFrameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
		DrawOneFrame();
		session.AddTestImageResult("OnGpuCopyWarmup", "Should be rendering a scene of random noise", std::move(frameBufferCapture), IImageDiff::Algorithm::AllOfThem, 0.95);
	}

	// Now copy in all the data entirely on the GPU.
	REQUIRE(texelArrayInactiveCoord->QueueDataTransfer(
	  texelArrayActiveCoord.get(),
	  pointCount * 3,
	  0,
	  0));

	REQUIRE(texelArrayInactiveNormal->QueueDataTransfer(
	  texelArrayActiveNormal.get(),
	  pointCount * 3,
	  0,
	  0));

	REQUIRE(texelArrayInactiveIndex->QueueDataTransfer(
	  texelArrayActiveIndex.get(),
	  indexData.size(),
	  0,
	  0));

	// Now, draw the bunny, hopefully.
	{
		auto frameBufferCapture = renderer.CreateFrameBufferOutput();
		frameBufferCapture->SetDetachAfterCapture(true);
		mainWindowFrameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
		DrawOneFrame();
		session.AddTestImageResult("OnGpuCopyABunny", "We've copied in the bunny over the null data", std::move(frameBufferCapture), IImageDiff::Algorithm::AllOfThem, 0.95);
	}

	// Next test, update the unused buffer with some bizare normals we just made up CPU->GPU
	std::fill(normalVertexDataGarbage.begin(), normalVertexDataGarbage.end(), V3Float32(0, 0, 1));
	REQUIRE(vertexAttributeNormalInactive.QueueDataUpdate(normalVertexDataGarbage.data(), normalVertexDataGarbage.size(), 0, 12));

	// Now, draw the bunny and hopefully it's unchanged by the distance modificaiton.
	{
		auto frameBufferCapture = renderer.CreateFrameBufferOutput();
		frameBufferCapture->SetDetachAfterCapture(true);
		mainWindowFrameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
		DrawOneFrame();
		session.AddTestImageResult("OnGpuCopyABunnyDistantChange", "Bunny should be unchanged, we've modified a background buffer", std::move(frameBufferCapture), IImageDiff::Algorithm::AllOfThem, 0.95);
	}

	// Now, draw the bunny and hopefully some parts are blank. Testing partial transfers.
	REQUIRE(texelArrayInactiveNormal->QueueDataTransfer(
	  texelArrayActiveNormal.get(),
	  pointCount, // 1/3 of the normals
	  0,
	  0));

	{
		auto frameBufferCapture = renderer.CreateFrameBufferOutput();
		frameBufferCapture->SetDetachAfterCapture(true);
		mainWindowFrameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
		DrawOneFrame();
		session.AddTestImageResult("OnGpuCopyABunnyNormalsKindaGone", "Bunny has some normals are blatted by a gpu->gpu copy", std::move(frameBufferCapture), IImageDiff::Algorithm::AllOfThem, 0.95);
	}

	// Now test copies which don't touch the start or end:
	REQUIRE(texelArrayInactiveNormal->QueueDataTransfer(
	  texelArrayActiveNormal.get(),
	  pointCount, // 1/3 of the normals
	  pointCount, // 1/3 of the way in
	  pointCount));

	// Now, draw the bunny and hopefully some parts are blank
	{
		auto frameBufferCapture = renderer.CreateFrameBufferOutput();
		frameBufferCapture->SetDetachAfterCapture(true);
		mainWindowFrameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
		DrawOneFrame();
		session.AddTestImageResult("OnGpuCopyABunnyNormalsKindaMoreGone", "Bunny has some normals are blatted by a gpu->gpu copy", std::move(frameBufferCapture), IImageDiff::Algorithm::AllOfThem, 0.95);
	}

	// Now test that we can update the active buffer using the normal CPU->GPU transfer
	REQUIRE(vertexAttributeNormalActive.QueueDataUpdate(normalVertexData.data(), normalVertexData.size(), 0, 12));

	{
		auto frameBufferCapture = renderer.CreateFrameBufferOutput();
		frameBufferCapture->SetDetachAfterCapture(true);
		mainWindowFrameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
		DrawOneFrame();
		session.AddTestImageResult("OnGpuCopyABunnyNormalsBack", "Bunny normals were correctly copied from the CPU", std::move(frameBufferCapture), IImageDiff::Algorithm::AllOfThem, 0.95);
	}

	// Now test that we can copy FROM a rendering buffer to a non rendering one.
	REQUIRE(texelArrayActiveCoord->QueueDataTransfer(
	  texelArrayInactiveCoord.get(),
	  pointCount * 3,
	  0,
	  0));

	// Barrier between the B->A and CPU->B copies. There could be a race, (and that's ok because nobody would
	// do this in the real world), but I don't want the test to fail if the renderer is optimised so these
	// copies are unordered.
	DrawOneFrame();

	// Now overwrite the active data with random noise so we can test this better.
	REQUIRE(vertexAttributePositionActive.QueueDataUpdate(
	  positionVertexDataGarbage.data(),
	  pointCount,
	  0,
	  12));

	{
		auto frameBufferCapture = renderer.CreateFrameBufferOutput();
		frameBufferCapture->SetDetachAfterCapture(true);
		mainWindowFrameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
		DrawOneFrame();
		session.AddTestImageResult("OnGpuCopyBackToNoise", "We copied the vertex coordinates out, and then blatted over them with noise", std::move(frameBufferCapture), IImageDiff::Algorithm::AllOfThem, 0.95);
	}

	REQUIRE(texelArrayInactiveCoord->QueueDataTransfer(
	  texelArrayActiveCoord.get(),
	  pointCount * 3,
	  0,
	  0));

	{
		auto frameBufferCapture = renderer.CreateFrameBufferOutput();
		frameBufferCapture->SetDetachAfterCapture(true);
		mainWindowFrameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
		DrawOneFrame();
		session.AddTestImageResult("OnGpuCopyTaDa", "And successfully copied the data back from the on-gpu copy, proving that we could copy both from and to a rendering buffer", std::move(frameBufferCapture), IImageDiff::Algorithm::AllOfThem, 0.95);
	}

	// Remove all our defined render passes
	renderer.RemoveAllRenderPasses();
	return true;
}

} // namespace cobalt::graphics::testing
