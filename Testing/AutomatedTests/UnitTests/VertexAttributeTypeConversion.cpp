// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "../GeometryHelper.h"
#include "../UnitTestBase.h"
#include <Cobalt/Debug/Debug.pkg>
WARNINGS_PUSH_OFF
#ifdef _MSC_VER
#pragma warning(disable : 26454)
#endif
#include <half.hpp>
WARNINGS_POP
#include <cmath>
#include <limits>
namespace cobalt::graphics::testing {
using namespace cobalt::graphics;

// Define our shader programs
const std::string VertexShader = R"(
struct VSInput {
	float3 position : position;
	float3 colorFloat : colorFloat;
	int3 colorInt : colorInt;
	uint3 colorUInt : colorUInt;
};

struct VSOutput {
	float4 position : SV_POSITION;
	nointerpolation float3 color : color;
};

uniform bool useColorFloat = false;
uniform bool useColorFloatFromInt8 = false;
uniform bool useColorFloatFromInt16 = false;
uniform bool useColorFloatFromUInt8 = false;
uniform bool useColorFloatFromUInt16 = false;
uniform bool useColorFloatFromUIntA2B10G10R10 = false;
uniform bool useColorIntFromInt8 = false;
uniform bool useColorIntFromInt16 = false;
uniform bool useColorIntFromUInt8 = false;
uniform bool useColorIntFromUInt16 = false;
uniform bool useColorUIntFromInt8 = false;
uniform bool useColorUIntFromInt16 = false;
uniform bool useColorUIntFromUInt8 = false;
uniform bool useColorUIntFromUInt16 = false;
uniform bool useColorUIntFromUIntA2B10G10R10 = false;

VSOutput main(VSInput IN)
{
	VSOutput OUT;

	OUT.position = float4(IN.position, 1.0);
	if (useColorFloat)
	{
		OUT.color = IN.colorFloat;
	}
	else if (useColorFloatFromInt8)
	{
		OUT.color.x = IN.colorFloat.x / 127.0f;
		OUT.color.y = IN.colorFloat.y / 127.0f;
		OUT.color.z = IN.colorFloat.z / 127.0f;
	}
	else if (useColorFloatFromInt16)
	{
		OUT.color.x = IN.colorFloat.x / 32767.0f;
		OUT.color.y = IN.colorFloat.y / 32767.0f;
		OUT.color.z = IN.colorFloat.z / 32767.0f;
	}
	else if (useColorFloatFromUInt8)
	{
		OUT.color.x = IN.colorFloat.x / 255.0f;
		OUT.color.y = IN.colorFloat.y / 255.0f;
		OUT.color.z = IN.colorFloat.z / 255.0f;
	}
	else if (useColorFloatFromUInt16)
	{
		OUT.color.x = IN.colorFloat.x / 65535.0f;
		OUT.color.y = IN.colorFloat.y / 65535.0f;
		OUT.color.z = IN.colorFloat.z / 65535.0f;
	}
	else if (useColorFloatFromUIntA2B10G10R10)
	{
		OUT.color.x = IN.colorFloat.x / 1023.0f;
		OUT.color.y = IN.colorFloat.y / 1023.0f;
		OUT.color.z = IN.colorFloat.z / 1023.0f;
	}
	else if (useColorIntFromInt8)
	{
		OUT.color.x = float(IN.colorInt.x) / 127.0f;
		OUT.color.y = float(IN.colorInt.y) / 127.0f;
		OUT.color.z = float(IN.colorInt.z) / 127.0f;
	}
	else if (useColorIntFromInt16)
	{
		OUT.color.x = float(IN.colorInt.x) / 32767.0f;
		OUT.color.y = float(IN.colorInt.y) / 32767.0f;
		OUT.color.z = float(IN.colorInt.z) / 32767.0f;
	}
	else if (useColorIntFromUInt8)
	{
		OUT.color.x = float(IN.colorInt.x) / 255.0f;
		OUT.color.y = float(IN.colorInt.y) / 255.0f;
		OUT.color.z = float(IN.colorInt.z) / 255.0f;
	}
	else if (useColorIntFromUInt16)
	{
		OUT.color.x = float(IN.colorInt.x) / 65535.0f;
		OUT.color.y = float(IN.colorInt.y) / 65535.0f;
		OUT.color.z = float(IN.colorInt.z) / 65535.0f;
	}
	else if (useColorUIntFromInt8)
	{
		OUT.color.x = float(IN.colorUInt.x) / 127.0f;
		OUT.color.y = float(IN.colorUInt.y) / 127.0f;
		OUT.color.z = float(IN.colorUInt.z) / 127.0f;
	}
	else if (useColorUIntFromInt16)
	{
		OUT.color.x = float(IN.colorUInt.x) / 32767.0f;
		OUT.color.y = float(IN.colorUInt.y) / 32767.0f;
		OUT.color.z = float(IN.colorUInt.z) / 32767.0f;
	}
	else if (useColorUIntFromUInt8)
	{
		OUT.color.x = float(IN.colorUInt.x) / 255.0f;
		OUT.color.y = float(IN.colorUInt.y) / 255.0f;
		OUT.color.z = float(IN.colorUInt.z) / 255.0f;
	}
	else if (useColorUIntFromUInt16)
	{
		OUT.color.x = float(IN.colorUInt.x) / 65535.0f;
		OUT.color.y = float(IN.colorUInt.y) / 65535.0f;
		OUT.color.z = float(IN.colorUInt.z) / 65535.0f;
	}
	else if (useColorUIntFromUIntA2B10G10R10)
	{
		OUT.color.x = float(IN.colorUInt.x) / 1023.0f;
		OUT.color.y = float(IN.colorUInt.y) / 1023.0f;
		OUT.color.z = float(IN.colorUInt.z) / 1023.0f;
	}

	return OUT;
}
)";
const std::string FragmentShader = R"(
struct VSOutput {
	float4 position : SV_POSITION;
	nointerpolation float3 color : color;
};

float4 main(VSOutput IN) : SV_TARGET0
{
	return float4(IN.color, 1.0);
}
)";

DEFINE_UNIT_TEST_WITH_BASE("Resources/VertexBuffer/VertexAttributeTypeConversion", UnitTestBase)
{
	// Retrieve our important objects
	auto& renderer = session.Renderer();
	auto& uiThread = session.UIThread();
	auto& testWindowPlatformInfo = *session.TestWindowPlatformInfo();

	// Define the framebuffer
	auto testWindowFrameBuffer = renderer.CreateFrameBuffer();
	testWindowFrameBuffer->DefineViewportRegion(V2UInt32(0, 0), session.TestWindowSize());
	REQUIRE(uiThread.InvokeSync([&] { return testWindowFrameBuffer->BindWindow(testWindowPlatformInfo, IFrameBuffer::WindowDepthStencilMode::DepthUNorm24); }));

	// Create and compile our shader program
	auto shaderProgram = renderer.CreateShaderProgram();
	REQUIRE(shaderProgram->LoadShaderStage(IShaderProgram::ShaderStage::Vertex, ShaderSourceInfoHLSL(VertexShader)));
	REQUIRE(shaderProgram->LoadShaderStage(IShaderProgram::ShaderStage::Fragment, ShaderSourceInfoHLSL(FragmentShader)));
	REQUIRE(shaderProgram->CompileProgram());

	// Retrieve our shader attribute IDs
	auto positionAttributeId = shaderProgram->GetVertexAttributeId("position");
	auto colorFloatAttributeId = shaderProgram->GetVertexAttributeId("colorFloat");
	auto colorIntAttributeId = shaderProgram->GetVertexAttributeId("colorInt");
	auto colorUIntAttributeId = shaderProgram->GetVertexAttributeId("colorUInt");

	// Retrieve our shader state IDs
	auto useColorFloatStateId = shaderProgram->GetStateValueId("useColorFloat");
	//auto useColorFloatFromInt8StateId = shaderProgram->GetStateValueId("useColorFloatFromInt8");
	//auto useColorFloatFromInt16StateId = shaderProgram->GetStateValueId("useColorFloatFromInt16");
	//auto useColorFloatFromUInt8StateId = shaderProgram->GetStateValueId("useColorFloatFromUInt8");
	//auto useColorFloatFromUInt16StateId = shaderProgram->GetStateValueId("useColorFloatFromUInt16");
	//auto useColorFloatFromUIntA2B10G10R10StateId = shaderProgram->GetStateValueId("useColorFloatFromUIntA2B10G10R10");
	auto useColorIntFromInt8StateId = shaderProgram->GetStateValueId("useColorIntFromInt8");
	auto useColorIntFromInt16StateId = shaderProgram->GetStateValueId("useColorIntFromInt16");
	//auto useColorIntFromUInt8StateId = shaderProgram->GetStateValueId("useColorIntFromUInt8");
	//auto useColorIntFromUInt16StateId = shaderProgram->GetStateValueId("useColorIntFromUInt16");
	//auto useColorUIntFromInt8StateId = shaderProgram->GetStateValueId("useColorUIntFromInt8");
	//auto useColorUIntFromInt16StateId = shaderProgram->GetStateValueId("useColorUIntFromInt16");
	auto useColorUIntFromUInt8StateId = shaderProgram->GetStateValueId("useColorUIntFromUInt8");
	auto useColorUIntFromUInt16StateId = shaderProgram->GetStateValueId("useColorUIntFromUInt16");
	//auto useColorUIntFromUIntA2B10G10R10StateId = shaderProgram->GetStateValueId("useColorUIntFromUIntA2B10G10R10");

	// Retrieve the primitive data
	size_t vertexCount = 50;
	std::vector<V3Float32> positionVertexData;
	std::vector<V1UInt32> indexData;
	Geometry().CreatePrimitiveCircleAsLines((uint32_t)vertexCount, positionVertexData, indexData);

	// Generate the color data
	static const float pi = 3.14159265358979323846f;
	std::vector<V3Float32> colorFloat32VertexData(vertexCount);
	for (size_t i = 0; i < vertexCount; ++i)
	{
		colorFloat32VertexData[i].X() = (float)i / (float)vertexCount;
		colorFloat32VertexData[i].Y() = 1.0f - ((float)i / (float)vertexCount);
		colorFloat32VertexData[i].Z() = std::sin(((float)i / (float)vertexCount) * pi);
	}
	std::vector<V3Int8> colorInt8VertexData(vertexCount);
	for (size_t i = 0; i < vertexCount; ++i)
	{
		auto r = static_cast<signed char>(std::lround(colorFloat32VertexData[i].X() * (float)std::numeric_limits<signed char>::max()));
		auto g = static_cast<signed char>(std::lround(colorFloat32VertexData[i].Y() * (float)std::numeric_limits<signed char>::max()));
		auto b = static_cast<signed char>(std::lround(colorFloat32VertexData[i].Z() * (float)std::numeric_limits<signed char>::max()));
		colorInt8VertexData[i].X() = r;
		colorInt8VertexData[i].Y() = g;
		colorInt8VertexData[i].Z() = b;
	}
	std::vector<V3Int16> colorInt16VertexData(vertexCount);
	for (size_t i = 0; i < vertexCount; ++i)
	{
		auto r = static_cast<short>(std::lround(colorFloat32VertexData[i].X() * (float)std::numeric_limits<short>::max()));
		auto g = static_cast<short>(std::lround(colorFloat32VertexData[i].Y() * (float)std::numeric_limits<short>::max()));
		auto b = static_cast<short>(std::lround(colorFloat32VertexData[i].Z() * (float)std::numeric_limits<short>::max()));
		colorInt16VertexData[i].X() = r;
		colorInt16VertexData[i].Y() = g;
		colorInt16VertexData[i].Z() = b;
	}
	std::vector<V3UInt8> colorUInt8VertexData(vertexCount);
	for (size_t i = 0; i < vertexCount; ++i)
	{
		auto r = static_cast<unsigned char>(std::lround(colorFloat32VertexData[i].X() * (float)std::numeric_limits<unsigned char>::max()));
		auto g = static_cast<unsigned char>(std::lround(colorFloat32VertexData[i].Y() * (float)std::numeric_limits<unsigned char>::max()));
		auto b = static_cast<unsigned char>(std::lround(colorFloat32VertexData[i].Z() * (float)std::numeric_limits<unsigned char>::max()));
		colorUInt8VertexData[i].X() = r;
		colorUInt8VertexData[i].Y() = g;
		colorUInt8VertexData[i].Z() = b;
	}
	std::vector<V3UInt16> colorUInt16VertexData(vertexCount);
	for (size_t i = 0; i < vertexCount; ++i)
	{
		auto r = static_cast<unsigned short>(std::lround(colorFloat32VertexData[i].X() * (float)std::numeric_limits<unsigned short>::max()));
		auto g = static_cast<unsigned short>(std::lround(colorFloat32VertexData[i].Y() * (float)std::numeric_limits<unsigned short>::max()));
		auto b = static_cast<unsigned short>(std::lround(colorFloat32VertexData[i].Z() * (float)std::numeric_limits<unsigned short>::max()));
		colorUInt16VertexData[i].X() = r;
		colorUInt16VertexData[i].Y() = g;
		colorUInt16VertexData[i].Z() = b;
	}
	std::vector<V3Norm8> colorNorm8VertexData(vertexCount);
	for (size_t i = 0; i < vertexCount; ++i)
	{
		auto r = static_cast<signed char>(std::lround(colorFloat32VertexData[i].X() * (float)std::numeric_limits<signed char>::max()));
		auto g = static_cast<signed char>(std::lround(colorFloat32VertexData[i].Y() * (float)std::numeric_limits<signed char>::max()));
		auto b = static_cast<signed char>(std::lround(colorFloat32VertexData[i].Z() * (float)std::numeric_limits<signed char>::max()));
		colorNorm8VertexData[i].X().data[0] = static_cast<unsigned char>(r);
		colorNorm8VertexData[i].Y().data[0] = static_cast<unsigned char>(g);
		colorNorm8VertexData[i].Z().data[0] = static_cast<unsigned char>(b);
	}
	std::vector<V3Norm16> colorNorm16VertexData(vertexCount);
	for (size_t i = 0; i < vertexCount; ++i)
	{
		auto r = static_cast<short>(std::lround(colorFloat32VertexData[i].X() * (float)std::numeric_limits<short>::max()));
		auto g = static_cast<short>(std::lround(colorFloat32VertexData[i].Y() * (float)std::numeric_limits<short>::max()));
		auto b = static_cast<short>(std::lround(colorFloat32VertexData[i].Z() * (float)std::numeric_limits<short>::max()));
		colorNorm16VertexData[i].X() = *reinterpret_cast<BasicNorm16*>(&r);
		colorNorm16VertexData[i].Y() = *reinterpret_cast<BasicNorm16*>(&g);
		colorNorm16VertexData[i].Z() = *reinterpret_cast<BasicNorm16*>(&b);
	}
	std::vector<V3UNorm8> colorUNorm8VertexData(vertexCount);
	for (size_t i = 0; i < vertexCount; ++i)
	{
		auto r = static_cast<unsigned char>(std::lround(colorFloat32VertexData[i].X() * (float)std::numeric_limits<unsigned char>::max()));
		auto g = static_cast<unsigned char>(std::lround(colorFloat32VertexData[i].Y() * (float)std::numeric_limits<unsigned char>::max()));
		auto b = static_cast<unsigned char>(std::lround(colorFloat32VertexData[i].Z() * (float)std::numeric_limits<unsigned char>::max()));
		colorUNorm8VertexData[i].X() = *reinterpret_cast<BasicUNorm8*>(&r);
		colorUNorm8VertexData[i].Y() = *reinterpret_cast<BasicUNorm8*>(&g);
		colorUNorm8VertexData[i].Z() = *reinterpret_cast<BasicUNorm8*>(&b);
	}
	std::vector<uint32_t> colorA2B10G10R10UNormVertexData(vertexCount);
	for (size_t i = 0; i < vertexCount; ++i)
	{
		auto r = static_cast<uint32_t>(std::lround(colorFloat32VertexData[i].X() * (float)1023));
		auto g = static_cast<uint32_t>(std::lround(colorFloat32VertexData[i].Y() * (float)1023));
		auto b = static_cast<uint32_t>(std::lround(colorFloat32VertexData[i].Z() * (float)1023));
		uint32_t a = 0x3;
		colorA2B10G10R10UNormVertexData[i] = ((a & 0x3) << 30) | ((b & 0x3FF) << 20) | ((g & 0x3FF) << 10) | (r & 0x3FF);
	}
	std::vector<V3UNorm16> colorUNorm16VertexData(vertexCount);
	for (size_t i = 0; i < vertexCount; ++i)
	{
		auto r = static_cast<unsigned short>(std::lround(colorFloat32VertexData[i].X() * (float)std::numeric_limits<unsigned short>::max()));
		auto g = static_cast<unsigned short>(std::lround(colorFloat32VertexData[i].Y() * (float)std::numeric_limits<unsigned short>::max()));
		auto b = static_cast<unsigned short>(std::lround(colorFloat32VertexData[i].Z() * (float)std::numeric_limits<unsigned short>::max()));
		colorUNorm16VertexData[i].X() = *reinterpret_cast<BasicUNorm16*>(&r);
		colorUNorm16VertexData[i].Y() = *reinterpret_cast<BasicUNorm16*>(&g);
		colorUNorm16VertexData[i].Z() = *reinterpret_cast<BasicUNorm16*>(&b);
	}
	std::vector<V3Float16> colorFloat16VertexData(vertexCount);
	for (size_t i = 0; i < vertexCount; ++i)
	{
		auto r = colorFloat32VertexData[i].X();
		auto g = colorFloat32VertexData[i].Y();
		auto b = colorFloat32VertexData[i].Z();
		*reinterpret_cast<half_float::half*>(&colorFloat16VertexData[i].X()) = half_float::half_cast<half_float::half>(r);
		*reinterpret_cast<half_float::half*>(&colorFloat16VertexData[i].Y()) = half_float::half_cast<half_float::half>(g);
		*reinterpret_cast<half_float::half*>(&colorFloat16VertexData[i].Z()) = half_float::half_cast<half_float::half>(b);
	}

	// Create our vertex buffer and populate it with data
	VertexAttribute<V3Float32> vertexAttributePosition(positionVertexData.size(), IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadNever, IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadOften);
	VertexAttribute<V3Int8> vertexAttributeColorInt8(colorInt8VertexData.size(), IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadNever, IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadOften);
	VertexAttribute<V3Int16> vertexAttributeColorInt16(colorInt16VertexData.size(), IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadNever, IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadOften);
	VertexAttribute<V3UInt8> vertexAttributeColorUInt8(colorUInt8VertexData.size(), IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadNever, IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadOften);
	VertexAttribute<V3UInt16> vertexAttributeColorUInt16(colorUInt16VertexData.size(), IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadNever, IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadOften);
	VertexAttribute<V3Norm8> vertexAttributeColorNorm8(colorNorm8VertexData.size(), IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadNever, IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadOften);
	VertexAttribute<V3Norm16> vertexAttributeColorNorm16(colorNorm16VertexData.size(), IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadNever, IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadOften);
	VertexAttribute<V3UNorm8> vertexAttributeColorUNorm8(colorUNorm8VertexData.size(), IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadNever, IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadOften);
	VertexAttribute<V3UNorm16> vertexAttributeColorUNorm16(colorUNorm16VertexData.size(), IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadNever, IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadOften);
	VertexAttribute<V3Float16> vertexAttributeColorFloat16(colorFloat16VertexData.size(), IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadNever, IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadOften);
	VertexAttribute<V3Float32> vertexAttributeColorFloat32(colorFloat32VertexData.size(), IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadNever, IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadOften);
	RawVertexAttribute vertexAttributeColorA2B10G10R10UNorm(IVertexAttribute::DataType::A2B10G10R10UNorm, 1, colorA2B10G10R10UNormVertexData.size(), IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadNever, IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadOften);
	//RawVertexAttribute vertexAttributeColorA2B10G10R10UInt(IVertexAttribute::DataType::A2B10G10R10UInt, 1, colorA2B10G10R10UNormVertexData.size(), IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadNever, IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadOften);
	auto vertexBuffer = renderer.CreateVertexBuffer();
	REQUIRE(vertexBuffer->BindVertexAttribute(vertexAttributePosition));
	REQUIRE(vertexBuffer->BindVertexAttribute(vertexAttributeColorInt8));
	REQUIRE(vertexBuffer->BindVertexAttribute(vertexAttributeColorInt16));
	REQUIRE(vertexBuffer->BindVertexAttribute(vertexAttributeColorUInt8));
	REQUIRE(vertexBuffer->BindVertexAttribute(vertexAttributeColorUInt16));
	REQUIRE(vertexBuffer->BindVertexAttribute(vertexAttributeColorNorm8));
	REQUIRE(vertexBuffer->BindVertexAttribute(vertexAttributeColorNorm16));
	REQUIRE(vertexBuffer->BindVertexAttribute(vertexAttributeColorUNorm8));
	REQUIRE(vertexBuffer->BindVertexAttribute(vertexAttributeColorUNorm16));
	REQUIRE(vertexBuffer->BindVertexAttribute(vertexAttributeColorFloat16));
	REQUIRE(vertexBuffer->BindVertexAttribute(vertexAttributeColorFloat32));
	REQUIRE(vertexBuffer->BindVertexAttribute(vertexAttributeColorA2B10G10R10UNorm));
	//REQUIRE(vertexBuffer->BindVertexAttribute(vertexAttributeColorA2B10G10R10UInt));
	REQUIRE(vertexAttributePosition.SetInitialData(positionVertexData));
	REQUIRE(vertexAttributeColorInt8.SetInitialData(colorInt8VertexData));
	REQUIRE(vertexAttributeColorInt16.SetInitialData(colorInt16VertexData));
	REQUIRE(vertexAttributeColorUInt8.SetInitialData(colorUInt8VertexData));
	REQUIRE(vertexAttributeColorUInt16.SetInitialData(colorUInt16VertexData));
	REQUIRE(vertexAttributeColorNorm8.SetInitialData(colorNorm8VertexData));
	REQUIRE(vertexAttributeColorNorm16.SetInitialData(colorNorm16VertexData));
	REQUIRE(vertexAttributeColorUNorm8.SetInitialData(colorUNorm8VertexData));
	REQUIRE(vertexAttributeColorUNorm16.SetInitialData(colorUNorm16VertexData));
	REQUIRE(vertexAttributeColorFloat16.SetInitialData(colorFloat16VertexData));
	REQUIRE(vertexAttributeColorFloat32.SetInitialData(colorFloat32VertexData));
	REQUIRE(vertexAttributeColorA2B10G10R10UNorm.SetInitialData(reinterpret_cast<const uint8_t*>(colorA2B10G10R10UNormVertexData.data()), colorA2B10G10R10UNormVertexData.size(), 4));
	//REQUIRE(vertexAttributeColorA2B10G10R10UInt.SetInitialData(reinterpret_cast<const uint8_t*>(colorA2B10G10R10UNormVertexData.data()), colorA2B10G10R10UNormVertexData.size(), 4));
	REQUIRE(vertexBuffer->AllocateMemory());

	// Create our index buffer and populate it with data
	IndexAttribute<V1UInt32> indexAttribute(indexData.size(), IIndexAttribute::PerformanceHint::WriteNever | IIndexAttribute::PerformanceHint::ReadNever, IIndexAttribute::PerformanceHint::WriteNever | IIndexAttribute::PerformanceHint::ReadOften);
	auto indexBuffer = renderer.CreateIndexBuffer();
	REQUIRE(indexBuffer->BindIndexAttribute(indexAttribute));
	REQUIRE(indexAttribute.SetInitialData(indexData));
	REQUIRE(indexBuffer->AllocateMemory());

	// Create our state group node
	auto groupNode = renderer.CreateStateGroupNode();

	// Create our program node
	auto programNode = renderer.CreateProgramNode();
	REQUIRE(programNode->BindShaderProgram(shaderProgram.get()));
	programNode->AddChildNode(groupNode.get());

	// Create our render pass node
	auto renderPassNode = renderer.CreateRenderPassNode();
	renderPassNode->BindFrameBuffer(testWindowFrameBuffer.get());
	renderPassNode->SetAttachmentClearData(IFrameBuffer::AttachmentType::Color, 0, V4Float32(0.0f, 0.0f, 0.0f, 1.0f));
	renderPassNode->SetAttachmentClearData(IFrameBuffer::AttachmentType::Depth, 0, V4Float32(1.0f, 0.0f, 0.0f, 0.0f));
	renderPassNode->AddChildNode(programNode.get());

	// Bind our render tree to the renderer
	renderer.SetRenderPasses(&renderPassNode, 1);

	// Pick the image diff algorithms we'll be using to compare our reference images. Differences in line rasterization,
	// especially under software rasterizers, mean not all the diff algorithms give good results.
	cobalt::graphics::IImageDiff::Algorithm lineCompareAlgorithm = cobalt::graphics::IImageDiff::Algorithm::BinaryCountExact | cobalt::graphics::IImageDiff::Algorithm::RegionRanges;

	// Test binding color data as ints, converted from Int8
	{
		// Create our renderable node
		auto renderableNode = renderer.CreateRenderableNode();
		REQUIRE(renderableNode->BindVertexAttribute(vertexAttributePosition, positionAttributeId));
		REQUIRE(renderableNode->BindVertexAttribute(vertexAttributeColorInt8, colorIntAttributeId));
		REQUIRE(renderableNode->BindIndexAttribute(indexAttribute));
		REQUIRE(renderableNode->SetPrimitiveMode(IRenderableNode::PrimitiveMode::Lines));
		renderableNode->SetStateValue(useColorIntFromInt8StateId, true);
		groupNode->AddChildNode(renderableNode.get());

		// Capture an image of the scene
		auto frameBufferCapture = renderer.CreateFrameBufferOutput();
		frameBufferCapture->SetDetachAfterCapture(true);
		testWindowFrameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
		DrawOneFrame();
		session.AddTestImageResult("CircleColorIntFromInt8", "A circle with color transitions", std::move(frameBufferCapture), lineCompareAlgorithm);

		// Remove the renderable node
		groupNode->RemoveChildNode(renderableNode.get());
	}

	// Test binding color data as ints, converted from Int16
	{
		// Create our renderable node
		auto renderableNode = renderer.CreateRenderableNode();
		REQUIRE(renderableNode->BindVertexAttribute(vertexAttributePosition, positionAttributeId));
		REQUIRE(renderableNode->BindVertexAttribute(vertexAttributeColorInt16, colorIntAttributeId));
		REQUIRE(renderableNode->BindIndexAttribute(indexAttribute));
		REQUIRE(renderableNode->SetPrimitiveMode(IRenderableNode::PrimitiveMode::Lines));
		renderableNode->SetStateValue(useColorIntFromInt16StateId, true);
		groupNode->AddChildNode(renderableNode.get());

		// Capture an image of the scene
		auto frameBufferCapture = renderer.CreateFrameBufferOutput();
		frameBufferCapture->SetDetachAfterCapture(true);
		testWindowFrameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
		DrawOneFrame();
		session.AddTestImageResult("CircleColorIntFromInt16", "A circle with color transitions", std::move(frameBufferCapture), lineCompareAlgorithm);

		// Remove the renderable node
		groupNode->RemoveChildNode(renderableNode.get());
	}

	// These conversions, or lack thereof, are technically undefined under each API. They can test if it's possible to
	// bind integer types using a mismatch between signed/unsigned data. They trigger validation errors under Vulkan
	// however which is stricter, and actually cause the device to be lost on some drivers, so we have them disabled
	// here.
	//// Test binding color data as ints, converted from UInt8
	//{
	//	// Create our renderable node
	//	auto renderableNode = renderer.CreateRenderableNode();
	//	REQUIRE(renderableNode->BindVertexAttribute(vertexAttributePosition, positionAttributeId));
	//	REQUIRE(renderableNode->BindVertexAttribute(vertexAttributeColorUInt8, colorIntAttributeId));
	//	REQUIRE(renderableNode->BindIndexAttribute(indexAttribute));
	//	REQUIRE(renderableNode->SetPrimitiveMode(IRenderableNode::PrimitiveMode::Lines));
	//	renderableNode->SetStateValue(useColorIntFromUInt8StateId, true);
	//	groupNode->AddChildNode(renderableNode.get());

	//	// Capture an image of the scene
	//	auto frameBufferCapture = renderer.CreateFrameBufferOutput();
	//	frameBufferCapture->SetDetachAfterCapture(true);
	//	testWindowFrameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
	//	DrawOneFrame();
	//	session.AddTestImageResult("CircleColorIntFromUInt8", "A circle with colour transitions", std::move(frameBufferCapture), lineCompareAlgorithm);

	//	// Remove the renderable node
	//	groupNode->RemoveChildNode(renderableNode.get());
	//}

	//// Test binding color data as ints, converted from UInt16
	//{
	//	// Create our renderable node
	//	auto renderableNode = renderer.CreateRenderableNode();
	//	REQUIRE(renderableNode->BindVertexAttribute(vertexAttributePosition, positionAttributeId));
	//	REQUIRE(renderableNode->BindVertexAttribute(vertexAttributeColorUInt16, colorIntAttributeId));
	//	REQUIRE(renderableNode->BindIndexAttribute(indexAttribute));
	//	REQUIRE(renderableNode->SetPrimitiveMode(IRenderableNode::PrimitiveMode::Lines));
	//	renderableNode->SetStateValue(useColorIntFromUInt16StateId, true);
	//	groupNode->AddChildNode(renderableNode.get());

	//	// Capture an image of the scene
	//	auto frameBufferCapture = renderer.CreateFrameBufferOutput();
	//	frameBufferCapture->SetDetachAfterCapture(true);
	//	testWindowFrameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
	//	DrawOneFrame();
	//	session.AddTestImageResult("CircleColorIntFromUInt16", "A circle with colour transitions", std::move(frameBufferCapture), lineCompareAlgorithm);

	//	// Remove the renderable node
	//	groupNode->RemoveChildNode(renderableNode.get());
	//}

	//// Test binding color data as unsigned ints, converted from Int8
	//{
	//	// Create our renderable node
	//	auto renderableNode = renderer.CreateRenderableNode();
	//	REQUIRE(renderableNode->BindVertexAttribute(vertexAttributePosition, positionAttributeId));
	//	REQUIRE(renderableNode->BindVertexAttribute(vertexAttributeColorInt8, colorUIntAttributeId));
	//	REQUIRE(renderableNode->BindIndexAttribute(indexAttribute));
	//	REQUIRE(renderableNode->SetPrimitiveMode(IRenderableNode::PrimitiveMode::Lines));
	//	renderableNode->SetStateValue(useColorUIntFromInt8StateId, true);
	//	groupNode->AddChildNode(renderableNode.get());

	//	// Capture an image of the scene
	//	auto frameBufferCapture = renderer.CreateFrameBufferOutput();
	//	frameBufferCapture->SetDetachAfterCapture(true);
	//	testWindowFrameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
	//	DrawOneFrame();
	//	session.AddTestImageResult("CircleColorUIntFromInt8", "A circle with colour transitions", std::move(frameBufferCapture), lineCompareAlgorithm);

	//	// Remove the renderable node
	//	groupNode->RemoveChildNode(renderableNode.get());
	//}

	//// Test binding color data as unsigned ints, converted from Int16
	//{
	//	// Create our renderable node
	//	auto renderableNode = renderer.CreateRenderableNode();
	//	REQUIRE(renderableNode->BindVertexAttribute(vertexAttributePosition, positionAttributeId));
	//	REQUIRE(renderableNode->BindVertexAttribute(vertexAttributeColorInt16, colorUIntAttributeId));
	//	REQUIRE(renderableNode->BindIndexAttribute(indexAttribute));
	//	REQUIRE(renderableNode->SetPrimitiveMode(IRenderableNode::PrimitiveMode::Lines));
	//	renderableNode->SetStateValue(useColorUIntFromInt16StateId, true);
	//	groupNode->AddChildNode(renderableNode.get());

	//	// Capture an image of the scene
	//	auto frameBufferCapture = renderer.CreateFrameBufferOutput();
	//	frameBufferCapture->SetDetachAfterCapture(true);
	//	testWindowFrameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
	//	DrawOneFrame();
	//	session.AddTestImageResult("CircleColorUIntFromInt16", "A circle with colour transitions", std::move(frameBufferCapture), lineCompareAlgorithm);

	//	// Remove the renderable node
	//	groupNode->RemoveChildNode(renderableNode.get());
	//}

	// Test binding color data as unsigned ints, converted from UInt8
	{
		// Create our renderable node
		auto renderableNode = renderer.CreateRenderableNode();
		REQUIRE(renderableNode->BindVertexAttribute(vertexAttributePosition, positionAttributeId));
		REQUIRE(renderableNode->BindVertexAttribute(vertexAttributeColorUInt8, colorUIntAttributeId));
		REQUIRE(renderableNode->BindIndexAttribute(indexAttribute));
		REQUIRE(renderableNode->SetPrimitiveMode(IRenderableNode::PrimitiveMode::Lines));
		renderableNode->SetStateValue(useColorUIntFromUInt8StateId, true);
		groupNode->AddChildNode(renderableNode.get());

		// Capture an image of the scene
		auto frameBufferCapture = renderer.CreateFrameBufferOutput();
		frameBufferCapture->SetDetachAfterCapture(true);
		testWindowFrameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
		DrawOneFrame();
		session.AddTestImageResult("CircleColorUIntFromUInt8", "A circle with color transitions", std::move(frameBufferCapture), lineCompareAlgorithm);

		// Remove the renderable node
		groupNode->RemoveChildNode(renderableNode.get());
	}

	// Test binding color data as unsigned ints, converted from UInt16
	{
		// Create our renderable node
		auto renderableNode = renderer.CreateRenderableNode();
		REQUIRE(renderableNode->BindVertexAttribute(vertexAttributePosition, positionAttributeId));
		REQUIRE(renderableNode->BindVertexAttribute(vertexAttributeColorUInt16, colorUIntAttributeId));
		REQUIRE(renderableNode->BindIndexAttribute(indexAttribute));
		REQUIRE(renderableNode->SetPrimitiveMode(IRenderableNode::PrimitiveMode::Lines));
		renderableNode->SetStateValue(useColorUIntFromUInt16StateId, true);
		groupNode->AddChildNode(renderableNode.get());

		// Capture an image of the scene
		auto frameBufferCapture = renderer.CreateFrameBufferOutput();
		frameBufferCapture->SetDetachAfterCapture(true);
		testWindowFrameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
		DrawOneFrame();
		session.AddTestImageResult("CircleColorUIntFromUInt16", "A circle with color transitions", std::move(frameBufferCapture), lineCompareAlgorithm);

		// Remove the renderable node
		groupNode->RemoveChildNode(renderableNode.get());
	}

	// This conversion is possible on Vulkan and Direct3D, however unfortunately is NOT possible on OpenGL, which only
	// supports converting this packed format to floating point types, not integer types.
	//// Test binding color data as unsigned ints, converted from A2B10G10R10UInt
	//{
	//	// Create our renderable node
	//	auto renderableNode = renderer.CreateRenderableNode();
	//	REQUIRE(renderableNode->BindVertexAttribute(vertexAttributePosition, positionAttributeId));
	//	REQUIRE(renderableNode->BindVertexAttribute(vertexAttributeColorA2B10G10R10UInt, colorUIntAttributeId));
	//	REQUIRE(renderableNode->BindIndexAttribute(indexAttribute));
	//	REQUIRE(renderableNode->SetPrimitiveMode(IRenderableNode::PrimitiveMode::Lines));
	//	renderableNode->SetStateValue(useColorUIntFromUIntA2B10G10R10StateId, true);
	//	groupNode->AddChildNode(renderableNode.get());

	//	// Capture an image of the scene
	//	auto frameBufferCapture = renderer.CreateFrameBufferOutput();
	//	frameBufferCapture->SetDetachAfterCapture(true);
	//	testWindowFrameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
	//	DrawOneFrame();
	//	session.AddTestImageResult("CircleColorUIntFromA2B10G10R10UInt", "A circle with colour transitions", std::move(frameBufferCapture), lineCompareAlgorithm);

	//	// Remove the renderable node
	//	groupNode->RemoveChildNode(renderableNode.get());
	//}

	// These conversions are possible on OpenGL and Vulkan, but do not appear to work under Direct3D 11 or 12. The
	// documentation is very poor in this area however, so it's unclear what conversions are supposed to be considered
	// valid, or if there's some alternate way to make these work under Direct3D. Since further investigation is
	// warranted, these test cases are being left here disabled, so that they can be used for testing in the future, and
	// restored if these conversions can be made to work under Direct3D.
	//// Test binding color data as floats, converted from Int8
	//{
	//	// Create our renderable node
	//	auto renderableNode = renderer.CreateRenderableNode();
	//	REQUIRE(renderableNode->BindVertexAttribute(vertexAttributePosition, positionAttributeId));
	//	REQUIRE(renderableNode->BindVertexAttribute(vertexAttributeColorInt8, colorFloatAttributeId));
	//	REQUIRE(renderableNode->BindIndexAttribute(indexAttribute));
	//	REQUIRE(renderableNode->SetPrimitiveMode(IRenderableNode::PrimitiveMode::Lines));
	//	renderableNode->SetStateValue(useColorFloatFromInt8StateId, true);
	//	groupNode->AddChildNode(renderableNode.get());

	//	// Capture an image of the scene
	//	auto frameBufferCapture = renderer.CreateFrameBufferOutput();
	//	frameBufferCapture->SetDetachAfterCapture(true);
	//	testWindowFrameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
	//	DrawOneFrame();
	//	session.AddTestImageResult("CircleColorFloatFromInt8", "A circle with colour transitions", std::move(frameBufferCapture), lineCompareAlgorithm);

	//	// Remove the renderable node
	//	groupNode->RemoveChildNode(renderableNode.get());
	//}

	//// Test binding color data as floats, converted from Int16
	//{
	//	// Create our renderable node
	//	auto renderableNode = renderer.CreateRenderableNode();
	//	REQUIRE(renderableNode->BindVertexAttribute(vertexAttributePosition, positionAttributeId));
	//	REQUIRE(renderableNode->BindVertexAttribute(vertexAttributeColorInt16, colorFloatAttributeId));
	//	REQUIRE(renderableNode->BindIndexAttribute(indexAttribute));
	//	REQUIRE(renderableNode->SetPrimitiveMode(IRenderableNode::PrimitiveMode::Lines));
	//	renderableNode->SetStateValue(useColorFloatFromInt16StateId, true);
	//	groupNode->AddChildNode(renderableNode.get());

	//	// Capture an image of the scene
	//	auto frameBufferCapture = renderer.CreateFrameBufferOutput();
	//	frameBufferCapture->SetDetachAfterCapture(true);
	//	testWindowFrameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
	//	DrawOneFrame();
	//	session.AddTestImageResult("CircleColorFloatFromUInt16", "A circle with colour transitions", std::move(frameBufferCapture), lineCompareAlgorithm);

	//	// Remove the renderable node
	//	groupNode->RemoveChildNode(renderableNode.get());
	//}

	//// Test binding color data as floats, converted from UInt8
	//{
	//	// Create our renderable node
	//	auto renderableNode = renderer.CreateRenderableNode();
	//	REQUIRE(renderableNode->BindVertexAttribute(vertexAttributePosition, positionAttributeId));
	//	REQUIRE(renderableNode->BindVertexAttribute(vertexAttributeColorUInt8, colorFloatAttributeId));
	//	REQUIRE(renderableNode->BindIndexAttribute(indexAttribute));
	//	REQUIRE(renderableNode->SetPrimitiveMode(IRenderableNode::PrimitiveMode::Lines));
	//	renderableNode->SetStateValue(useColorFloatFromUInt8StateId, true);
	//	groupNode->AddChildNode(renderableNode.get());

	//	// Capture an image of the scene
	//	auto frameBufferCapture = renderer.CreateFrameBufferOutput();
	//	frameBufferCapture->SetDetachAfterCapture(true);
	//	testWindowFrameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
	//	DrawOneFrame();
	//	session.AddTestImageResult("CircleColorFloatFromUInt8", "A circle with colour transitions", std::move(frameBufferCapture), lineCompareAlgorithm);

	//	// Remove the renderable node
	//	groupNode->RemoveChildNode(renderableNode.get());
	//}

	//// Test binding color data as floats, converted from UInt16
	//{
	//	// Create our renderable node
	//	auto renderableNode = renderer.CreateRenderableNode();
	//	REQUIRE(renderableNode->BindVertexAttribute(vertexAttributePosition, positionAttributeId));
	//	REQUIRE(renderableNode->BindVertexAttribute(vertexAttributeColorUInt16, colorFloatAttributeId));
	//	REQUIRE(renderableNode->BindIndexAttribute(indexAttribute));
	//	REQUIRE(renderableNode->SetPrimitiveMode(IRenderableNode::PrimitiveMode::Lines));
	//	renderableNode->SetStateValue(useColorFloatFromUInt16StateId, true);
	//	groupNode->AddChildNode(renderableNode.get());

	//	// Capture an image of the scene
	//	auto frameBufferCapture = renderer.CreateFrameBufferOutput();
	//	frameBufferCapture->SetDetachAfterCapture(true);
	//	testWindowFrameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
	//	DrawOneFrame();
	//	session.AddTestImageResult("CircleColorFloatFromUInt16", "A circle with colour transitions", std::move(frameBufferCapture), lineCompareAlgorithm);

	//	// Remove the renderable node
	//	groupNode->RemoveChildNode(renderableNode.get());
	//}

	//// Test binding color data as floats, converted from A2B10G10R10UInt
	//{
	//	// Create our renderable node
	//	auto renderableNode = renderer.CreateRenderableNode();
	//	REQUIRE(renderableNode->BindVertexAttribute(vertexAttributePosition, positionAttributeId));
	//	REQUIRE(renderableNode->BindVertexAttribute(vertexAttributeColorA2B10G10R10UInt, colorFloatAttributeId));
	//	REQUIRE(renderableNode->BindIndexAttribute(indexAttribute));
	//	REQUIRE(renderableNode->SetPrimitiveMode(IRenderableNode::PrimitiveMode::Lines));
	//	renderableNode->SetStateValue(useColorFloatFromUIntA2B10G10R10StateId, true);
	//	groupNode->AddChildNode(renderableNode.get());

	//	// Capture an image of the scene
	//	auto frameBufferCapture = renderer.CreateFrameBufferOutput();
	//	frameBufferCapture->SetDetachAfterCapture(true);
	//	testWindowFrameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
	//	DrawOneFrame();
	//	session.AddTestImageResult("CircleColorFloatFromA2B10G10R10UInt", "A circle with colour transitions", std::move(frameBufferCapture), lineCompareAlgorithm);

	//	// Remove the renderable node
	//	groupNode->RemoveChildNode(renderableNode.get());
	//}

	// Test binding color data as floats, converted from Norm8
	{
		// Create our renderable node
		auto renderableNode = renderer.CreateRenderableNode();
		REQUIRE(renderableNode->BindVertexAttribute(vertexAttributePosition, positionAttributeId));
		REQUIRE(renderableNode->BindVertexAttribute(vertexAttributeColorNorm8, colorFloatAttributeId));
		REQUIRE(renderableNode->BindIndexAttribute(indexAttribute));
		REQUIRE(renderableNode->SetPrimitiveMode(IRenderableNode::PrimitiveMode::Lines));
		renderableNode->SetStateValue(useColorFloatStateId, true);
		groupNode->AddChildNode(renderableNode.get());

		// Capture an image of the scene
		auto frameBufferCapture = renderer.CreateFrameBufferOutput();
		frameBufferCapture->SetDetachAfterCapture(true);
		testWindowFrameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
		DrawOneFrame();
		session.AddTestImageResult("CircleColorFloatFromNorm8", "A circle with color transitions", std::move(frameBufferCapture), lineCompareAlgorithm);

		// Remove the renderable node
		groupNode->RemoveChildNode(renderableNode.get());
	}

	// Test binding color data as floats, converted from Norm16
	{
		// Create our renderable node
		auto renderableNode = renderer.CreateRenderableNode();
		REQUIRE(renderableNode->BindVertexAttribute(vertexAttributePosition, positionAttributeId));
		REQUIRE(renderableNode->BindVertexAttribute(vertexAttributeColorNorm16, colorFloatAttributeId));
		REQUIRE(renderableNode->BindIndexAttribute(indexAttribute));
		REQUIRE(renderableNode->SetPrimitiveMode(IRenderableNode::PrimitiveMode::Lines));
		renderableNode->SetStateValue(useColorFloatStateId, true);
		groupNode->AddChildNode(renderableNode.get());

		// Capture an image of the scene
		auto frameBufferCapture = renderer.CreateFrameBufferOutput();
		frameBufferCapture->SetDetachAfterCapture(true);
		testWindowFrameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
		DrawOneFrame();
		session.AddTestImageResult("CircleColorFloatFromNorm16", "A circle with color transitions", std::move(frameBufferCapture), lineCompareAlgorithm);

		// Remove the renderable node
		groupNode->RemoveChildNode(renderableNode.get());
	}

	// Test binding color data as floats, converted from UNorm8
	{
		// Create our renderable node
		auto renderableNode = renderer.CreateRenderableNode();
		REQUIRE(renderableNode->BindVertexAttribute(vertexAttributePosition, positionAttributeId));
		REQUIRE(renderableNode->BindVertexAttribute(vertexAttributeColorUNorm8, colorFloatAttributeId));
		REQUIRE(renderableNode->BindIndexAttribute(indexAttribute));
		REQUIRE(renderableNode->SetPrimitiveMode(IRenderableNode::PrimitiveMode::Lines));
		renderableNode->SetStateValue(useColorFloatStateId, true);
		groupNode->AddChildNode(renderableNode.get());

		// Capture an image of the scene
		auto frameBufferCapture = renderer.CreateFrameBufferOutput();
		frameBufferCapture->SetDetachAfterCapture(true);
		testWindowFrameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
		DrawOneFrame();
		session.AddTestImageResult("CircleColorFloatFromUNorm8", "A circle with color transitions", std::move(frameBufferCapture), lineCompareAlgorithm);

		// Remove the renderable node
		groupNode->RemoveChildNode(renderableNode.get());
	}

	// Test binding color data as floats, converted from UNorm16
	{
		// Create our renderable node
		auto renderableNode = renderer.CreateRenderableNode();
		REQUIRE(renderableNode->BindVertexAttribute(vertexAttributePosition, positionAttributeId));
		REQUIRE(renderableNode->BindVertexAttribute(vertexAttributeColorUNorm16, colorFloatAttributeId));
		REQUIRE(renderableNode->BindIndexAttribute(indexAttribute));
		REQUIRE(renderableNode->SetPrimitiveMode(IRenderableNode::PrimitiveMode::Lines));
		renderableNode->SetStateValue(useColorFloatStateId, true);
		groupNode->AddChildNode(renderableNode.get());

		// Capture an image of the scene
		auto frameBufferCapture = renderer.CreateFrameBufferOutput();
		frameBufferCapture->SetDetachAfterCapture(true);
		testWindowFrameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
		DrawOneFrame();
		session.AddTestImageResult("CircleColorFloatFromUNorm16", "A circle with color transitions", std::move(frameBufferCapture), lineCompareAlgorithm);

		// Remove the renderable node
		groupNode->RemoveChildNode(renderableNode.get());
	}

	// Test binding color data as floats, converted from Float16
	{
		// Create our renderable node
		auto renderableNode = renderer.CreateRenderableNode();
		REQUIRE(renderableNode->BindVertexAttribute(vertexAttributePosition, positionAttributeId));
		REQUIRE(renderableNode->BindVertexAttribute(vertexAttributeColorFloat16, colorFloatAttributeId));
		REQUIRE(renderableNode->BindIndexAttribute(indexAttribute));
		REQUIRE(renderableNode->SetPrimitiveMode(IRenderableNode::PrimitiveMode::Lines));
		renderableNode->SetStateValue(useColorFloatStateId, true);
		groupNode->AddChildNode(renderableNode.get());

		// Capture an image of the scene
		auto frameBufferCapture = renderer.CreateFrameBufferOutput();
		frameBufferCapture->SetDetachAfterCapture(true);
		testWindowFrameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
		DrawOneFrame();
		session.AddTestImageResult("CircleColorFloatFromFloat16", "A circle with color transitions", std::move(frameBufferCapture), lineCompareAlgorithm);

		// Remove the renderable node
		groupNode->RemoveChildNode(renderableNode.get());
	}

	// Test binding color data as floats, converted from A2B10G10R10UNorm
	{
		// Create our renderable node
		auto renderableNode = renderer.CreateRenderableNode();
		REQUIRE(renderableNode->BindVertexAttribute(vertexAttributePosition, positionAttributeId));
		REQUIRE(renderableNode->BindVertexAttribute(vertexAttributeColorA2B10G10R10UNorm, colorFloatAttributeId));
		REQUIRE(renderableNode->BindIndexAttribute(indexAttribute));
		REQUIRE(renderableNode->SetPrimitiveMode(IRenderableNode::PrimitiveMode::Lines));
		renderableNode->SetStateValue(useColorFloatStateId, true);
		groupNode->AddChildNode(renderableNode.get());

		// Capture an image of the scene
		auto frameBufferCapture = renderer.CreateFrameBufferOutput();
		frameBufferCapture->SetDetachAfterCapture(true);
		testWindowFrameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
		DrawOneFrame();
		session.AddTestImageResult("CircleColorFloatFromA2B10G10R10UNorm", "A circle with color transitions", std::move(frameBufferCapture), lineCompareAlgorithm);

		// Remove the renderable node
		groupNode->RemoveChildNode(renderableNode.get());
	}

	// Test binding color data as floats
	{
		// Create our renderable node
		auto renderableNode = renderer.CreateRenderableNode();
		REQUIRE(renderableNode->BindVertexAttribute(vertexAttributePosition, positionAttributeId));
		REQUIRE(renderableNode->BindVertexAttribute(vertexAttributeColorFloat32, colorFloatAttributeId));
		REQUIRE(renderableNode->BindIndexAttribute(indexAttribute));
		REQUIRE(renderableNode->SetPrimitiveMode(IRenderableNode::PrimitiveMode::Lines));
		renderableNode->SetStateValue(useColorFloatStateId, true);
		groupNode->AddChildNode(renderableNode.get());

		// Capture an image of the scene
		auto frameBufferCapture = renderer.CreateFrameBufferOutput();
		frameBufferCapture->SetDetachAfterCapture(true);
		testWindowFrameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
		DrawOneFrame();
		session.AddTestImageResult("CircleColorFloatFromFloat32", "A circle with color transitions", std::move(frameBufferCapture), lineCompareAlgorithm);

		// Remove the renderable node
		groupNode->RemoveChildNode(renderableNode.get());
	}

	// Remove all our defined render passes
	renderer.RemoveAllRenderPasses();
	return true;
}

} // namespace cobalt::graphics::testing
