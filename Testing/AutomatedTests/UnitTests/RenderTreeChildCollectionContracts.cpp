// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "../UnitTestBase.h"
#include <array>
#include <vector>
namespace cobalt::graphics::testing {
using namespace cobalt::graphics;

namespace {

const std::string VertexShader = R"(
struct VSInput {
    float4 position : position;
};

float4 main(VSInput IN) : SV_POSITION
{
    return IN.position;
}
)";
const std::string FragmentShader = R"(
uniform float4 drawColor;

float4 main() : SV_TARGET0
{
    return drawColor;
}
)";

IRenderableNode::unique_ptr CreateRenderableNode(IRenderer& renderer)
{
	auto renderableNode = renderer.CreateRenderableNode();
	if (!renderableNode->SetPrimitiveMode(IRenderableNode::PrimitiveMode::Triangles))
	{
		return nullptr;
	}
	return renderableNode;
}

std::vector<V4Float32> CreateVerticalStrip(float minX, float maxX)
{
	return {
	  V4Float32(minX, 1.0f, 0.5f, 1.0f),
	  V4Float32(minX, -1.0f, 0.5f, 1.0f),
	  V4Float32(maxX, -1.0f, 0.5f, 1.0f),
	  V4Float32(minX, 1.0f, 0.5f, 1.0f),
	  V4Float32(maxX, -1.0f, 0.5f, 1.0f),
	  V4Float32(maxX, 1.0f, 0.5f, 1.0f),
	};
}

} // namespace

DEFINE_UNIT_TEST_WITH_BASE("Scene/RenderTreeChildCollectionContracts", UnitTestBase)
{
	// Retrieve our important objects
	auto& renderer = session.Renderer();
	auto& uiThread = session.UIThread();
	auto& testWindowInfo = *session.TestWindowPlatformInfo();

	// Exercise the optimized child-removal path where leading nodes are skipped, consecutive target nodes are removed,
	// and trailing nodes are shifted down after all targets have been found.
	{
		std::array<IStateGroupNode::unique_ptr, 5> stateGroupNodes = {renderer.CreateStateGroupNode(), renderer.CreateStateGroupNode(), renderer.CreateStateGroupNode(), renderer.CreateStateGroupNode(), renderer.CreateStateGroupNode()};
		std::array<IStateGroupNode*, 5> rawStateGroupNodes = {stateGroupNodes[0].get(), stateGroupNodes[1].get(), stateGroupNodes[2].get(), stateGroupNodes[3].get(), stateGroupNodes[4].get()};
		std::array<IStateGroupNode*, 2> removedStateGroupNodes = {stateGroupNodes[1].get(), stateGroupNodes[2].get()};

		auto programNode = renderer.CreateProgramNode();
		programNode->AddChildNodes(rawStateGroupNodes.data(), rawStateGroupNodes.size());
		programNode->RemoveChildNodes(removedStateGroupNodes.data(), removedStateGroupNodes.size());
	}

	// Repeat the same removal shape for render-pass program children, including the raw-pointer default-state array
	// overload and the unique_ptr overload with no default-state array supplied.
	{
		std::array<IProgramNode::unique_ptr, 5> programNodes = {renderer.CreateProgramNode(), renderer.CreateProgramNode(), renderer.CreateProgramNode(), renderer.CreateProgramNode(), renderer.CreateProgramNode()};
		std::array<IDefaultState::unique_ptr, 5> defaultStates = {renderer.CreateDefaultState(), renderer.CreateDefaultState(), renderer.CreateDefaultState(), renderer.CreateDefaultState(), renderer.CreateDefaultState()};
		std::array<IProgramNode*, 5> rawProgramNodes = {programNodes[0].get(), programNodes[1].get(), programNodes[2].get(), programNodes[3].get(), programNodes[4].get()};
		std::array<IDefaultState*, 5> rawDefaultStates = {defaultStates[0].get(), defaultStates[1].get(), defaultStates[2].get(), defaultStates[3].get(), defaultStates[4].get()};
		std::array<IProgramNode*, 2> removedProgramNodes = {programNodes[1].get(), programNodes[2].get()};

		auto renderPassNode = renderer.CreateRenderPassNode();
		renderPassNode->SetChildNodes(rawProgramNodes.data(), rawProgramNodes.size(), rawDefaultStates.data());
		renderPassNode->RemoveChildNodes(removedProgramNodes.data(), removedProgramNodes.size());

		std::array<IProgramNode::unique_ptr, 2> replacementProgramNodes = {renderer.CreateProgramNode(), renderer.CreateProgramNode()};
		renderPassNode->SetChildNodes(replacementProgramNodes.data(), replacementProgramNodes.size());
	}

	// Exercise the equivalent collection operations on renderable children using fresh renderables for every parent.
	{
		std::array<IRenderableNode::unique_ptr, 3> renderableNodes = {CreateRenderableNode(renderer), CreateRenderableNode(renderer), CreateRenderableNode(renderer)};
		for (const auto& renderableNode : renderableNodes)
		{
			REQUIRE(renderableNode);
		}
		std::array<IRenderableNode*, 3> rawRenderableNodes = {renderableNodes[0].get(), renderableNodes[1].get(), renderableNodes[2].get()};
		std::array<IRenderableNode*, 1> removedRenderableNodes = {renderableNodes[1].get()};

		auto stateGroupNode = renderer.CreateStateGroupNode();
		stateGroupNode->SetChildNodes(rawRenderableNodes.data(), rawRenderableNodes.size());
		stateGroupNode->RemoveChildNodes(removedRenderableNodes.data(), removedRenderableNodes.size());
		stateGroupNode->RemoveAllChildNodes();
	}

	// Draw a render tree after removing two adjacent renderable children from the middle of a five-child array. This
	// proves the collection compaction path leaves the expected visible children in place.
	{
		auto frameBuffer = renderer.CreateFrameBuffer();
		frameBuffer->DefineViewportRegion(V2UInt32(0, 0), session.TestWindowSize());
		REQUIRE(uiThread.InvokeSync([&] { return frameBuffer->BindWindow(testWindowInfo, IFrameBuffer::WindowDepthStencilMode::None); }));

		auto shaderProgram = renderer.CreateShaderProgram();
		REQUIRE(shaderProgram->LoadShaderStage(IShaderProgram::ShaderStage::Vertex, ShaderSourceInfoHLSL(VertexShader)));
		REQUIRE(shaderProgram->LoadShaderStage(IShaderProgram::ShaderStage::Fragment, ShaderSourceInfoHLSL(FragmentShader)));
		REQUIRE(shaderProgram->CompileProgram());
		auto positionAttributeId = shaderProgram->GetVertexAttributeId("position");
		auto drawColorStateId = shaderProgram->GetStateValueId("drawColor");

		std::array<std::vector<V4Float32>, 5> stripData = {{
		  CreateVerticalStrip(-1.0f, -0.6f),
		  CreateVerticalStrip(-0.6f, -0.2f),
		  CreateVerticalStrip(-0.2f, 0.2f),
		  CreateVerticalStrip(0.2f, 0.6f),
		  CreateVerticalStrip(0.6f, 1.0f),
		}};
		std::array<V4Float32, 5> stripColors = {{
		  V4Float32(1.0f, 0.0f, 0.0f, 1.0f),
		  V4Float32(0.0f, 1.0f, 0.0f, 1.0f),
		  V4Float32(0.0f, 0.0f, 1.0f, 1.0f),
		  V4Float32(1.0f, 1.0f, 0.0f, 1.0f),
		  V4Float32(1.0f, 0.0f, 1.0f, 1.0f),
		}};

		auto vertexBuffer = renderer.CreateVertexBuffer();
		std::array<std::unique_ptr<VertexAttribute<V4Float32>>, 5> stripAttributes;
		for (size_t i = 0; i < stripAttributes.size(); ++i)
		{
			stripAttributes[i] = std::make_unique<VertexAttribute<V4Float32>>(stripData[i].size(), IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadNever, IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadOften);
			REQUIRE(vertexBuffer->BindVertexAttribute(*stripAttributes[i]));
			REQUIRE(stripAttributes[i]->SetInitialData(stripData[i]));
		}
		REQUIRE(vertexBuffer->AllocateMemory());

		// Draw the program-node state-group collection after removing two adjacent middle children.
		{
			std::array<IRenderableNode::unique_ptr, 5> renderableNodes;
			std::array<IStateGroupNode::unique_ptr, 5> stateGroupNodes;
			std::array<IStateGroupNode*, 5> rawStateGroupNodes = {};
			for (size_t i = 0; i < renderableNodes.size(); ++i)
			{
				renderableNodes[i] = renderer.CreateRenderableNode();
				REQUIRE(renderableNodes[i]->BindVertexAttribute(*stripAttributes[i], positionAttributeId));
				REQUIRE(renderableNodes[i]->SetPrimitiveMode(IRenderableNode::PrimitiveMode::Triangles));
				renderableNodes[i]->SetStateValue(drawColorStateId, stripColors[i]);

				stateGroupNodes[i] = renderer.CreateStateGroupNode();
				stateGroupNodes[i]->SetDepthTestEnabled(false);
				stateGroupNodes[i]->SetDepthWriteEnabled(false);
				stateGroupNodes[i]->AddChildNode(renderableNodes[i].get());
				rawStateGroupNodes[i] = stateGroupNodes[i].get();
			}

			auto programNode = renderer.CreateProgramNode();
			REQUIRE(programNode->BindShaderProgram(shaderProgram.get()));
			programNode->AddChildNodes(rawStateGroupNodes.data(), rawStateGroupNodes.size());
			std::array<IStateGroupNode*, 2> removedStateGroupNodes = {stateGroupNodes[1].get(), stateGroupNodes[2].get()};
			programNode->RemoveChildNodes(removedStateGroupNodes.data(), removedStateGroupNodes.size());

			auto renderPassNode = renderer.CreateRenderPassNode();
			renderPassNode->BindFrameBuffer(frameBuffer.get());
			renderPassNode->SetAttachmentClearData(IFrameBuffer::AttachmentType::Color, 0, V4Float32(0.0f, 0.0f, 0.0f, 1.0f));
			renderPassNode->AddChildNode(programNode.get());
			renderer.SetRenderPasses(&renderPassNode, 1);

			auto frameBufferCapture = renderer.CreateFrameBufferOutput();
			frameBufferCapture->SetDetachAfterCapture(true);
			frameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
			DrawOneFrame();
			session.AddTestImageResult("RemovedMiddleStateGroupChildren", "A red strip on the left, black gap in the middle, and yellow and magenta strips on the right after removing two middle state-group children.", std::move(frameBufferCapture), IImageDiff::Algorithm::NaiveDiff, 0.98);
			renderer.RemoveAllRenderPasses();
		}

		// Draw the render-pass program collection after removing two adjacent middle children.
		{
			std::array<IRenderableNode::unique_ptr, 5> renderableNodes;
			std::array<IStateGroupNode::unique_ptr, 5> stateGroupNodes;
			std::array<IProgramNode::unique_ptr, 5> programNodes;
			std::array<IProgramNode*, 5> rawProgramNodes = {};
			for (size_t i = 0; i < renderableNodes.size(); ++i)
			{
				renderableNodes[i] = renderer.CreateRenderableNode();
				REQUIRE(renderableNodes[i]->BindVertexAttribute(*stripAttributes[i], positionAttributeId));
				REQUIRE(renderableNodes[i]->SetPrimitiveMode(IRenderableNode::PrimitiveMode::Triangles));
				renderableNodes[i]->SetStateValue(drawColorStateId, stripColors[i]);

				stateGroupNodes[i] = renderer.CreateStateGroupNode();
				stateGroupNodes[i]->SetDepthTestEnabled(false);
				stateGroupNodes[i]->SetDepthWriteEnabled(false);
				stateGroupNodes[i]->AddChildNode(renderableNodes[i].get());

				programNodes[i] = renderer.CreateProgramNode();
				REQUIRE(programNodes[i]->BindShaderProgram(shaderProgram.get()));
				programNodes[i]->AddChildNode(stateGroupNodes[i].get());
				rawProgramNodes[i] = programNodes[i].get();
			}

			auto renderPassNode = renderer.CreateRenderPassNode();
			renderPassNode->BindFrameBuffer(frameBuffer.get());
			renderPassNode->SetAttachmentClearData(IFrameBuffer::AttachmentType::Color, 0, V4Float32(0.0f, 0.0f, 0.0f, 1.0f));
			renderPassNode->SetChildNodes(rawProgramNodes.data(), rawProgramNodes.size());
			std::array<IProgramNode*, 2> removedProgramNodes = {programNodes[1].get(), programNodes[2].get()};
			renderPassNode->RemoveChildNodes(removedProgramNodes.data(), removedProgramNodes.size());
			renderer.SetRenderPasses(&renderPassNode, 1);

			auto frameBufferCapture = renderer.CreateFrameBufferOutput();
			frameBufferCapture->SetDetachAfterCapture(true);
			frameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
			DrawOneFrame();
			session.AddTestImageResult("RemovedMiddleProgramChildren", "A red strip on the left, black gap in the middle, and yellow and magenta strips on the right after removing two middle program children.", std::move(frameBufferCapture), IImageDiff::Algorithm::NaiveDiff, 0.98);
			renderer.RemoveAllRenderPasses();
		}

		// Draw after replacing the render-pass program children, proving the replacement list is the active collection.
		{
			std::array<IRenderableNode::unique_ptr, 2> renderableNodes;
			std::array<IStateGroupNode::unique_ptr, 2> stateGroupNodes;
			std::array<IProgramNode::unique_ptr, 2> replacementProgramNodes = {renderer.CreateProgramNode(), renderer.CreateProgramNode()};
			const std::array<size_t, 2> stripIndices = {1, 3};
			const std::array<V4Float32, 2> replacementColors = {V4Float32(0.0f, 1.0f, 1.0f, 1.0f), V4Float32(1.0f, 0.5f, 0.0f, 1.0f)};
			for (size_t i = 0; i < renderableNodes.size(); ++i)
			{
				renderableNodes[i] = renderer.CreateRenderableNode();
				REQUIRE(renderableNodes[i]->BindVertexAttribute(*stripAttributes[stripIndices[i]], positionAttributeId));
				REQUIRE(renderableNodes[i]->SetPrimitiveMode(IRenderableNode::PrimitiveMode::Triangles));
				renderableNodes[i]->SetStateValue(drawColorStateId, replacementColors[i]);

				stateGroupNodes[i] = renderer.CreateStateGroupNode();
				stateGroupNodes[i]->SetDepthTestEnabled(false);
				stateGroupNodes[i]->SetDepthWriteEnabled(false);
				stateGroupNodes[i]->AddChildNode(renderableNodes[i].get());

				REQUIRE(replacementProgramNodes[i]->BindShaderProgram(shaderProgram.get()));
				replacementProgramNodes[i]->AddChildNode(stateGroupNodes[i].get());
			}

			auto renderPassNode = renderer.CreateRenderPassNode();
			renderPassNode->BindFrameBuffer(frameBuffer.get());
			renderPassNode->SetAttachmentClearData(IFrameBuffer::AttachmentType::Color, 0, V4Float32(0.0f, 0.0f, 0.0f, 1.0f));
			renderPassNode->SetChildNodes(replacementProgramNodes.data(), replacementProgramNodes.size());
			renderer.SetRenderPasses(&renderPassNode, 1);

			auto frameBufferCapture = renderer.CreateFrameBufferOutput();
			frameBufferCapture->SetDetachAfterCapture(true);
			frameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
			DrawOneFrame();
			session.AddTestImageResult("ReplacedProgramChildren", "A cyan strip left of center and an orange strip right of center after replacing the render-pass program children.", std::move(frameBufferCapture), IImageDiff::Algorithm::NaiveDiff, 0.98);
			renderer.RemoveAllRenderPasses();
		}

		std::array<IRenderableNode::unique_ptr, 5> renderableNodes;
		std::array<IRenderableNode*, 5> rawRenderableNodes = {};
		for (size_t i = 0; i < renderableNodes.size(); ++i)
		{
			renderableNodes[i] = renderer.CreateRenderableNode();
			REQUIRE(renderableNodes[i]->BindVertexAttribute(*stripAttributes[i], positionAttributeId));
			REQUIRE(renderableNodes[i]->SetPrimitiveMode(IRenderableNode::PrimitiveMode::Triangles));
			renderableNodes[i]->SetStateValue(drawColorStateId, stripColors[i]);
			rawRenderableNodes[i] = renderableNodes[i].get();
		}

		auto stateGroupNode = renderer.CreateStateGroupNode();
		stateGroupNode->SetDepthTestEnabled(false);
		stateGroupNode->SetDepthWriteEnabled(false);
		stateGroupNode->SetChildNodes(rawRenderableNodes.data(), rawRenderableNodes.size());
		std::array<IRenderableNode*, 2> removedRenderableNodes = {renderableNodes[1].get(), renderableNodes[2].get()};
		stateGroupNode->RemoveChildNodes(removedRenderableNodes.data(), removedRenderableNodes.size());

		auto programNode = renderer.CreateProgramNode();
		REQUIRE(programNode->BindShaderProgram(shaderProgram.get()));
		programNode->AddChildNode(stateGroupNode.get());

		auto renderPassNode = renderer.CreateRenderPassNode();
		renderPassNode->BindFrameBuffer(frameBuffer.get());
		renderPassNode->SetAttachmentClearData(IFrameBuffer::AttachmentType::Color, 0, V4Float32(0.0f, 0.0f, 0.0f, 1.0f));
		renderPassNode->AddChildNode(programNode.get());
		renderer.SetRenderPasses(&renderPassNode, 1);

		auto frameBufferCapture = renderer.CreateFrameBufferOutput();
		frameBufferCapture->SetDetachAfterCapture(true);
		frameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
		DrawOneFrame();
		session.AddTestImageResult("RemovedMiddleRenderableChildren", "A red strip on the left, black gap in the middle, and yellow and magenta strips on the right after removing two middle renderable children.", std::move(frameBufferCapture), IImageDiff::Algorithm::NaiveDiff, 0.98);
		renderer.RemoveAllRenderPasses();
	}

	return true;
}

} // namespace cobalt::graphics::testing
