// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "../UnitTestBase.h"
namespace cobalt::graphics::testing {
using namespace cobalt::graphics;

namespace {

// Define our shader programs
const std::string PointExpansionVertexShader = R"(
struct VSInput
{
	float2 position : position;
	float4 color : color;
	float2 halfSize : halfSize;
};

struct VSOutput
{
	float4 position : SV_POSITION;
	float4 color : COLOR0;
	float2 halfSize : TEXCOORD0;
};

VSOutput main(VSInput IN)
{
	VSOutput OUT;
	OUT.position = float4(IN.position, 0.0f, 1.0f);
	OUT.color = IN.color;
	OUT.halfSize = IN.halfSize;
	return OUT;
}
)";
const std::string PointExpansionGeometryShader = R"(
uniform float2 geometryOffset;

struct VSOutput
{
	float4 position : SV_POSITION;
	float4 color : COLOR0;
	float2 halfSize : TEXCOORD0;
};

struct GSOutput
{
	float4 position : SV_POSITION;
	float4 color : COLOR0;
};

[maxvertexcount(4)]
void main(point VSOutput IN[1], inout TriangleStream<GSOutput> stream)
{
	float2 center = IN[0].position.xy + geometryOffset;
	float2 halfSize = IN[0].halfSize;

	GSOutput OUT;
	OUT.color = IN[0].color;

	OUT.position = float4(center + float2(-halfSize.x, -halfSize.y), 0.0f, 1.0f);
	stream.Append(OUT);
	OUT.position = float4(center + float2(-halfSize.x, halfSize.y), 0.0f, 1.0f);
	stream.Append(OUT);
	OUT.position = float4(center + float2(halfSize.x, -halfSize.y), 0.0f, 1.0f);
	stream.Append(OUT);
	OUT.position = float4(center + float2(halfSize.x, halfSize.y), 0.0f, 1.0f);
	stream.Append(OUT);
	stream.RestartStrip();
}
)";

const std::string LineExpansionVertexShader = R"(
struct VSInput
{
	float2 position : position;
	float4 color : color;
};

struct VSOutput
{
	float4 position : SV_POSITION;
	float4 color : COLOR0;
};

VSOutput main(VSInput IN)
{
	VSOutput OUT;
	OUT.position = float4(IN.position, 0.0f, 1.0f);
	OUT.color = IN.color;
	return OUT;
}
)";
const std::string LineExpansionGeometryShader = R"(
struct VSOutput
{
	float4 position : SV_POSITION;
	float4 color : COLOR0;
};

struct GSOutput
{
	float4 position : SV_POSITION;
	float4 color : COLOR0;
};

[maxvertexcount(4)]
void main(line VSOutput IN[2], inout TriangleStream<GSOutput> stream)
{
	float2 startPosition = IN[0].position.xy;
	float2 endPosition = IN[1].position.xy;
	float2 direction = normalize(endPosition - startPosition);
	float2 normal = float2(-direction.y, direction.x) * 0.12f;

	GSOutput OUT;
	OUT.color = IN[0].color;

	OUT.position = float4(startPosition - normal, 0.0f, 1.0f);
	stream.Append(OUT);
	OUT.position = float4(startPosition + normal, 0.0f, 1.0f);
	stream.Append(OUT);
	OUT.position = float4(endPosition - normal, 0.0f, 1.0f);
	stream.Append(OUT);
	OUT.position = float4(endPosition + normal, 0.0f, 1.0f);
	stream.Append(OUT);
	stream.RestartStrip();
}
)";
const std::string LineAdjacencyExpansionGeometryShader = R"(
struct VSOutput
{
	float4 position : SV_POSITION;
	float4 color : COLOR0;
};

struct GSOutput
{
	float4 position : SV_POSITION;
	float4 color : COLOR0;
};

[maxvertexcount(4)]
void main(lineadj VSOutput IN[4], inout TriangleStream<GSOutput> stream)
{
	float2 startPosition = IN[1].position.xy;
	float2 endPosition = IN[2].position.xy;
	float2 direction = normalize(endPosition - startPosition);
	float2 normal = float2(-direction.y, direction.x) * 0.14f;

	GSOutput OUT;
	OUT.color = saturate(IN[0].color + IN[3].color);

	OUT.position = float4(startPosition - normal, 0.0f, 1.0f);
	stream.Append(OUT);
	OUT.position = float4(startPosition + normal, 0.0f, 1.0f);
	stream.Append(OUT);
	OUT.position = float4(endPosition - normal, 0.0f, 1.0f);
	stream.Append(OUT);
	OUT.position = float4(endPosition + normal, 0.0f, 1.0f);
	stream.Append(OUT);
	stream.RestartStrip();
}
)";

const std::string TrianglePassthroughVertexShader = R"(
struct VSInput
{
	float2 position : position;
};

struct VSOutput
{
	float4 position : SV_POSITION;
};

VSOutput main(VSInput IN)
{
	VSOutput OUT;
	OUT.position = float4(IN.position, 0.0f, 1.0f);
	return OUT;
}
)";
const std::string TrianglePassthroughGeometryShader = R"(
uniform float4 triangleColor;

struct VSOutput
{
	float4 position : SV_POSITION;
};

struct GSOutput
{
	float4 position : SV_POSITION;
	float4 color : COLOR0;
};

[maxvertexcount(3)]
void main(triangle VSOutput IN[3], inout TriangleStream<GSOutput> stream)
{
	GSOutput OUT;
	OUT.color = triangleColor;

	OUT.position = IN[0].position;
	stream.Append(OUT);
	OUT.position = IN[1].position;
	stream.Append(OUT);
	OUT.position = IN[2].position;
	stream.Append(OUT);
	stream.RestartStrip();
}
)";

const std::string GeometryColorFragmentShader = R"(
struct GSOutput
{
	float4 position : SV_POSITION;
	float4 color : COLOR0;
};

float4 main(GSOutput IN) : SV_TARGET0
{
	return IN.color;
}
)";

const std::string PointStarVertexShader = R"(
struct VSInput
{
	float2 position : position;
	float4 color : color;
	float2 radius : radius;
};

struct VSOutput
{
	float4 position : SV_POSITION;
	float4 color : COLOR0;
	float2 radius : TEXCOORD0;
};

VSOutput main(VSInput IN)
{
	VSOutput OUT;
	OUT.position = float4(IN.position, 0.0f, 1.0f);
	OUT.color = IN.color;
	OUT.radius = IN.radius;
	return OUT;
}
)";
const std::string PointStarGeometryShader = R"(
struct VSOutput
{
	float4 position : SV_POSITION;
	float4 color : COLOR0;
	float2 radius : TEXCOORD0;
};

struct GSOutput
{
	float4 position : SV_POSITION;
	float4 color : COLOR0;
};

[maxvertexcount(18)]
void main(point VSOutput IN[1], inout TriangleStream<GSOutput> stream)
{
	float2 center = IN[0].position.xy;
	float outerRadius = IN[0].radius.x;
	float innerRadius = IN[0].radius.y;

	GSOutput OUT;
	OUT.color = IN[0].color;

	for (int i = 0; i < 6; ++i)
	{
		float angle = (float)i * 1.0471975512f;
		float2 direction = float2(cos(angle), sin(angle));
		float2 side = float2(-direction.y, direction.x);

		OUT.position = float4(center + direction * outerRadius, 0.0f, 1.0f);
		stream.Append(OUT);
		OUT.position = float4(center + side * innerRadius, 0.0f, 1.0f);
		stream.Append(OUT);
		OUT.position = float4(center - side * innerRadius, 0.0f, 1.0f);
		stream.Append(OUT);
		stream.RestartStrip();
	}
}
)";

} // namespace

DEFINE_UNIT_TEST_WITH_BASE("Shader/GeometryShaders", UnitTestBase)
{
	if (!session.Device().IsFeatureSupported(IGraphicsDevice::Feature::GeometryShaders))
	{
		session.AddTestSkipped("GeometryShaderRendering", "This part of the test was skipped, as the current renderer doesn't support geometry shaders on this device.");
		return true;
	}

	// Retrieve our important objects
	auto& renderer = session.Renderer();
	auto& uiThread = session.UIThread();
	auto& testWindowInfo = *session.TestWindowPlatformInfo();

	// Define the framebuffer
	auto frameBuffer = renderer.CreateFrameBuffer();
	frameBuffer->DefineViewportRegion(V2UInt32(0, 0), session.TestWindowSize());
	REQUIRE(uiThread.InvokeSync([&] { return frameBuffer->BindWindow(testWindowInfo, IFrameBuffer::WindowDepthStencilMode::None); }));

	// Point input: expand each source vertex into a quad, then move the generated output.
	{
		// Create and compile our shader program
		auto shaderProgram = renderer.CreateShaderProgram();
		REQUIRE(shaderProgram->LoadShaderStage(IShaderProgram::ShaderStage::Vertex, ShaderSourceInfoHLSL(PointExpansionVertexShader)));
		REQUIRE(shaderProgram->LoadShaderStage(IShaderProgram::ShaderStage::Geometry, ShaderSourceInfoHLSL(PointExpansionGeometryShader)));
		REQUIRE(shaderProgram->LoadShaderStage(IShaderProgram::ShaderStage::Fragment, ShaderSourceInfoHLSL(GeometryColorFragmentShader)));
		REQUIRE(shaderProgram->CompileProgram());

		// Create our renderable node
		auto renderableNode = renderer.CreateRenderableNode();
		REQUIRE(renderableNode->SetPrimitiveMode(IRenderableNode::PrimitiveMode::Points));

		// Create our vertex data
		std::vector<V2Float32> positions =
		  {
		    V2Float32(-0.50f, 0.0f),
		    V2Float32(0.00f, 0.0f),
		    V2Float32(0.50f, 0.0f),
		  };
		std::vector<V4Float32> colors =
		  {
		    V4Float32(1.0f, 0.0f, 0.0f, 1.0f),
		    V4Float32(0.0f, 1.0f, 0.0f, 1.0f),
		    V4Float32(0.0f, 0.0f, 1.0f, 1.0f),
		  };
		std::vector<V2Float32> halfSizes =
		  {
		    V2Float32(0.16f, 0.24f),
		    V2Float32(0.16f, 0.24f),
		    V2Float32(0.16f, 0.24f),
		  };

		// Create our vertex buffer and populate it with data
		VertexAttribute<V2Float32> positionAttribute(positions.size(), IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadNever, IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadOften);
		VertexAttribute<V4Float32> colorAttribute(colors.size(), IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadNever, IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadOften);
		VertexAttribute<V2Float32> halfSizeAttribute(halfSizes.size(), IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadNever, IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadOften);
		auto vertexBuffer = renderer.CreateVertexBuffer();
		REQUIRE(vertexBuffer->BindVertexAttribute(positionAttribute));
		REQUIRE(vertexBuffer->BindVertexAttribute(colorAttribute));
		REQUIRE(vertexBuffer->BindVertexAttribute(halfSizeAttribute));
		REQUIRE(positionAttribute.SetInitialData(positions));
		REQUIRE(colorAttribute.SetInitialData(colors));
		REQUIRE(halfSizeAttribute.SetInitialData(halfSizes));
		REQUIRE(vertexBuffer->AllocateMemory());
		REQUIRE(renderableNode->BindVertexAttribute(positionAttribute, shaderProgram->GetVertexAttributeId("position")));
		REQUIRE(renderableNode->BindVertexAttribute(colorAttribute, shaderProgram->GetVertexAttributeId("color")));
		REQUIRE(renderableNode->BindVertexAttribute(halfSizeAttribute, shaderProgram->GetVertexAttributeId("halfSize")));

		// Bind our geometry shader state value
		const auto geometryOffsetId = shaderProgram->GetStateValueId("geometryOffset");
		REQUIRE(geometryOffsetId != StateValueId::Null);
		renderableNode->SetStateValue(geometryOffsetId, V2Float32(0.0f, 0.0f));

		// Create our render tree
		auto groupNode = renderer.CreateStateGroupNode();
		groupNode->SetPolygonCullMode(IStateGroupNode::PolygonCullMode::None);
		groupNode->AddChildNode(renderableNode.get());

		auto programNode = renderer.CreateProgramNode();
		REQUIRE(programNode->BindShaderProgram(shaderProgram.get()));
		programNode->AddChildNode(groupNode.get());

		auto renderPassNode = renderer.CreateRenderPassNode();
		renderPassNode->BindFrameBuffer(frameBuffer.get());
		renderPassNode->SetAttachmentClearData(IFrameBuffer::AttachmentType::Color, 0, V4Float32(0.0f, 0.0f, 0.0f, 1.0f));
		renderPassNode->AddChildNode(programNode.get());
		renderer.SetRenderPasses(&renderPassNode, 1);

		// Capture an image of the original point expansion output
		auto originalFrameBufferCapture = renderer.CreateFrameBufferOutput();
		originalFrameBufferCapture->SetDetachAfterCapture(true);
		frameBuffer->AddOutputCaptureTarget(originalFrameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
		DrawOneFrame();
		session.AddTestImageResult("PointExpansion", "Red, green and blue quads generated from point primitives by a geometry shader.", std::move(originalFrameBufferCapture), IImageDiff::Algorithm::NaiveDiff, 0.95);

		// Capture the output again after changing a geometry shader state value
		renderableNode->SetStateValue(geometryOffsetId, V2Float32(0.25f, 0.0f));
		auto shiftedFrameBufferCapture = renderer.CreateFrameBufferOutput();
		shiftedFrameBufferCapture->SetDetachAfterCapture(true);
		frameBuffer->AddOutputCaptureTarget(shiftedFrameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
		DrawOneFrame();
		session.AddTestImageResult("GeometryShaderStateValues", "The red, green and blue generated quads shifted right by a geometry-stage state value.", std::move(shiftedFrameBufferCapture), IImageDiff::Algorithm::NaiveDiff, 0.95);

		// Remove all our defined render passes
		renderer.RemoveAllRenderPasses();
	}

	// Line input: expand a two-vertex primitive into a visible strip.
	{
		// Create and compile our shader program
		auto shaderProgram = renderer.CreateShaderProgram();
		REQUIRE(shaderProgram->LoadShaderStage(IShaderProgram::ShaderStage::Vertex, ShaderSourceInfoHLSL(LineExpansionVertexShader)));
		REQUIRE(shaderProgram->LoadShaderStage(IShaderProgram::ShaderStage::Geometry, ShaderSourceInfoHLSL(LineExpansionGeometryShader)));
		REQUIRE(shaderProgram->LoadShaderStage(IShaderProgram::ShaderStage::Fragment, ShaderSourceInfoHLSL(GeometryColorFragmentShader)));
		REQUIRE(shaderProgram->CompileProgram());

		// Create our renderable node
		auto renderableNode = renderer.CreateRenderableNode();
		REQUIRE(renderableNode->SetPrimitiveMode(IRenderableNode::PrimitiveMode::Lines));

		// Create our vertex data
		std::vector<V2Float32> positions =
		  {
		    V2Float32(-0.70f, 0.0f),
		    V2Float32(0.70f, 0.0f),
		  };
		std::vector<V4Float32> colors =
		  {
		    V4Float32(1.0f, 1.0f, 0.0f, 1.0f),
		    V4Float32(1.0f, 1.0f, 0.0f, 1.0f),
		  };

		// Create our vertex buffer and populate it with data
		VertexAttribute<V2Float32> positionAttribute(positions.size(), IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadNever, IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadOften);
		VertexAttribute<V4Float32> colorAttribute(colors.size(), IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadNever, IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadOften);
		auto vertexBuffer = renderer.CreateVertexBuffer();
		REQUIRE(vertexBuffer->BindVertexAttribute(positionAttribute));
		REQUIRE(vertexBuffer->BindVertexAttribute(colorAttribute));
		REQUIRE(positionAttribute.SetInitialData(positions));
		REQUIRE(colorAttribute.SetInitialData(colors));
		REQUIRE(vertexBuffer->AllocateMemory());
		REQUIRE(renderableNode->BindVertexAttribute(positionAttribute, shaderProgram->GetVertexAttributeId("position")));
		REQUIRE(renderableNode->BindVertexAttribute(colorAttribute, shaderProgram->GetVertexAttributeId("color")));

		// Create our render tree
		auto groupNode = renderer.CreateStateGroupNode();
		groupNode->SetPolygonCullMode(IStateGroupNode::PolygonCullMode::None);
		groupNode->AddChildNode(renderableNode.get());

		auto programNode = renderer.CreateProgramNode();
		REQUIRE(programNode->BindShaderProgram(shaderProgram.get()));
		programNode->AddChildNode(groupNode.get());

		auto renderPassNode = renderer.CreateRenderPassNode();
		renderPassNode->BindFrameBuffer(frameBuffer.get());
		renderPassNode->SetAttachmentClearData(IFrameBuffer::AttachmentType::Color, 0, V4Float32(0.0f, 0.0f, 0.0f, 1.0f));
		renderPassNode->AddChildNode(programNode.get());
		renderer.SetRenderPasses(&renderPassNode, 1);

		// Capture an image of the generated line strip
		auto frameBufferCapture = renderer.CreateFrameBufferOutput();
		frameBufferCapture->SetDetachAfterCapture(true);
		frameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
		DrawOneFrame();
		session.AddTestImageResult("LineExpansion", "A yellow strip generated from a line primitive by a geometry shader.", std::move(frameBufferCapture), IImageDiff::Algorithm::NaiveDiff, 0.95);

		// Remove all our defined render passes
		renderer.RemoveAllRenderPasses();
	}

	// Line adjacency input: read adjacency vertices while generating the output strip.
	{
		// Create and compile our shader program
		auto shaderProgram = renderer.CreateShaderProgram();
		REQUIRE(shaderProgram->LoadShaderStage(IShaderProgram::ShaderStage::Vertex, ShaderSourceInfoHLSL(LineExpansionVertexShader)));
		REQUIRE(shaderProgram->LoadShaderStage(IShaderProgram::ShaderStage::Geometry, ShaderSourceInfoHLSL(LineAdjacencyExpansionGeometryShader)));
		REQUIRE(shaderProgram->LoadShaderStage(IShaderProgram::ShaderStage::Fragment, ShaderSourceInfoHLSL(GeometryColorFragmentShader)));
		REQUIRE(shaderProgram->CompileProgram());

		// Create our renderable node
		auto renderableNode = renderer.CreateRenderableNode();
		REQUIRE(renderableNode->SetPrimitiveMode(IRenderableNode::PrimitiveMode::Lines, false, true));

		// Create our vertex data
		std::vector<V2Float32> positions =
		  {
		    V2Float32(-0.90f, 0.0f),
		    V2Float32(-0.45f, 0.0f),
		    V2Float32(0.45f, 0.0f),
		    V2Float32(0.90f, 0.0f),
		  };
		std::vector<V4Float32> colors =
		  {
		    V4Float32(1.0f, 0.0f, 0.0f, 0.0f),
		    V4Float32(0.0f, 0.0f, 0.0f, 0.0f),
		    V4Float32(0.0f, 0.0f, 0.0f, 0.0f),
		    V4Float32(0.0f, 0.0f, 1.0f, 1.0f),
		  };

		// Create our vertex buffer and populate it with data
		VertexAttribute<V2Float32> positionAttribute(positions.size(), IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadNever, IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadOften);
		VertexAttribute<V4Float32> colorAttribute(colors.size(), IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadNever, IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadOften);
		auto vertexBuffer = renderer.CreateVertexBuffer();
		REQUIRE(vertexBuffer->BindVertexAttribute(positionAttribute));
		REQUIRE(vertexBuffer->BindVertexAttribute(colorAttribute));
		REQUIRE(positionAttribute.SetInitialData(positions));
		REQUIRE(colorAttribute.SetInitialData(colors));
		REQUIRE(vertexBuffer->AllocateMemory());
		REQUIRE(renderableNode->BindVertexAttribute(positionAttribute, shaderProgram->GetVertexAttributeId("position")));
		REQUIRE(renderableNode->BindVertexAttribute(colorAttribute, shaderProgram->GetVertexAttributeId("color")));

		// Create our render tree
		auto groupNode = renderer.CreateStateGroupNode();
		groupNode->SetPolygonCullMode(IStateGroupNode::PolygonCullMode::None);
		groupNode->AddChildNode(renderableNode.get());

		auto programNode = renderer.CreateProgramNode();
		REQUIRE(programNode->BindShaderProgram(shaderProgram.get()));
		programNode->AddChildNode(groupNode.get());

		auto renderPassNode = renderer.CreateRenderPassNode();
		renderPassNode->BindFrameBuffer(frameBuffer.get());
		renderPassNode->SetAttachmentClearData(IFrameBuffer::AttachmentType::Color, 0, V4Float32(0.0f, 0.0f, 0.0f, 1.0f));
		renderPassNode->AddChildNode(programNode.get());
		renderer.SetRenderPasses(&renderPassNode, 1);

		// Capture an image of the generated adjacency strip
		auto frameBufferCapture = renderer.CreateFrameBufferOutput();
		frameBufferCapture->SetDetachAfterCapture(true);
		frameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
		DrawOneFrame();
		session.AddTestImageResult("LineAdjacencyExpansion", "A magenta strip generated from a line-adjacency primitive by a geometry shader.", std::move(frameBufferCapture), IImageDiff::Algorithm::NaiveDiff, 0.95);

		// Remove all our defined render passes
		renderer.RemoveAllRenderPasses();
	}

	// Triangle input: pass through source geometry while producing fragment data in the geometry stage.
	{
		// Create and compile our shader program
		auto shaderProgram = renderer.CreateShaderProgram();
		REQUIRE(shaderProgram->LoadShaderStage(IShaderProgram::ShaderStage::Vertex, ShaderSourceInfoHLSL(TrianglePassthroughVertexShader)));
		REQUIRE(shaderProgram->LoadShaderStage(IShaderProgram::ShaderStage::Geometry, ShaderSourceInfoHLSL(TrianglePassthroughGeometryShader)));
		REQUIRE(shaderProgram->LoadShaderStage(IShaderProgram::ShaderStage::Fragment, ShaderSourceInfoHLSL(GeometryColorFragmentShader)));
		REQUIRE(shaderProgram->CompileProgram());

		// Create our renderable node
		auto renderableNode = renderer.CreateRenderableNode();
		REQUIRE(renderableNode->SetPrimitiveMode(IRenderableNode::PrimitiveMode::Triangles));

		// Create our vertex data
		std::vector<V2Float32> positions =
		  {
		    V2Float32(-0.45f, 0.40f),
		    V2Float32(0.00f, -0.45f),
		    V2Float32(0.45f, 0.40f),
		  };

		// Create our vertex buffer and populate it with data
		VertexAttribute<V2Float32> positionAttribute(positions.size(), IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadNever, IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadOften);
		auto vertexBuffer = renderer.CreateVertexBuffer();
		REQUIRE(vertexBuffer->BindVertexAttribute(positionAttribute));
		REQUIRE(positionAttribute.SetInitialData(positions));
		REQUIRE(vertexBuffer->AllocateMemory());
		REQUIRE(renderableNode->BindVertexAttribute(positionAttribute, shaderProgram->GetVertexAttributeId("position")));

		// Bind our geometry shader state value
		const auto triangleColorId = shaderProgram->GetStateValueId("triangleColor");
		REQUIRE(triangleColorId != StateValueId::Null);
		renderableNode->SetStateValue(triangleColorId, V4Float32(0.0f, 1.0f, 1.0f, 1.0f));

		// Create our render tree
		auto groupNode = renderer.CreateStateGroupNode();
		groupNode->SetPolygonCullMode(IStateGroupNode::PolygonCullMode::None);
		groupNode->AddChildNode(renderableNode.get());

		auto programNode = renderer.CreateProgramNode();
		REQUIRE(programNode->BindShaderProgram(shaderProgram.get()));
		programNode->AddChildNode(groupNode.get());

		auto renderPassNode = renderer.CreateRenderPassNode();
		renderPassNode->BindFrameBuffer(frameBuffer.get());
		renderPassNode->SetAttachmentClearData(IFrameBuffer::AttachmentType::Color, 0, V4Float32(0.0f, 0.0f, 0.0f, 1.0f));
		renderPassNode->AddChildNode(programNode.get());
		renderer.SetRenderPasses(&renderPassNode, 1);

		// Capture an image of the triangle passthrough output
		auto frameBufferCapture = renderer.CreateFrameBufferOutput();
		frameBufferCapture->SetDetachAfterCapture(true);
		frameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
		DrawOneFrame();
		session.AddTestImageResult("TrianglePassthrough", "A cyan triangle passed through by a geometry shader with geometry-stage fragment data.", std::move(frameBufferCapture), IImageDiff::Algorithm::NaiveDiff, 0.95);

		// Remove all our defined render passes
		renderer.RemoveAllRenderPasses();
	}

	// Point image: generate a ring of starbursts entirely from point primitives.
	{
		// Create and compile our shader program
		auto shaderProgram = renderer.CreateShaderProgram();
		REQUIRE(shaderProgram->LoadShaderStage(IShaderProgram::ShaderStage::Vertex, ShaderSourceInfoHLSL(PointStarVertexShader)));
		REQUIRE(shaderProgram->LoadShaderStage(IShaderProgram::ShaderStage::Geometry, ShaderSourceInfoHLSL(PointStarGeometryShader)));
		REQUIRE(shaderProgram->LoadShaderStage(IShaderProgram::ShaderStage::Fragment, ShaderSourceInfoHLSL(GeometryColorFragmentShader)));
		REQUIRE(shaderProgram->CompileProgram());

		// Create our renderable node
		auto renderableNode = renderer.CreateRenderableNode();
		REQUIRE(renderableNode->SetPrimitiveMode(IRenderableNode::PrimitiveMode::Points));

		// Create our vertex data
		std::vector<V2Float32> positions =
		  {
		    V2Float32(0.00f, 0.00f),
		    V2Float32(0.00f, 0.62f),
		    V2Float32(0.44f, 0.44f),
		    V2Float32(0.62f, 0.00f),
		    V2Float32(0.44f, -0.44f),
		    V2Float32(0.00f, -0.62f),
		    V2Float32(-0.44f, -0.44f),
		    V2Float32(-0.62f, 0.00f),
		    V2Float32(-0.44f, 0.44f),
		  };
		std::vector<V4Float32> colors =
		  {
		    V4Float32(1.0f, 1.0f, 1.0f, 1.0f),
		    V4Float32(1.0f, 0.0f, 0.0f, 1.0f),
		    V4Float32(1.0f, 0.5f, 0.0f, 1.0f),
		    V4Float32(1.0f, 1.0f, 0.0f, 1.0f),
		    V4Float32(0.0f, 1.0f, 0.0f, 1.0f),
		    V4Float32(0.0f, 1.0f, 1.0f, 1.0f),
		    V4Float32(0.0f, 0.0f, 1.0f, 1.0f),
		    V4Float32(0.5f, 0.0f, 1.0f, 1.0f),
		    V4Float32(1.0f, 0.0f, 1.0f, 1.0f),
		  };
		std::vector<V2Float32> radii =
		  {
		    V2Float32(0.24f, 0.06f),
		    V2Float32(0.18f, 0.04f),
		    V2Float32(0.18f, 0.04f),
		    V2Float32(0.18f, 0.04f),
		    V2Float32(0.18f, 0.04f),
		    V2Float32(0.18f, 0.04f),
		    V2Float32(0.18f, 0.04f),
		    V2Float32(0.18f, 0.04f),
		    V2Float32(0.18f, 0.04f),
		  };

		// Create our vertex buffer and populate it with data
		VertexAttribute<V2Float32> positionAttribute(positions.size(), IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadNever, IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadOften);
		VertexAttribute<V4Float32> colorAttribute(colors.size(), IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadNever, IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadOften);
		VertexAttribute<V2Float32> radiusAttribute(radii.size(), IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadNever, IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadOften);
		auto vertexBuffer = renderer.CreateVertexBuffer();
		REQUIRE(vertexBuffer->BindVertexAttribute(positionAttribute));
		REQUIRE(vertexBuffer->BindVertexAttribute(colorAttribute));
		REQUIRE(vertexBuffer->BindVertexAttribute(radiusAttribute));
		REQUIRE(positionAttribute.SetInitialData(positions));
		REQUIRE(colorAttribute.SetInitialData(colors));
		REQUIRE(radiusAttribute.SetInitialData(radii));
		REQUIRE(vertexBuffer->AllocateMemory());
		REQUIRE(renderableNode->BindVertexAttribute(positionAttribute, shaderProgram->GetVertexAttributeId("position")));
		REQUIRE(renderableNode->BindVertexAttribute(colorAttribute, shaderProgram->GetVertexAttributeId("color")));
		REQUIRE(renderableNode->BindVertexAttribute(radiusAttribute, shaderProgram->GetVertexAttributeId("radius")));

		// Create our render tree
		auto groupNode = renderer.CreateStateGroupNode();
		groupNode->SetPolygonCullMode(IStateGroupNode::PolygonCullMode::None);
		groupNode->AddChildNode(renderableNode.get());

		auto programNode = renderer.CreateProgramNode();
		REQUIRE(programNode->BindShaderProgram(shaderProgram.get()));
		programNode->AddChildNode(groupNode.get());

		auto renderPassNode = renderer.CreateRenderPassNode();
		renderPassNode->BindFrameBuffer(frameBuffer.get());
		renderPassNode->SetAttachmentClearData(IFrameBuffer::AttachmentType::Color, 0, V4Float32(0.02f, 0.02f, 0.05f, 1.0f));
		renderPassNode->AddChildNode(programNode.get());
		renderer.SetRenderPasses(&renderPassNode, 1);

		// Capture an image of the generated starburst scene
		auto frameBufferCapture = renderer.CreateFrameBufferOutput();
		frameBufferCapture->SetDetachAfterCapture(true);
		frameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
		DrawOneFrame();
		session.AddTestImageResult("PointGeneratedStarbursts", "A ring of colored starbursts generated from point primitives by a geometry shader.", std::move(frameBufferCapture), IImageDiff::Algorithm::NaiveDiff, 0.95);

		// Remove all our defined render passes
		renderer.RemoveAllRenderPasses();
	}

	// Line adjacency image: generate several thick ribbons from adjacency-aware line primitives.
	{
		// Create and compile our shader program
		auto shaderProgram = renderer.CreateShaderProgram();
		REQUIRE(shaderProgram->LoadShaderStage(IShaderProgram::ShaderStage::Vertex, ShaderSourceInfoHLSL(LineExpansionVertexShader)));
		REQUIRE(shaderProgram->LoadShaderStage(IShaderProgram::ShaderStage::Geometry, ShaderSourceInfoHLSL(LineAdjacencyExpansionGeometryShader)));
		REQUIRE(shaderProgram->LoadShaderStage(IShaderProgram::ShaderStage::Fragment, ShaderSourceInfoHLSL(GeometryColorFragmentShader)));
		REQUIRE(shaderProgram->CompileProgram());

		// Create our renderable node
		auto renderableNode = renderer.CreateRenderableNode();
		REQUIRE(renderableNode->SetPrimitiveMode(IRenderableNode::PrimitiveMode::Lines, false, true));

		// Create our vertex data
		std::vector<V2Float32> positions =
		  {
		    V2Float32(-0.95f, -0.55f),
		    V2Float32(-0.78f, -0.32f),
		    V2Float32(-0.28f, 0.28f),
		    V2Float32(-0.08f, 0.55f),
		    V2Float32(-0.40f, 0.60f),
		    V2Float32(-0.18f, 0.34f),
		    V2Float32(0.35f, -0.28f),
		    V2Float32(0.60f, -0.58f),
		    V2Float32(0.02f, -0.58f),
		    V2Float32(0.26f, -0.28f),
		    V2Float32(0.76f, 0.30f),
		    V2Float32(0.96f, 0.58f),
		  };
		std::vector<V4Float32> colors =
		  {
		    V4Float32(1.0f, 0.0f, 0.0f, 0.0f),
		    V4Float32(0.0f, 0.0f, 0.0f, 0.0f),
		    V4Float32(0.0f, 0.0f, 0.0f, 0.0f),
		    V4Float32(0.0f, 0.0f, 1.0f, 1.0f),
		    V4Float32(0.0f, 1.0f, 0.0f, 0.0f),
		    V4Float32(0.0f, 0.0f, 0.0f, 0.0f),
		    V4Float32(0.0f, 0.0f, 0.0f, 0.0f),
		    V4Float32(0.0f, 0.0f, 1.0f, 1.0f),
		    V4Float32(1.0f, 0.0f, 0.0f, 0.0f),
		    V4Float32(0.0f, 0.0f, 0.0f, 0.0f),
		    V4Float32(0.0f, 0.0f, 0.0f, 0.0f),
		    V4Float32(0.0f, 1.0f, 0.0f, 1.0f),
		  };

		// Create our vertex buffer and populate it with data
		VertexAttribute<V2Float32> positionAttribute(positions.size(), IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadNever, IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadOften);
		VertexAttribute<V4Float32> colorAttribute(colors.size(), IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadNever, IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadOften);
		auto vertexBuffer = renderer.CreateVertexBuffer();
		REQUIRE(vertexBuffer->BindVertexAttribute(positionAttribute));
		REQUIRE(vertexBuffer->BindVertexAttribute(colorAttribute));
		REQUIRE(positionAttribute.SetInitialData(positions));
		REQUIRE(colorAttribute.SetInitialData(colors));
		REQUIRE(vertexBuffer->AllocateMemory());
		REQUIRE(renderableNode->BindVertexAttribute(positionAttribute, shaderProgram->GetVertexAttributeId("position")));
		REQUIRE(renderableNode->BindVertexAttribute(colorAttribute, shaderProgram->GetVertexAttributeId("color")));

		// Create our render tree
		auto groupNode = renderer.CreateStateGroupNode();
		groupNode->SetPolygonCullMode(IStateGroupNode::PolygonCullMode::None);
		groupNode->AddChildNode(renderableNode.get());

		auto programNode = renderer.CreateProgramNode();
		REQUIRE(programNode->BindShaderProgram(shaderProgram.get()));
		programNode->AddChildNode(groupNode.get());

		auto renderPassNode = renderer.CreateRenderPassNode();
		renderPassNode->BindFrameBuffer(frameBuffer.get());
		renderPassNode->SetAttachmentClearData(IFrameBuffer::AttachmentType::Color, 0, V4Float32(0.02f, 0.02f, 0.05f, 1.0f));
		renderPassNode->AddChildNode(programNode.get());
		renderer.SetRenderPasses(&renderPassNode, 1);

		// Capture an image of the generated ribbon scene
		auto frameBufferCapture = renderer.CreateFrameBufferOutput();
		frameBufferCapture->SetDetachAfterCapture(true);
		frameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
		DrawOneFrame();
		session.AddTestImageResult("LineAdjacencyGeneratedRibbons", "Magenta, cyan and yellow ribbons generated from line-adjacency primitives by a geometry shader.", std::move(frameBufferCapture), IImageDiff::Algorithm::NaiveDiff, 0.95);

		// Remove all our defined render passes
		renderer.RemoveAllRenderPasses();
	}

	return true;
}

} // namespace cobalt::graphics::testing
