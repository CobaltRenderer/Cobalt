// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "../UnitTestBase.h"
#include <algorithm>
#include <array>
#include <cstring>
#include <utility>
namespace cobalt::graphics::testing {
using namespace cobalt::graphics;

namespace {

// Define the number of logical values and resource entries used by the test
constexpr size_t AlignmentValueCount = 70;
constexpr size_t AlignmentResultGroupCount = 3;
constexpr size_t AlignmentResultCount = AlignmentValueCount * AlignmentResultGroupCount;

// Lay out the visualization as four consecutive 70-value blocks: globals, state, structured input, and structured output
constexpr size_t AlignmentVisualizationGroupCount = 4;
constexpr size_t AlignmentVisualizationResultCount = AlignmentValueCount * AlignmentVisualizationGroupCount;
constexpr size_t OpenGLMatrixStrideResultCount = 19;
constexpr size_t StructuredBufferEntryCount = 3;

// Track unexecuted and unsupported comparisons separately even though unsupported comparisons are drawn as successful.
enum class AlignmentResult
{
	Pending,
	Passed,
	Failed,
	NotApplicable,
};

// Use non-zero entries in both directions to verify each structured-buffer entry stride as well as every member.
constexpr size_t StructuredBufferTestEntry = 1;
constexpr size_t StructuredBufferOutputTestEntry = 2;

// Identify the native structured-buffer layout used by each renderer family
enum class StructuredAlignmentLayout
{
	Compact,
	Std430,
	VulkanHlsl,
};

// Define the vertex shader shared by both test paths
const std::string VertexShader = R"(
float4 main(float4 position : position) : SV_POSITION
{
    return position;
}
)";

// Define the shaders used to draw one visualization square for every individual comparison
const std::string VisualizationVertexShader = R"(
struct VertexInput
{
    float4 position : position;
    float3 color : color;
};

struct VertexOutput
{
    float4 position : SV_POSITION;
    float3 color : COLOR;
};

VertexOutput main(VertexInput vertexInput)
{
    VertexOutput vertexOutput;
    vertexOutput.position = vertexInput.position;
    vertexOutput.color = vertexInput.color;
    return vertexOutput;
}
)";

const std::string VisualizationFragmentShader = R"(
struct VertexOutput
{
    float4 position : SV_POSITION;
    float3 color : COLOR;
};

float4 main(VertexOutput fragmentInput) : SV_TARGET0
{
    return float4(fragmentInput.color, 1.0f);
}
)";

// Define native OpenGL shaders which leave uniform-block packing to the driver
const std::string OpenGLMatrixStrideVertexShader = R"(
#version 330

in vec4 position;

void main()
{
    gl_Position = position;
}
)";

const std::string OpenGLMatrixStrideFragmentShader = R"(
#version 330

layout(packed) uniform MatrixStrideStateBuffer
{
    mat2 matrix2;
    float scalarAfterMatrix2;
    mat3 matrix3;
    float scalarAfterMatrix3;
    vec4 tailValue;
};

out vec4 outputColor;

void main()
{
    int targetIndex = int(gl_FragCoord.x);
    if (targetIndex >= 19)
    {
        outputColor = vec4(0.0, 0.0, 0.0, 1.0);
        return;
    }
    bool matches = false;
    if (targetIndex == 0) matches = matrix2[0][0] == 1.0;
    else if (targetIndex == 1) matches = matrix2[0][1] == 2.0;
    else if (targetIndex == 2) matches = matrix2[1][0] == 3.0;
    else if (targetIndex == 3) matches = matrix2[1][1] == 4.0;
    else if (targetIndex == 4) matches = scalarAfterMatrix2 == 5.0;
    else if (targetIndex == 5) matches = matrix3[0][0] == 6.0;
    else if (targetIndex == 6) matches = matrix3[0][1] == 7.0;
    else if (targetIndex == 7) matches = matrix3[0][2] == 8.0;
    else if (targetIndex == 8) matches = matrix3[1][0] == 9.0;
    else if (targetIndex == 9) matches = matrix3[1][1] == 10.0;
    else if (targetIndex == 10) matches = matrix3[1][2] == 11.0;
    else if (targetIndex == 11) matches = matrix3[2][0] == 12.0;
    else if (targetIndex == 12) matches = matrix3[2][1] == 13.0;
    else if (targetIndex == 13) matches = matrix3[2][2] == 14.0;
    else if (targetIndex == 14) matches = scalarAfterMatrix3 == 15.0;
    else if (targetIndex == 15) matches = tailValue.x == 16.0;
    else if (targetIndex == 16) matches = tailValue.y == 17.0;
    else if (targetIndex == 17) matches = tailValue.z == 18.0;
    else if (targetIndex == 18) matches = tailValue.w == 19.0;
    outputColor = matches ? vec4(0.0, 1.0, 0.0, 1.0) : vec4(1.0, 0.0, 0.0, 1.0);
}
)";

// Define a minimal vertex shader in SPIR-V assembly so the regression does not depend on source-language packing.
const std::string VulkanMatrixStrideVertexShader = R"(
OpCapability Shader
OpMemoryModel Logical GLSL450
OpEntryPoint Vertex %main "main" %position %outputPosition

OpName %main "main"
OpName %position "position"
OpName %outputPosition "@entryPointOutput.position"

OpDecorate %position Location 0
OpDecorate %outputPosition BuiltIn Position

%void = OpTypeVoid
%functionType = OpTypeFunction %void
%float = OpTypeFloat 32
%v4float = OpTypeVector %float 4
%inputV4Float = OpTypePointer Input %v4float
%outputV4Float = OpTypePointer Output %v4float
%position = OpVariable %inputV4Float Input
%outputPosition = OpVariable %outputV4Float Output

%main = OpFunction %void None %functionType
%entry = OpLabel
%positionValue = OpLoad %v4float %position
OpStore %outputPosition %positionValue
OpReturn
OpFunctionEnd
)";

// Define uniform blocks with matrix strides larger than their vectors and array strides distinct from matrix strides.
// The final state-buffer matrix also uses a different stride and ends the block, exercising reflected allocation size.
const std::string VulkanMatrixStrideFragmentShader = R"(
OpCapability Shader
OpMemoryModel Logical GLSL450
OpEntryPoint Fragment %main "main" %outputColor
OpExecutionMode %main OriginUpperLeft

OpName %main "main"
OpName %outputColor "@entryPointOutput"
OpName %globalBuffer "$Global"
OpName %GlobalBlock "$Global"
OpMemberName %GlobalBlock 0 "globalMatrix"
OpMemberName %GlobalBlock 1 "globalSentinel"
OpMemberName %GlobalBlock 2 "globalMatrixArray"
OpMemberName %GlobalBlock 3 "globalTail"
OpName %stateBuffer "MatrixStrideStateBuffer"
OpName %StateBlock "MatrixStrideStateBuffer"
OpMemberName %StateBlock 0 "stateMatrix"
OpMemberName %StateBlock 1 "stateSentinel"
OpMemberName %StateBlock 2 "stateMatrixArray"
OpMemberName %StateBlock 3 "stateTail"
OpMemberName %StateBlock 4 "stateFinalMatrix"

OpDecorate %outputColor Location 0
OpDecorate %matrixArray ArrayStride 80
OpDecorate %GlobalBlock Block
OpMemberDecorate %GlobalBlock 0 ColMajor
OpMemberDecorate %GlobalBlock 0 Offset 0
OpMemberDecorate %GlobalBlock 0 MatrixStride 32
OpMemberDecorate %GlobalBlock 1 Offset 64
OpMemberDecorate %GlobalBlock 2 ColMajor
OpMemberDecorate %GlobalBlock 2 Offset 80
OpMemberDecorate %GlobalBlock 2 MatrixStride 32
OpMemberDecorate %GlobalBlock 3 Offset 240
OpDecorate %globalBuffer DescriptorSet 0
OpDecorate %globalBuffer Binding 0
OpDecorate %StateBlock Block
OpMemberDecorate %StateBlock 0 ColMajor
OpMemberDecorate %StateBlock 0 Offset 0
OpMemberDecorate %StateBlock 0 MatrixStride 32
OpMemberDecorate %StateBlock 1 Offset 64
OpMemberDecorate %StateBlock 2 ColMajor
OpMemberDecorate %StateBlock 2 Offset 80
OpMemberDecorate %StateBlock 2 MatrixStride 32
OpMemberDecorate %StateBlock 3 Offset 240
OpMemberDecorate %StateBlock 4 ColMajor
OpMemberDecorate %StateBlock 4 Offset 256
OpMemberDecorate %StateBlock 4 MatrixStride 48
OpDecorate %stateBuffer DescriptorSet 0
OpDecorate %stateBuffer Binding 1

%void = OpTypeVoid
%functionType = OpTypeFunction %void
%bool = OpTypeBool
%v2bool = OpTypeVector %bool 2
%int = OpTypeInt 32 1
%int_0 = OpConstant %int 0
%int_1 = OpConstant %int 1
%int_2 = OpConstant %int 2
%int_3 = OpConstant %int 3
%int_4 = OpConstant %int 4
%uint = OpTypeInt 32 0
%uint_2 = OpConstant %uint 2
%float = OpTypeFloat 32
%v2float = OpTypeVector %float 2
%v4float = OpTypeVector %float 4
%matrix2 = OpTypeMatrix %v2float 2
%matrixArray = OpTypeArray %matrix2 %uint_2
%GlobalBlock = OpTypeStruct %matrix2 %float %matrixArray %float
%StateBlock = OpTypeStruct %matrix2 %float %matrixArray %float %matrix2
%uniformGlobalBlock = OpTypePointer Uniform %GlobalBlock
%uniformStateBlock = OpTypePointer Uniform %StateBlock
%uniformMatrix2 = OpTypePointer Uniform %matrix2
%uniformFloat = OpTypePointer Uniform %float
%outputV4Float = OpTypePointer Output %v4float
%globalBuffer = OpVariable %uniformGlobalBlock Uniform
%stateBuffer = OpVariable %uniformStateBlock Uniform
%outputColor = OpVariable %outputV4Float Output

%float_0 = OpConstant %float 0
%float_1 = OpConstant %float 1
%float_2 = OpConstant %float 2
%float_3 = OpConstant %float 3
%float_4 = OpConstant %float 4
%float_5 = OpConstant %float 5
%float_6 = OpConstant %float 6
%float_7 = OpConstant %float 7
%float_8 = OpConstant %float 8
%float_9 = OpConstant %float 9
%float_10 = OpConstant %float 10
%float_11 = OpConstant %float 11
%float_12 = OpConstant %float 12
%float_13 = OpConstant %float 13
%float_14 = OpConstant %float 14
%float_21 = OpConstant %float 21
%float_22 = OpConstant %float 22
%float_23 = OpConstant %float 23
%float_24 = OpConstant %float 24
%float_25 = OpConstant %float 25
%float_26 = OpConstant %float 26
%float_27 = OpConstant %float 27
%float_28 = OpConstant %float 28
%float_29 = OpConstant %float 29
%float_30 = OpConstant %float 30
%float_31 = OpConstant %float 31
%float_32 = OpConstant %float 32
%float_33 = OpConstant %float 33
%float_34 = OpConstant %float 34
%float_35 = OpConstant %float 35
%float_36 = OpConstant %float 36
%float_37 = OpConstant %float 37
%float_38 = OpConstant %float 38
%expected_1_2 = OpConstantComposite %v2float %float_1 %float_2
%expected_3_4 = OpConstantComposite %v2float %float_3 %float_4
%expected_6_7 = OpConstantComposite %v2float %float_6 %float_7
%expected_8_9 = OpConstantComposite %v2float %float_8 %float_9
%expected_10_11 = OpConstantComposite %v2float %float_10 %float_11
%expected_12_13 = OpConstantComposite %v2float %float_12 %float_13
%expected_21_22 = OpConstantComposite %v2float %float_21 %float_22
%expected_23_24 = OpConstantComposite %v2float %float_23 %float_24
%expected_26_27 = OpConstantComposite %v2float %float_26 %float_27
%expected_28_29 = OpConstantComposite %v2float %float_28 %float_29
%expected_30_31 = OpConstantComposite %v2float %float_30 %float_31
%expected_32_33 = OpConstantComposite %v2float %float_32 %float_33
%expected_35_36 = OpConstantComposite %v2float %float_35 %float_36
%expected_37_38 = OpConstantComposite %v2float %float_37 %float_38
%green = OpConstantComposite %v4float %float_0 %float_1 %float_0 %float_1
%red = OpConstantComposite %v4float %float_1 %float_0 %float_0 %float_1

%main = OpFunction %void None %functionType
%entry = OpLabel

%globalMatrixPointer = OpAccessChain %uniformMatrix2 %globalBuffer %int_0
%globalMatrixValue = OpLoad %matrix2 %globalMatrixPointer
%globalMatrixColumn0 = OpCompositeExtract %v2float %globalMatrixValue 0
%globalMatrixColumn1 = OpCompositeExtract %v2float %globalMatrixValue 1
%globalMatrixColumn0Equal = OpFOrdEqual %v2bool %globalMatrixColumn0 %expected_1_2
%globalMatrixColumn1Equal = OpFOrdEqual %v2bool %globalMatrixColumn1 %expected_3_4
%globalMatrixColumn0Matches = OpAll %bool %globalMatrixColumn0Equal
%globalMatrixColumn1Matches = OpAll %bool %globalMatrixColumn1Equal
%matches0 = OpLogicalAnd %bool %globalMatrixColumn0Matches %globalMatrixColumn1Matches

%globalSentinelPointer = OpAccessChain %uniformFloat %globalBuffer %int_1
%globalSentinelValue = OpLoad %float %globalSentinelPointer
%globalSentinelMatches = OpFOrdEqual %bool %globalSentinelValue %float_5
%matches1 = OpLogicalAnd %bool %matches0 %globalSentinelMatches

%globalArray0Pointer = OpAccessChain %uniformMatrix2 %globalBuffer %int_2 %int_0
%globalArray0Value = OpLoad %matrix2 %globalArray0Pointer
%globalArray0Column0 = OpCompositeExtract %v2float %globalArray0Value 0
%globalArray0Column1 = OpCompositeExtract %v2float %globalArray0Value 1
%globalArray0Column0Equal = OpFOrdEqual %v2bool %globalArray0Column0 %expected_6_7
%globalArray0Column1Equal = OpFOrdEqual %v2bool %globalArray0Column1 %expected_8_9
%globalArray0Column0Matches = OpAll %bool %globalArray0Column0Equal
%globalArray0Column1Matches = OpAll %bool %globalArray0Column1Equal
%matches2a = OpLogicalAnd %bool %globalArray0Column0Matches %globalArray0Column1Matches
%matches2 = OpLogicalAnd %bool %matches1 %matches2a

%globalArray1Pointer = OpAccessChain %uniformMatrix2 %globalBuffer %int_2 %int_1
%globalArray1Value = OpLoad %matrix2 %globalArray1Pointer
%globalArray1Column0 = OpCompositeExtract %v2float %globalArray1Value 0
%globalArray1Column1 = OpCompositeExtract %v2float %globalArray1Value 1
%globalArray1Column0Equal = OpFOrdEqual %v2bool %globalArray1Column0 %expected_10_11
%globalArray1Column1Equal = OpFOrdEqual %v2bool %globalArray1Column1 %expected_12_13
%globalArray1Column0Matches = OpAll %bool %globalArray1Column0Equal
%globalArray1Column1Matches = OpAll %bool %globalArray1Column1Equal
%matches3a = OpLogicalAnd %bool %globalArray1Column0Matches %globalArray1Column1Matches
%matches3 = OpLogicalAnd %bool %matches2 %matches3a

%globalTailPointer = OpAccessChain %uniformFloat %globalBuffer %int_3
%globalTailValue = OpLoad %float %globalTailPointer
%globalTailMatches = OpFOrdEqual %bool %globalTailValue %float_14
%matches4 = OpLogicalAnd %bool %matches3 %globalTailMatches

%stateMatrixPointer = OpAccessChain %uniformMatrix2 %stateBuffer %int_0
%stateMatrixValue = OpLoad %matrix2 %stateMatrixPointer
%stateMatrixColumn0 = OpCompositeExtract %v2float %stateMatrixValue 0
%stateMatrixColumn1 = OpCompositeExtract %v2float %stateMatrixValue 1
%stateMatrixColumn0Equal = OpFOrdEqual %v2bool %stateMatrixColumn0 %expected_21_22
%stateMatrixColumn1Equal = OpFOrdEqual %v2bool %stateMatrixColumn1 %expected_23_24
%stateMatrixColumn0Matches = OpAll %bool %stateMatrixColumn0Equal
%stateMatrixColumn1Matches = OpAll %bool %stateMatrixColumn1Equal
%stateMatches0 = OpLogicalAnd %bool %stateMatrixColumn0Matches %stateMatrixColumn1Matches

%stateSentinelPointer = OpAccessChain %uniformFloat %stateBuffer %int_1
%stateSentinelValue = OpLoad %float %stateSentinelPointer
%stateSentinelMatches = OpFOrdEqual %bool %stateSentinelValue %float_25
%stateMatches1 = OpLogicalAnd %bool %stateMatches0 %stateSentinelMatches

%stateArray0Pointer = OpAccessChain %uniformMatrix2 %stateBuffer %int_2 %int_0
%stateArray0Value = OpLoad %matrix2 %stateArray0Pointer
%stateArray0Column0 = OpCompositeExtract %v2float %stateArray0Value 0
%stateArray0Column1 = OpCompositeExtract %v2float %stateArray0Value 1
%stateArray0Column0Equal = OpFOrdEqual %v2bool %stateArray0Column0 %expected_26_27
%stateArray0Column1Equal = OpFOrdEqual %v2bool %stateArray0Column1 %expected_28_29
%stateArray0Column0Matches = OpAll %bool %stateArray0Column0Equal
%stateArray0Column1Matches = OpAll %bool %stateArray0Column1Equal
%stateArray0Matches = OpLogicalAnd %bool %stateArray0Column0Matches %stateArray0Column1Matches
%stateMatches2 = OpLogicalAnd %bool %stateMatches1 %stateArray0Matches

%stateArray1Pointer = OpAccessChain %uniformMatrix2 %stateBuffer %int_2 %int_1
%stateArray1Value = OpLoad %matrix2 %stateArray1Pointer
%stateArray1Column0 = OpCompositeExtract %v2float %stateArray1Value 0
%stateArray1Column1 = OpCompositeExtract %v2float %stateArray1Value 1
%stateArray1Column0Equal = OpFOrdEqual %v2bool %stateArray1Column0 %expected_30_31
%stateArray1Column1Equal = OpFOrdEqual %v2bool %stateArray1Column1 %expected_32_33
%stateArray1Column0Matches = OpAll %bool %stateArray1Column0Equal
%stateArray1Column1Matches = OpAll %bool %stateArray1Column1Equal
%stateArray1Matches = OpLogicalAnd %bool %stateArray1Column0Matches %stateArray1Column1Matches
%stateMatches3 = OpLogicalAnd %bool %stateMatches2 %stateArray1Matches

%stateTailPointer = OpAccessChain %uniformFloat %stateBuffer %int_3
%stateTailValue = OpLoad %float %stateTailPointer
%stateTailMatches = OpFOrdEqual %bool %stateTailValue %float_34
%stateMatches4 = OpLogicalAnd %bool %stateMatches3 %stateTailMatches

%stateFinalMatrixPointer = OpAccessChain %uniformMatrix2 %stateBuffer %int_4
%stateFinalMatrixValue = OpLoad %matrix2 %stateFinalMatrixPointer
%stateFinalMatrixColumn0 = OpCompositeExtract %v2float %stateFinalMatrixValue 0
%stateFinalMatrixColumn1 = OpCompositeExtract %v2float %stateFinalMatrixValue 1
%stateFinalMatrixColumn0Equal = OpFOrdEqual %v2bool %stateFinalMatrixColumn0 %expected_35_36
%stateFinalMatrixColumn1Equal = OpFOrdEqual %v2bool %stateFinalMatrixColumn1 %expected_37_38
%stateFinalMatrixColumn0Matches = OpAll %bool %stateFinalMatrixColumn0Equal
%stateFinalMatrixColumn1Matches = OpAll %bool %stateFinalMatrixColumn1Equal
%stateFinalMatrixMatches = OpLogicalAnd %bool %stateFinalMatrixColumn0Matches %stateFinalMatrixColumn1Matches
%stateMatches5 = OpLogicalAnd %bool %stateMatches4 %stateFinalMatrixMatches

%allMatches = OpLogicalAnd %bool %matches4 %stateMatches5
OpSelectionMerge %resultMerge None
OpBranchConditional %allMatches %successResult %failureResult
%successResult = OpLabel
OpStore %outputColor %green
OpBranch %resultMerge
%failureResult = OpLabel
OpStore %outputColor %red
OpBranch %resultMerge
%resultMerge = OpLabel
OpReturn
OpFunctionEnd
)";

// Build a compact matrix containing four distinct values in storage-vector order.
M2Float32 CreateVulkanMatrixStrideMatrix(float firstValue)
{
	M2Float32 matrix;
	for (size_t i = 0; i < matrix.size(); ++i)
	{
		matrix.data()[i] = firstValue + (float)i;
	}
	return matrix;
}

// Define the full fragment shader used by renderers which support resource arrays
const std::string FragmentShader = R"(
struct AlignmentEntry
{
    float scalarBefore;
    float2 vector2;
    float scalarAfterVector2;
    float3 vector3;
    float scalarAfterVector3;
    float4 vector4;
    row_major float2x2 matrix2;
    float scalarAfterMatrix2;
    row_major float3x3 matrix3;
    float2 vectorAfterMatrix3;
    row_major float4x4 matrix4;
    float scalarArray[2];
    float3 vector3Array[2];
    row_major float3x3 matrix3Array[2];
};

uniform float globalScalarBefore;
uniform float2 globalVector2;
uniform float globalScalarAfterVector2;
uniform float3 globalVector3;
uniform float globalScalarAfterVector3;
uniform float4 globalVector4;
uniform row_major float2x2 globalMatrix2;
uniform float globalScalarAfterMatrix2;
uniform row_major float3x3 globalMatrix3;
uniform float2 globalVectorAfterMatrix3;
uniform row_major float4x4 globalMatrix4;
uniform float globalScalarArray[2];
uniform float3 globalVector3Array[2];
uniform row_major float3x3 globalMatrix3Array[2];

cbuffer AlignmentStateBuffer
{
    float bufferScalarBefore;
    float2 bufferVector2;
    float bufferScalarAfterVector2;
    float3 bufferVector3;
    float bufferScalarAfterVector3;
    float4 bufferVector4;
    row_major float2x2 bufferMatrix2;
    float bufferScalarAfterMatrix2;
    row_major float3x3 bufferMatrix3;
    float2 bufferVectorAfterMatrix3;
    row_major float4x4 bufferMatrix4;
    float bufferScalarArray[2];
    float3 bufferVector3Array[2];
    row_major float3x3 bufferMatrix3Array[2];
};

StructuredBuffer<AlignmentEntry> structuredInput;
RWStructuredBuffer<AlignmentEntry> structuredOutput;
RWBuffer<float> alignmentResults;

void StoreEntry(uint firstResult, AlignmentEntry value)
{
    uint result = firstResult;
    alignmentResults[result++] = value.scalarBefore;
    alignmentResults[result++] = value.vector2.x;
    alignmentResults[result++] = value.vector2.y;
    alignmentResults[result++] = value.scalarAfterVector2;
    alignmentResults[result++] = value.vector3.x;
    alignmentResults[result++] = value.vector3.y;
    alignmentResults[result++] = value.vector3.z;
    alignmentResults[result++] = value.scalarAfterVector3;
    alignmentResults[result++] = value.vector4.x;
    alignmentResults[result++] = value.vector4.y;
    alignmentResults[result++] = value.vector4.z;
    alignmentResults[result++] = value.vector4.w;
    [unroll]
    for (uint row = 0; row < 2; ++row)
    {
        [unroll]
        for (uint column = 0; column < 2; ++column)
        {
            alignmentResults[result++] = value.matrix2[row][column];
        }
    }
    alignmentResults[result++] = value.scalarAfterMatrix2;
    [unroll]
    for (uint row = 0; row < 3; ++row)
    {
        [unroll]
        for (uint column = 0; column < 3; ++column)
        {
            alignmentResults[result++] = value.matrix3[row][column];
        }
    }
    alignmentResults[result++] = value.vectorAfterMatrix3.x;
    alignmentResults[result++] = value.vectorAfterMatrix3.y;
    [unroll]
    for (uint row = 0; row < 4; ++row)
    {
        [unroll]
        for (uint column = 0; column < 4; ++column)
        {
            alignmentResults[result++] = value.matrix4[row][column];
        }
    }
    [unroll]
    for (uint element = 0; element < 2; ++element)
    {
        alignmentResults[result++] = value.scalarArray[element];
    }
    [unroll]
    for (uint element = 0; element < 2; ++element)
    {
        alignmentResults[result++] = value.vector3Array[element].x;
        alignmentResults[result++] = value.vector3Array[element].y;
        alignmentResults[result++] = value.vector3Array[element].z;
    }
    [unroll]
    for (uint element = 0; element < 2; ++element)
    {
        [unroll]
        for (uint row = 0; row < 3; ++row)
        {
            [unroll]
            for (uint column = 0; column < 3; ++column)
            {
                alignmentResults[result++] = value.matrix3Array[element][row][column];
            }
        }
    }
}

void StoreGeneratedStructuredEntry(uint entryIndex, float firstValue)
{
    float value = firstValue;
    structuredOutput[entryIndex].scalarBefore = value++;
    structuredOutput[entryIndex].vector2.x = value++;
    structuredOutput[entryIndex].vector2.y = value++;
    structuredOutput[entryIndex].scalarAfterVector2 = value++;
    structuredOutput[entryIndex].vector3.x = value++;
    structuredOutput[entryIndex].vector3.y = value++;
    structuredOutput[entryIndex].vector3.z = value++;
    structuredOutput[entryIndex].scalarAfterVector3 = value++;
    structuredOutput[entryIndex].vector4.x = value++;
    structuredOutput[entryIndex].vector4.y = value++;
    structuredOutput[entryIndex].vector4.z = value++;
    structuredOutput[entryIndex].vector4.w = value++;
    [unroll]
    for (uint row = 0; row < 2; ++row)
    {
        [unroll]
        for (uint column = 0; column < 2; ++column)
        {
            structuredOutput[entryIndex].matrix2[row][column] = value++;
        }
    }
    structuredOutput[entryIndex].scalarAfterMatrix2 = value++;
    [unroll]
    for (uint row = 0; row < 3; ++row)
    {
        [unroll]
        for (uint column = 0; column < 3; ++column)
        {
            structuredOutput[entryIndex].matrix3[row][column] = value++;
        }
    }
    structuredOutput[entryIndex].vectorAfterMatrix3.x = value++;
    structuredOutput[entryIndex].vectorAfterMatrix3.y = value++;
    [unroll]
    for (uint row = 0; row < 4; ++row)
    {
        [unroll]
        for (uint column = 0; column < 4; ++column)
        {
            structuredOutput[entryIndex].matrix4[row][column] = value++;
        }
    }
    [unroll]
    for (uint element = 0; element < 2; ++element)
    {
        structuredOutput[entryIndex].scalarArray[element] = value++;
    }
    [unroll]
    for (uint element = 0; element < 2; ++element)
    {
        structuredOutput[entryIndex].vector3Array[element].x = value++;
        structuredOutput[entryIndex].vector3Array[element].y = value++;
        structuredOutput[entryIndex].vector3Array[element].z = value++;
    }
    [unroll]
    for (uint element = 0; element < 2; ++element)
    {
        [unroll]
        for (uint row = 0; row < 3; ++row)
        {
            [unroll]
            for (uint column = 0; column < 3; ++column)
            {
                structuredOutput[entryIndex].matrix3Array[element][row][column] = value++;
            }
        }
    }
}

float4 main(float4 position : SV_POSITION) : SV_TARGET0
{
    if ((uint(position.x) == 0) && (uint(position.y) == 0))
    {
        AlignmentEntry globalEntry;
        globalEntry.scalarBefore = globalScalarBefore;
        globalEntry.vector2 = globalVector2;
        globalEntry.scalarAfterVector2 = globalScalarAfterVector2;
        globalEntry.vector3 = globalVector3;
        globalEntry.scalarAfterVector3 = globalScalarAfterVector3;
        globalEntry.vector4 = globalVector4;
        globalEntry.matrix2 = globalMatrix2;
        globalEntry.scalarAfterMatrix2 = globalScalarAfterMatrix2;
        globalEntry.matrix3 = globalMatrix3;
        globalEntry.vectorAfterMatrix3 = globalVectorAfterMatrix3;
        globalEntry.matrix4 = globalMatrix4;
        globalEntry.scalarArray[0] = globalScalarArray[0];
        globalEntry.scalarArray[1] = globalScalarArray[1];
        globalEntry.vector3Array[0] = globalVector3Array[0];
        globalEntry.vector3Array[1] = globalVector3Array[1];
        globalEntry.matrix3Array[0] = globalMatrix3Array[0];
        globalEntry.matrix3Array[1] = globalMatrix3Array[1];
        StoreEntry(0, globalEntry);

        AlignmentEntry bufferEntry;
        bufferEntry.scalarBefore = bufferScalarBefore;
        bufferEntry.vector2 = bufferVector2;
        bufferEntry.scalarAfterVector2 = bufferScalarAfterVector2;
        bufferEntry.vector3 = bufferVector3;
        bufferEntry.scalarAfterVector3 = bufferScalarAfterVector3;
        bufferEntry.vector4 = bufferVector4;
        bufferEntry.matrix2 = bufferMatrix2;
        bufferEntry.scalarAfterMatrix2 = bufferScalarAfterMatrix2;
        bufferEntry.matrix3 = bufferMatrix3;
        bufferEntry.vectorAfterMatrix3 = bufferVectorAfterMatrix3;
        bufferEntry.matrix4 = bufferMatrix4;
        bufferEntry.scalarArray[0] = bufferScalarArray[0];
        bufferEntry.scalarArray[1] = bufferScalarArray[1];
        bufferEntry.vector3Array[0] = bufferVector3Array[0];
        bufferEntry.vector3Array[1] = bufferVector3Array[1];
        bufferEntry.matrix3Array[0] = bufferMatrix3Array[0];
        bufferEntry.matrix3Array[1] = bufferMatrix3Array[1];
        StoreEntry(70, bufferEntry);

        StoreEntry(140, structuredInput[1]);
        StoreGeneratedStructuredEntry(2, 4000.0f);
    }
    return float4(0.0f, 0.0f, 0.0f, 1.0f);
}
)";

// Define the fallback fragment shader used by renderers without resource-array support
const std::string FragmentShaderWithoutResourceArrays = R"(
uniform float globalScalarBefore;
uniform float2 globalVector2;
uniform float globalScalarAfterVector2;
uniform float3 globalVector3;
uniform float globalScalarAfterVector3;
uniform float4 globalVector4;
uniform row_major float2x2 globalMatrix2;
uniform float globalScalarAfterMatrix2;
uniform row_major float3x3 globalMatrix3;
uniform float2 globalVectorAfterMatrix3;
uniform row_major float4x4 globalMatrix4;
uniform float globalScalarArray[2];
uniform float3 globalVector3Array[2];
uniform row_major float3x3 globalMatrix3Array[2];

cbuffer AlignmentStateBuffer
{
    float bufferScalarBefore;
    float2 bufferVector2;
    float bufferScalarAfterVector2;
    float3 bufferVector3;
    float bufferScalarAfterVector3;
    float4 bufferVector4;
    row_major float2x2 bufferMatrix2;
    float bufferScalarAfterMatrix2;
    row_major float3x3 bufferMatrix3;
    float2 bufferVectorAfterMatrix3;
    row_major float4x4 bufferMatrix4;
    float bufferScalarArray[2];
    float3 bufferVector3Array[2];
    row_major float3x3 bufferMatrix3Array[2];
};

#define CHECK_ALIGNMENT_VALUE(VALUE) { if (valueIndex == targetIndex) matches = (VALUE) == expectedValue; expectedValue += 1.0f; ++valueIndex; }

bool GlobalValueMatches(uint targetIndex)
{
	uint valueIndex = 0;
    float expectedValue = 1000.0f;
    bool matches = false;
    CHECK_ALIGNMENT_VALUE(globalScalarBefore);
    CHECK_ALIGNMENT_VALUE(globalVector2.x);
    CHECK_ALIGNMENT_VALUE(globalVector2.y);
    CHECK_ALIGNMENT_VALUE(globalScalarAfterVector2);
    CHECK_ALIGNMENT_VALUE(globalVector3.x);
    CHECK_ALIGNMENT_VALUE(globalVector3.y);
    CHECK_ALIGNMENT_VALUE(globalVector3.z);
    CHECK_ALIGNMENT_VALUE(globalScalarAfterVector3);
    CHECK_ALIGNMENT_VALUE(globalVector4.x);
    CHECK_ALIGNMENT_VALUE(globalVector4.y);
    CHECK_ALIGNMENT_VALUE(globalVector4.z);
    CHECK_ALIGNMENT_VALUE(globalVector4.w);
    [unroll]
    for (uint row = 0; row < 2; ++row)
    {
        [unroll]
        for (uint column = 0; column < 2; ++column)
        {
            CHECK_ALIGNMENT_VALUE(globalMatrix2[row][column]);
        }
    }
    CHECK_ALIGNMENT_VALUE(globalScalarAfterMatrix2);
    [unroll]
    for (uint row = 0; row < 3; ++row)
    {
        [unroll]
        for (uint column = 0; column < 3; ++column)
        {
            CHECK_ALIGNMENT_VALUE(globalMatrix3[row][column]);
        }
    }
    CHECK_ALIGNMENT_VALUE(globalVectorAfterMatrix3.x);
    CHECK_ALIGNMENT_VALUE(globalVectorAfterMatrix3.y);
    [unroll]
    for (uint row = 0; row < 4; ++row)
    {
        [unroll]
        for (uint column = 0; column < 4; ++column)
        {
            CHECK_ALIGNMENT_VALUE(globalMatrix4[row][column]);
        }
    }
    [unroll]
    for (uint element = 0; element < 2; ++element)
    {
        CHECK_ALIGNMENT_VALUE(globalScalarArray[element]);
    }
    [unroll]
    for (uint element = 0; element < 2; ++element)
    {
        CHECK_ALIGNMENT_VALUE(globalVector3Array[element].x);
        CHECK_ALIGNMENT_VALUE(globalVector3Array[element].y);
        CHECK_ALIGNMENT_VALUE(globalVector3Array[element].z);
    }
    [unroll]
    for (uint element = 0; element < 2; ++element)
    {
        [unroll]
        for (uint row = 0; row < 3; ++row)
        {
            [unroll]
            for (uint column = 0; column < 3; ++column)
            {
                CHECK_ALIGNMENT_VALUE(globalMatrix3Array[element][row][column]);
            }
        }
    }
    return matches;
}

bool BufferValueMatches(uint targetIndex)
{
	uint valueIndex = 0;
    float expectedValue = 2000.0f;
    bool matches = false;
    CHECK_ALIGNMENT_VALUE(bufferScalarBefore);
    CHECK_ALIGNMENT_VALUE(bufferVector2.x);
    CHECK_ALIGNMENT_VALUE(bufferVector2.y);
    CHECK_ALIGNMENT_VALUE(bufferScalarAfterVector2);
    CHECK_ALIGNMENT_VALUE(bufferVector3.x);
    CHECK_ALIGNMENT_VALUE(bufferVector3.y);
    CHECK_ALIGNMENT_VALUE(bufferVector3.z);
    CHECK_ALIGNMENT_VALUE(bufferScalarAfterVector3);
    CHECK_ALIGNMENT_VALUE(bufferVector4.x);
    CHECK_ALIGNMENT_VALUE(bufferVector4.y);
    CHECK_ALIGNMENT_VALUE(bufferVector4.z);
    CHECK_ALIGNMENT_VALUE(bufferVector4.w);
    [unroll]
    for (uint row = 0; row < 2; ++row)
    {
        [unroll]
        for (uint column = 0; column < 2; ++column)
        {
            CHECK_ALIGNMENT_VALUE(bufferMatrix2[row][column]);
        }
    }
    CHECK_ALIGNMENT_VALUE(bufferScalarAfterMatrix2);
    [unroll]
    for (uint row = 0; row < 3; ++row)
    {
        [unroll]
        for (uint column = 0; column < 3; ++column)
        {
            CHECK_ALIGNMENT_VALUE(bufferMatrix3[row][column]);
        }
    }
    CHECK_ALIGNMENT_VALUE(bufferVectorAfterMatrix3.x);
    CHECK_ALIGNMENT_VALUE(bufferVectorAfterMatrix3.y);
    [unroll]
    for (uint row = 0; row < 4; ++row)
    {
        [unroll]
        for (uint column = 0; column < 4; ++column)
        {
            CHECK_ALIGNMENT_VALUE(bufferMatrix4[row][column]);
        }
    }
    [unroll]
    for (uint element = 0; element < 2; ++element)
    {
        CHECK_ALIGNMENT_VALUE(bufferScalarArray[element]);
    }
    [unroll]
    for (uint element = 0; element < 2; ++element)
    {
        CHECK_ALIGNMENT_VALUE(bufferVector3Array[element].x);
        CHECK_ALIGNMENT_VALUE(bufferVector3Array[element].y);
        CHECK_ALIGNMENT_VALUE(bufferVector3Array[element].z);
    }
    [unroll]
    for (uint element = 0; element < 2; ++element)
    {
        [unroll]
        for (uint row = 0; row < 3; ++row)
        {
            [unroll]
            for (uint column = 0; column < 3; ++column)
            {
                CHECK_ALIGNMENT_VALUE(bufferMatrix3Array[element][row][column]);
            }
        }
    }
    return matches;
}

float4 main(float4 position : SV_POSITION) : SV_TARGET0
{
    uint targetIndex = uint(position.x);
    if (targetIndex >= 140)
    {
        return float4(0.0f, 0.0f, 0.0f, 1.0f);
    }
    bool matches = false;
    if (targetIndex < 70)
    {
        matches = GlobalValueMatches(targetIndex);
    }
    else
    {
        matches = BufferValueMatches(targetIndex - 70);
    }
    return matches ? float4(0.0f, 1.0f, 0.0f, 1.0f) : float4(1.0f, 0.0f, 0.0f, 1.0f);
}
)";

// Define the tightly packed CPU representation of one complete alignment value set
struct AlignmentValues
{
	V1Float32 scalarBefore;
	V2Float32 vector2;
	V1Float32 scalarAfterVector2;
	V3Float32 vector3;
	V1Float32 scalarAfterVector3;
	V4Float32 vector4;
	M2Float32 matrix2;
	V1Float32 scalarAfterMatrix2;
	M3Float32 matrix3;
	V2Float32 vectorAfterMatrix3;
	M4Float32 matrix4;
	std::array<V1Float32, 2> scalarArray;
	std::array<V3Float32, 2> vector3Array;
	std::array<M3Float32, 2> matrix3Array;
};

// Ensure the CPU structure contains no implicit padding of its own
static_assert(sizeof(AlignmentValues) == AlignmentValueCount * sizeof(float));

// Create one value set with a distinct sequential value in every component
AlignmentValues CreateAlignmentValues(float firstValue)
{
	AlignmentValues result{};
	float value = firstValue;

	// Populate the interleaved scalar and vector fields
	result.scalarBefore = V1Float32(value++);
	result.vector2 = V2Float32(value, value + 1.0f);
	value += 2.0f;
	result.scalarAfterVector2 = V1Float32(value++);
	result.vector3 = V3Float32(value, value + 1.0f, value + 2.0f);
	value += 3.0f;
	result.scalarAfterVector3 = V1Float32(value++);
	result.vector4 = V4Float32(value, value + 1.0f, value + 2.0f, value + 3.0f);
	value += 4.0f;

	// Populate every component of each standalone matrix
	for (size_t i = 0; i < result.matrix2.size(); ++i)
	{
		result.matrix2.data()[i] = value++;
	}
	result.scalarAfterMatrix2 = V1Float32(value++);
	for (size_t i = 0; i < result.matrix3.size(); ++i)
	{
		result.matrix3.data()[i] = value++;
	}
	result.vectorAfterMatrix3 = V2Float32(value, value + 1.0f);
	value += 2.0f;
	for (size_t i = 0; i < result.matrix4.size(); ++i)
	{
		result.matrix4.data()[i] = value++;
	}

	// Populate every component of the scalar, vector, and matrix arrays
	for (auto& element : result.scalarArray)
	{
		element = V1Float32(value++);
	}
	for (auto& element : result.vector3Array)
	{
		element = V3Float32(value, value + 1.0f, value + 2.0f);
		value += 3.0f;
	}
	for (auto& matrix : result.matrix3Array)
	{
		for (size_t i = 0; i < matrix.size(); ++i)
		{
			matrix.data()[i] = value++;
		}
	}
	return result;
}

// Flatten a value set into the exact logical order used by the shaders
void AppendExpectedValues(std::vector<float>& output, const AlignmentValues& values)
{
	output.push_back(values.scalarBefore.X());
	output.insert(output.end(), values.vector2.data(), values.vector2.data() + values.vector2.size());
	output.push_back(values.scalarAfterVector2.X());
	output.insert(output.end(), values.vector3.data(), values.vector3.data() + values.vector3.size());
	output.push_back(values.scalarAfterVector3.X());
	output.insert(output.end(), values.vector4.data(), values.vector4.data() + values.vector4.size());
	output.insert(output.end(), values.matrix2.data(), values.matrix2.data() + values.matrix2.size());
	output.push_back(values.scalarAfterMatrix2.X());
	output.insert(output.end(), values.matrix3.data(), values.matrix3.data() + values.matrix3.size());
	output.insert(output.end(), values.vectorAfterMatrix3.data(), values.vectorAfterMatrix3.data() + values.vectorAfterMatrix3.size());
	output.insert(output.end(), values.matrix4.data(), values.matrix4.data() + values.matrix4.size());
	for (const auto& element : values.scalarArray)
	{
		output.push_back(element.X());
	}
	for (const auto& element : values.vector3Array)
	{
		output.insert(output.end(), element.data(), element.data() + element.size());
	}
	for (const auto& matrix : values.matrix3Array)
	{
		output.insert(output.end(), matrix.data(), matrix.data() + matrix.size());
	}
}

// IDataArray intentionally accepts raw entries rather than a member schema, so callers must provide bytes in the
// shader ABI used by the target API. Direct3D uses the compact structured layout, OpenGL uses std430 after HLSL-to-GLSL
// conversion, and Vulkan preserves glslang's relaxed HLSL member offsets with its reflected entry stride.
size_t GetStructuredAlignmentEntrySize(StructuredAlignmentLayout layout)
{
	return layout == StructuredAlignmentLayout::Compact ? sizeof(AlignmentValues) : 368;
}

// Build the byte offset table used to pack and unpack every logical structured-buffer value
std::array<size_t, AlignmentValueCount> GetStructuredAlignmentOffsets(StructuredAlignmentLayout layout)
{
	std::array<size_t, AlignmentValueCount> offsets{};

	// Direct3D structured buffers use the tightly packed CPU value order
	if (layout == StructuredAlignmentLayout::Compact)
	{
		for (size_t i = 0; i < offsets.size(); ++i)
		{
			offsets[i] = i * sizeof(float);
		}
		return offsets;
	}

	// Vulkan preserves the relaxed HLSL member offsets emitted by glslang
	if (layout == StructuredAlignmentLayout::VulkanHlsl)
	{
		offsets[0] = 0;
		offsets[1] = 4;
		offsets[2] = 8;
		offsets[3] = 12;
		for (size_t i = 0; i < 3; ++i)
		{
			offsets[4 + i] = 16 + (i * sizeof(float));
		}
		offsets[7] = 28;
		for (size_t i = 0; i < 4; ++i)
		{
			offsets[8 + i] = 32 + (i * sizeof(float));
			offsets[12 + i] = 48 + (i * sizeof(float));
		}
		offsets[16] = 64;
		for (size_t vector = 0; vector < 3; ++vector)
		{
			for (size_t component = 0; component < 3; ++component)
			{
				offsets[17 + (vector * 3) + component] = 80 + (vector * 16) + (component * sizeof(float));
			}
		}
		offsets[26] = 128;
		offsets[27] = 132;
		for (size_t i = 0; i < 16; ++i)
		{
			offsets[28 + i] = 144 + (i * sizeof(float));
		}
		offsets[44] = 208;
		offsets[45] = 212;
		for (size_t element = 0; element < 2; ++element)
		{
			for (size_t component = 0; component < 3; ++component)
			{
				offsets[46 + (element * 3) + component] = 224 + (element * 16) + (component * sizeof(float));
			}
		}
		for (size_t element = 0; element < 2; ++element)
		{
			for (size_t vector = 0; vector < 3; ++vector)
			{
				for (size_t component = 0; component < 3; ++component)
				{
					offsets[52 + (element * 9) + (vector * 3) + component] = 256 + (element * 48) + (vector * 16) + (component * sizeof(float));
				}
			}
		}
		return offsets;
	}

	// OpenGL storage buffers use the std430 member, array, and matrix strides
	offsets[0] = 0;
	offsets[1] = 8;
	offsets[2] = 12;
	offsets[3] = 16;
	for (size_t i = 0; i < 3; ++i)
	{
		offsets[4 + i] = 32 + (i * sizeof(float));
	}
	offsets[7] = 44;
	for (size_t i = 0; i < 4; ++i)
	{
		offsets[8 + i] = 48 + (i * sizeof(float));
		offsets[12 + i] = 64 + (i * sizeof(float));
	}
	offsets[16] = 80;
	for (size_t vector = 0; vector < 3; ++vector)
	{
		for (size_t component = 0; component < 3; ++component)
		{
			offsets[17 + (vector * 3) + component] = 96 + (vector * 16) + (component * sizeof(float));
		}
	}
	offsets[26] = 144;
	offsets[27] = 148;
	for (size_t i = 0; i < 16; ++i)
	{
		offsets[28 + i] = 160 + (i * sizeof(float));
	}
	offsets[44] = 224;
	offsets[45] = 228;
	for (size_t element = 0; element < 2; ++element)
	{
		for (size_t component = 0; component < 3; ++component)
		{
			offsets[46 + (element * 3) + component] = 240 + (element * 16) + (component * sizeof(float));
		}
	}
	for (size_t element = 0; element < 2; ++element)
	{
		for (size_t vector = 0; vector < 3; ++vector)
		{
			for (size_t component = 0; component < 3; ++component)
			{
				offsets[52 + (element * 9) + (vector * 3) + component] = 272 + (element * 48) + (vector * 16) + (component * sizeof(float));
			}
		}
	}
	return offsets;
}

// Pack all structured-buffer entries using the native layout for the active renderer
std::vector<uint8_t> PackStructuredAlignmentValues(const std::array<AlignmentValues, StructuredBufferEntryCount>& entries, StructuredAlignmentLayout layout)
{
	const size_t entrySize = GetStructuredAlignmentEntrySize(layout);
	const auto offsets = GetStructuredAlignmentOffsets(layout);
	std::vector<uint8_t> result(entrySize * entries.size(), 0);
	for (size_t entryIndex = 0; entryIndex < entries.size(); ++entryIndex)
	{
		std::vector<float> values;
		values.reserve(AlignmentValueCount);
		AppendExpectedValues(values, entries[entryIndex]);
		for (size_t valueIndex = 0; valueIndex < values.size(); ++valueIndex)
		{
			std::memcpy(result.data() + (entryIndex * entrySize) + offsets[valueIndex], &values[valueIndex], sizeof(float));
		}
	}
	return result;
}

// Extract every logical value from one captured structured-buffer entry
std::vector<float> UnpackStructuredAlignmentValues(const std::vector<uint8_t>& data, size_t entryIndex, StructuredAlignmentLayout layout)
{
	const size_t entrySize = GetStructuredAlignmentEntrySize(layout);
	const auto offsets = GetStructuredAlignmentOffsets(layout);
	std::vector<float> result(AlignmentValueCount);
	for (size_t valueIndex = 0; valueIndex < result.size(); ++valueIndex)
	{
		std::memcpy(&result[valueIndex], data.data() + (entryIndex * entrySize) + offsets[valueIndex], sizeof(float));
	}
	return result;
}

// Apply every value to either a state container or state buffer using its reflected IDs
template<class TargetType, class IdSourceType>
void SetAlignmentValues(TargetType& target, IdSourceType& idSource, const std::string& prefix, const AlignmentValues& values)
{
	// Apply all non-array scalar, vector, and matrix values
	target.SetStateValue(idSource.GetStateValueId(prefix + "ScalarBefore"), values.scalarBefore);
	target.SetStateValue(idSource.GetStateValueId(prefix + "Vector2"), values.vector2);
	target.SetStateValue(idSource.GetStateValueId(prefix + "ScalarAfterVector2"), values.scalarAfterVector2);
	target.SetStateValue(idSource.GetStateValueId(prefix + "Vector3"), values.vector3);
	target.SetStateValue(idSource.GetStateValueId(prefix + "ScalarAfterVector3"), values.scalarAfterVector3);
	target.SetStateValue(idSource.GetStateValueId(prefix + "Vector4"), values.vector4);
	target.SetStateValue(idSource.GetStateValueId(prefix + "Matrix2"), values.matrix2);
	target.SetStateValue(idSource.GetStateValueId(prefix + "ScalarAfterMatrix2"), values.scalarAfterMatrix2);
	target.SetStateValue(idSource.GetStateValueId(prefix + "Matrix3"), values.matrix3);
	target.SetStateValue(idSource.GetStateValueId(prefix + "VectorAfterMatrix3"), values.vectorAfterMatrix3);
	target.SetStateValue(idSource.GetStateValueId(prefix + "Matrix4"), values.matrix4);

	// Apply every scalar array element independently
	for (size_t i = 0; i < values.scalarArray.size(); ++i)
	{
		target.SetStateValue(idSource.GetStateValueId(prefix + "ScalarArray[]"), values.scalarArray[i], i);
	}

	// Apply every vector array element independently
	for (size_t i = 0; i < values.vector3Array.size(); ++i)
	{
		target.SetStateValue(idSource.GetStateValueId(prefix + "Vector3Array[]"), values.vector3Array[i], i);
	}

	// Apply every matrix array element independently
	for (size_t i = 0; i < values.matrix3Array.size(); ++i)
	{
		target.SetStateValue(idSource.GetStateValueId(prefix + "Matrix3Array[]"), values.matrix3Array[i], i);
	}
}

// Build the same state-buffer member sequence without using shader reflection
bool BuildAutomaticStateBufferLayout(IStateBufferLayout& layout)
{
	// Start an automatic layout definition
	if (!layout.BeginLayoutDefinition())
	{
		return false;
	}

	// Append the interleaved scalar, vector, and matrix fields
	layout.AppendField("bufferScalarBefore", IStateBufferLayout::DataType::Float32);
	layout.AppendVector("bufferVector2", IStateBufferLayout::DataType::Float32, 2);
	layout.AppendField("bufferScalarAfterVector2", IStateBufferLayout::DataType::Float32);
	layout.AppendVector("bufferVector3", IStateBufferLayout::DataType::Float32, 3);
	layout.AppendField("bufferScalarAfterVector3", IStateBufferLayout::DataType::Float32);
	layout.AppendVector("bufferVector4", IStateBufferLayout::DataType::Float32, 4);
	layout.AppendMatrix("bufferMatrix2", IStateBufferLayout::DataType::Float32, 2, 2);
	layout.AppendField("bufferScalarAfterMatrix2", IStateBufferLayout::DataType::Float32);
	layout.AppendMatrix("bufferMatrix3", IStateBufferLayout::DataType::Float32, 3, 3);
	layout.AppendVector("bufferVectorAfterMatrix3", IStateBufferLayout::DataType::Float32, 2);
	layout.AppendMatrix("bufferMatrix4", IStateBufferLayout::DataType::Float32, 4, 4);

	// Append representative scalar, vector, and matrix arrays
	layout.AppendField("bufferScalarArray[]", IStateBufferLayout::DataType::Float32, 2);
	layout.AppendVector("bufferVector3Array[]", IStateBufferLayout::DataType::Float32, 3, 2);
	layout.AppendMatrix("bufferMatrix3Array[]", IStateBufferLayout::DataType::Float32, 3, 3, 2);

	// Complete and validate the automatic layout
	return layout.ConstructStateLayout();
}

// Convert an integer pixel boundary to the corresponding clip-space coordinate
float PixelBoundaryToClipSpace(uint32_t coordinate, uint32_t extent)
{
	return ((2.0f * float(coordinate)) / float(extent)) - 1.0f;
}

// Append a non-indexed square with one constant result color to the visualization geometry
void AppendVisualizationSquare(std::vector<V4Float32>& positions, std::vector<V3Float32>& colors, float minX, float maxX, float minY, float maxY, const V3Float32& color)
{
	positions.emplace_back(minX, minY, 0.5f, 1.0f);
	positions.emplace_back(minX, maxY, 0.5f, 1.0f);
	positions.emplace_back(maxX, maxY, 0.5f, 1.0f);
	positions.emplace_back(minX, minY, 0.5f, 1.0f);
	positions.emplace_back(maxX, maxY, 0.5f, 1.0f);
	positions.emplace_back(maxX, minY, 0.5f, 1.0f);
	colors.insert(colors.end(), 6, color);
}

// Record one exact CPU comparison while preserving useful details for the first failure
void RecordAlignmentComparison(std::vector<AlignmentResult>& results, size_t resultIndex, float actual, float expected, const std::string& valueGroup, size_t valueIndex, size_t& failureCount, std::string& firstFailure)
{
	if (actual == expected)
	{
		results[resultIndex] = AlignmentResult::Passed;
		return;
	}

	results[resultIndex] = AlignmentResult::Failed;
	++failureCount;
	if (firstFailure.empty())
	{
		firstFailure = valueGroup + " value " + std::to_string(valueIndex) + " was " + std::to_string(actual) + ", expected " + std::to_string(expected) + ".";
	}
}

// Share the two alignment test paths between reflected and automatically constructed layouts
class BufferLayoutAlignmentTestBase : public UnitTestBase
{
public:
	explicit BufferLayoutAlignmentTestBase(const std::string& testFullName)
	: UnitTestBase(testFullName)
	{}

protected:
	bool RenderAlignmentVisualization(ITestSession& session, IFrameBuffer& frameBuffer, const std::vector<AlignmentResult>& results, uint32_t columnCount, const std::string& resultName, const std::string& description)
	{
		// Ensure every square has been deliberately classified before drawing it
		REQUIRE(!results.empty());
		REQUIRE(columnCount != 0);
		for (const auto result : results)
		{
			REQUIRE(result != AlignmentResult::Pending);
		}

		// Compile the simple color-pass-through visualization program
		auto& renderer = session.Renderer();
		auto shaderProgram = renderer.CreateShaderProgram();
		REQUIRE(shaderProgram->LoadShaderStage(IShaderProgram::ShaderStage::Vertex, ShaderSourceInfoHLSL(VisualizationVertexShader)));
		REQUIRE(shaderProgram->LoadShaderStage(IShaderProgram::ShaderStage::Fragment, ShaderSourceInfoHLSL(VisualizationFragmentShader)));
		REQUIRE(shaderProgram->CompileProgram());

		// Size and center an integer-pixel grid with a one-pixel black gap around every square
		const V2UInt32 windowSize = session.TestWindowSize();
		const uint32_t rowCount = ((uint32_t)results.size() + columnCount - 1) / columnCount;
		const uint32_t cellSize = std::min(windowSize.X() / columnCount, windowSize.Y() / rowCount);
		REQUIRE(cellSize >= 3);
		const uint32_t gridWidth = cellSize * columnCount;
		const uint32_t gridHeight = cellSize * rowCount;
		const uint32_t gridOriginX = (windowSize.X() - gridWidth) / 2;
		const uint32_t gridOriginY = (windowSize.Y() - gridHeight) / 2;
		const double squarePixelCount = double(cellSize - 2) * double(cellSize - 2);
		const double imagePixelCount = double(windowSize.X()) * double(windowSize.Y());
		const double imageDiffThreshold = 1.0 - (squarePixelCount / (4.0 * imagePixelCount));

		// Build one independently colored square for every component comparison
		std::vector<V4Float32> positions;
		std::vector<V3Float32> colors;
		positions.reserve(results.size() * 6);
		colors.reserve(results.size() * 6);
		for (size_t i = 0; i < results.size(); ++i)
		{
			const uint32_t column = (uint32_t)i % columnCount;
			const uint32_t row = (uint32_t)i / columnCount;
			const uint32_t minPixelX = gridOriginX + (column * cellSize) + 1;
			const uint32_t maxPixelX = gridOriginX + ((column + 1) * cellSize) - 1;
			const uint32_t minPixelY = gridOriginY + (row * cellSize) + 1;
			const uint32_t maxPixelY = gridOriginY + ((row + 1) * cellSize) - 1;
			const float minX = PixelBoundaryToClipSpace(minPixelX, windowSize.X());
			const float maxX = PixelBoundaryToClipSpace(maxPixelX, windowSize.X());
			const float minY = PixelBoundaryToClipSpace(minPixelY, windowSize.Y());
			const float maxY = PixelBoundaryToClipSpace(maxPixelY, windowSize.Y());
			const V3Float32 color = results[i] == AlignmentResult::Failed ? V3Float32(1.0f, 0.0f, 0.0f) : V3Float32(0.0f, 1.0f, 0.0f);
			AppendVisualizationSquare(positions, colors, minX, maxX, minY, maxY, color);
		}

		// Upload the positions and the independently selected result color for every square
		VertexAttribute<V4Float32> positionAttribute(positions.size(), IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadNever, IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadOften);
		VertexAttribute<V3Float32> colorAttribute(colors.size(), IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadNever, IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadOften);
		auto vertexBuffer = renderer.CreateVertexBuffer();
		REQUIRE(vertexBuffer->BindVertexAttribute(positionAttribute));
		REQUIRE(positionAttribute.SetInitialData(positions));
		REQUIRE(vertexBuffer->BindVertexAttribute(colorAttribute));
		REQUIRE(colorAttribute.SetInitialData(colors));
		REQUIRE(vertexBuffer->AllocateMemory());

		// Bind the geometry into a render tree with no culling or blending
		auto renderableNode = renderer.CreateRenderableNode();
		REQUIRE(renderableNode->BindVertexAttribute(positionAttribute, shaderProgram->GetVertexAttributeId("position")));
		REQUIRE(renderableNode->BindVertexAttribute(colorAttribute, shaderProgram->GetVertexAttributeId("color")));
		REQUIRE(renderableNode->SetPrimitiveMode(IRenderableNode::PrimitiveMode::Triangles));
		auto groupNode = renderer.CreateStateGroupNode();
		groupNode->SetPolygonCullMode(IStateGroupNode::PolygonCullMode::None);
		groupNode->AddChildNode(renderableNode.get());
		auto programNode = renderer.CreateProgramNode();
		REQUIRE(programNode->BindShaderProgram(shaderProgram.get()));
		programNode->AddChildNode(groupNode.get());
		auto renderPassNode = renderer.CreateRenderPassNode();
		renderPassNode->BindFrameBuffer(&frameBuffer);
		renderPassNode->SetAttachmentClearData(IFrameBuffer::AttachmentType::Color, 0, V4Float32(0.0f, 0.0f, 0.0f, 1.0f));
		renderPassNode->AddChildNode(programNode.get());
		renderer.SetRenderPasses(&renderPassNode, 1);

		// Capture the complete grid with a binary threshold above the score produced by one red square
		auto frameBufferCapture = renderer.CreateFrameBufferOutput();
		frameBufferCapture->SetDetachAfterCapture(true);
		frameBufferCapture->SetFrameBufferCaptureRegion(V2UInt32(0, 0), windowSize);
		frameBuffer.AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
		DrawOneFrame();
		REQUIRE(frameBufferCapture->HasCapturedOutput());
		session.AddTestImageResult(resultName, description, std::move(frameBufferCapture), IImageDiff::Algorithm::BinaryCountExact, imageDiffThreshold);
		renderer.RemoveAllRenderPasses();
		return true;
	}

	bool RunOpenGLReflectedMatrixStrideTest(ITestSession& session, IFrameBuffer& frameBuffer)
	{
		// Compile native GLSL which permits the driver to select its own uniform-block matrix stride
		auto& renderer = session.Renderer();
		auto shaderProgram = renderer.CreateShaderProgram();
		REQUIRE(shaderProgram->LoadShaderStage(IShaderProgram::ShaderStage::Vertex, ShaderSourceInfoGLSL(OpenGLMatrixStrideVertexShader)));
		REQUIRE(shaderProgram->LoadShaderStage(IShaderProgram::ShaderStage::Fragment, ShaderSourceInfoGLSL(OpenGLMatrixStrideFragmentShader)));
		REQUIRE(shaderProgram->CompileProgram());

		// Reflect the driver-selected state-buffer layout
		const auto stateBufferId = shaderProgram->GetStateBufferId("MatrixStrideStateBuffer");
		auto stateBufferLayout = renderer.CreateStateBufferLayout();
		REQUIRE(shaderProgram->LoadStateBufferLayoutFromShader(stateBufferId, stateBufferLayout.get()));

		// Allocate the state buffer using the reflected layout
		auto stateBuffer = renderer.CreateStateBuffer();
		REQUIRE(stateBuffer->BindBufferLayout(stateBufferLayout.get()));
		stateBuffer->SetPageSettings(1);
		REQUIRE(stateBuffer->AllocateMemory());

		// Populate every matrix component and each value following a matrix
		M2Float32 matrix2;
		for (size_t i = 0; i < matrix2.size(); ++i)
		{
			matrix2.data()[i] = float(i + 1);
		}
		M3Float32 matrix3;
		for (size_t i = 0; i < matrix3.size(); ++i)
		{
			matrix3.data()[i] = float(i + 6);
		}
		stateBuffer->SetStateValue(stateBuffer->GetStateValueId("matrix2"), matrix2);
		stateBuffer->SetStateValue(stateBuffer->GetStateValueId("scalarAfterMatrix2"), V1Float32(5.0f));
		stateBuffer->SetStateValue(stateBuffer->GetStateValueId("matrix3"), matrix3);
		stateBuffer->SetStateValue(stateBuffer->GetStateValueId("scalarAfterMatrix3"), V1Float32(15.0f));
		stateBuffer->SetStateValue(stateBuffer->GetStateValueId("tailValue"), V4Float32(16.0f, 17.0f, 18.0f, 19.0f));

		// Create a fullscreen quad to execute the fragment shader
		std::vector<V4Float32> positionData;
		Geometry().CreateFullscreenQuad(0.5f, positionData);
		VertexAttribute<V4Float32> positionAttribute(positionData.size(), IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadNever, IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadOften);
		auto vertexBuffer = renderer.CreateVertexBuffer();
		REQUIRE(vertexBuffer->BindVertexAttribute(positionAttribute));
		REQUIRE(positionAttribute.SetInitialData(positionData));
		REQUIRE(vertexBuffer->AllocateMemory());

		// Create the renderable node and bind the reflected state buffer
		auto renderableNode = renderer.CreateRenderableNode();
		REQUIRE(renderableNode->BindVertexAttribute(positionAttribute, shaderProgram->GetVertexAttributeId("position")));
		REQUIRE(renderableNode->SetPrimitiveMode(IRenderableNode::PrimitiveMode::Triangles));
		renderableNode->BindStateBuffer(stateBufferId, stateBuffer.get());

		// Assemble and bind the render tree
		auto groupNode = renderer.CreateStateGroupNode();
		groupNode->SetPolygonCullMode(IStateGroupNode::PolygonCullMode::None);
		groupNode->AddChildNode(renderableNode.get());
		auto programNode = renderer.CreateProgramNode();
		REQUIRE(programNode->BindShaderProgram(shaderProgram.get()));
		programNode->AddChildNode(groupNode.get());
		auto renderPassNode = renderer.CreateRenderPassNode();
		renderPassNode->BindFrameBuffer(&frameBuffer);
		renderPassNode->SetAttachmentClearData(IFrameBuffer::AttachmentType::Color, 0, V4Float32(0.0f, 0.0f, 0.0f, 1.0f));
		renderPassNode->AddChildNode(programNode.get());
		renderer.SetRenderPasses(&renderPassNode, 1);

		// Capture one independent status pixel for every reflected matrix-stride value
		auto frameBufferCapture = renderer.CreateFrameBufferOutput();
		frameBufferCapture->SetDetachAfterCapture(true);
		frameBufferCapture->SetFrameBufferCaptureRegion(V2UInt32(0, 0), V2UInt32(uint32_t(OpenGLMatrixStrideResultCount), 1));
		frameBuffer.AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
		DrawOneFrame();
		REQUIRE(frameBufferCapture->HasCapturedOutput());
		std::vector<V4UInt8> capturedPixels;
		REQUIRE(frameBufferCapture->ReadBufferData(capturedPixels));
		REQUIRE(capturedPixels.size() == OpenGLMatrixStrideResultCount);

		// Convert every shader comparison into a CPU-visible result without stopping at the first failure
		std::vector<AlignmentResult> results(OpenGLMatrixStrideResultCount, AlignmentResult::Pending);
		size_t failureCount = 0;
		size_t firstFailure = 0;
		for (size_t i = 0; i < capturedPixels.size(); ++i)
		{
			const auto& pixel = capturedPixels[i];
			if ((pixel.X() == 0) && (pixel.Y() == 255) && (pixel.Z() == 0) && (pixel.W() == 255))
			{
				results[i] = AlignmentResult::Passed;
			}
			else
			{
				results[i] = AlignmentResult::Failed;
				if (failureCount++ == 0)
				{
					firstFailure = i;
				}
			}
		}

		// Draw the shared per-value image before reporting the detailed manual result
		renderer.RemoveAllRenderPasses();
		REQUIRE(RenderAlignmentVisualization(session, frameBuffer, results, uint32_t(OpenGLMatrixStrideResultCount), "OpenGLReflectedMatrixStrideVisualization", "Nineteen squares show the individual driver-reflected matrix-stride comparisons; green means matched and red means failed."));
		if (failureCount != 0)
		{
			session.AddTestFailure("OpenGLReflectedMatrixStride", "Every matrix component and adjacent scalar must match using the matrix stride selected and reflected by the OpenGL driver.", "Comparison " + std::to_string(firstFailure) + " was the first of " + std::to_string(failureCount) + " failures.");
			return false;
		}
		session.AddTestSuccess("OpenGLReflectedMatrixStride", "Every matrix component and adjacent scalar matched using the matrix stride selected and reflected by the OpenGL driver.");
		return true;
	}

	bool RunVulkanMatrixStrideTest(ITestSession& session, IFrameBuffer& frameBuffer)
	{
		// Retrieve the renderer used by the explicit SPIR-V matrix-stride regression.
		auto& renderer = session.Renderer();

		// Compile the exact SPIR-V assembly whose MatrixStride and ArrayStride decorations are under test.
		auto shaderProgram = renderer.CreateShaderProgram();
		REQUIRE(shaderProgram->LoadShaderStage(IShaderProgram::ShaderStage::Vertex, ShaderSourceInfoSPIRVAssembly(VulkanMatrixStrideVertexShader)));
		REQUIRE(shaderProgram->LoadShaderStage(IShaderProgram::ShaderStage::Fragment, ShaderSourceInfoSPIRVAssembly(VulkanMatrixStrideFragmentShader)));
		REQUIRE(shaderProgram->CompileProgram());

		// Reflect and allocate the reusable state buffer, including the differently-strided final matrix.
		const auto stateBufferId = shaderProgram->GetStateBufferId("MatrixStrideStateBuffer");
		auto stateBufferLayout = renderer.CreateStateBufferLayout();
		REQUIRE(shaderProgram->LoadStateBufferLayoutFromShader(stateBufferId, stateBufferLayout.get()));
		auto stateBuffer = renderer.CreateStateBuffer();
		REQUIRE(stateBuffer->BindBufferLayout(stateBufferLayout.get()));
		stateBuffer->SetPageSettings(1);
		REQUIRE(stateBuffer->AllocateMemory());

		// Populate every state-buffer matrix vector, array element, and adjacent sentinel.
		const auto stateMatrixId = stateBuffer->GetStateValueId("stateMatrix");
		const auto stateSentinelId = stateBuffer->GetStateValueId("stateSentinel");
		const auto stateMatrixArrayId = stateBuffer->GetStateValueId("stateMatrixArray[]");
		const auto stateTailId = stateBuffer->GetStateValueId("stateTail");
		const auto stateFinalMatrixId = stateBuffer->GetStateValueId("stateFinalMatrix");
		REQUIRE(stateMatrixId != StateValueId::Null);
		REQUIRE(stateSentinelId != StateValueId::Null);
		REQUIRE(stateMatrixArrayId != StateValueId::Null);
		REQUIRE(stateTailId != StateValueId::Null);
		REQUIRE(stateFinalMatrixId != StateValueId::Null);
		stateBuffer->SetStateValue(stateMatrixId, CreateVulkanMatrixStrideMatrix(21.0f));
		stateBuffer->SetStateValue(stateSentinelId, V1Float32(25.0f));
		stateBuffer->SetStateValue(stateMatrixArrayId, CreateVulkanMatrixStrideMatrix(26.0f), 0);
		stateBuffer->SetStateValue(stateMatrixArrayId, CreateVulkanMatrixStrideMatrix(30.0f), 1);
		stateBuffer->SetStateValue(stateTailId, V1Float32(34.0f));
		stateBuffer->SetStateValue(stateFinalMatrixId, CreateVulkanMatrixStrideMatrix(35.0f));

		// Create the fullscreen geometry and bind the reflected state buffer.
		std::vector<V4Float32> positionData;
		Geometry().CreateFullscreenQuad(0.5f, positionData);
		VertexAttribute<V4Float32> positionAttribute(positionData.size(), IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadNever, IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadOften);
		auto vertexBuffer = renderer.CreateVertexBuffer();
		REQUIRE(vertexBuffer->BindVertexAttribute(positionAttribute));
		REQUIRE(positionAttribute.SetInitialData(positionData));
		REQUIRE(vertexBuffer->AllocateMemory());
		auto renderableNode = renderer.CreateRenderableNode();
		REQUIRE(renderableNode->BindVertexAttribute(positionAttribute, shaderProgram->GetVertexAttributeId("position")));
		REQUIRE(renderableNode->SetPrimitiveMode(IRenderableNode::PrimitiveMode::Triangles));
		renderableNode->BindStateBuffer(stateBufferId, stateBuffer.get());

		// Populate every global matrix vector, array element, and adjacent sentinel.
		auto groupNode = renderer.CreateStateGroupNode();
		const auto globalMatrixId = shaderProgram->GetStateValueId("globalMatrix");
		const auto globalSentinelId = shaderProgram->GetStateValueId("globalSentinel");
		const auto globalMatrixArrayId = shaderProgram->GetStateValueId("globalMatrixArray[]");
		const auto globalTailId = shaderProgram->GetStateValueId("globalTail");
		REQUIRE(globalMatrixId != StateValueId::Null);
		REQUIRE(globalSentinelId != StateValueId::Null);
		REQUIRE(globalMatrixArrayId != StateValueId::Null);
		REQUIRE(globalTailId != StateValueId::Null);
		groupNode->SetStateValue(globalMatrixId, CreateVulkanMatrixStrideMatrix(1.0f));
		groupNode->SetStateValue(globalSentinelId, V1Float32(5.0f));
		groupNode->SetStateValue(globalMatrixArrayId, CreateVulkanMatrixStrideMatrix(6.0f), 0);
		groupNode->SetStateValue(globalMatrixArrayId, CreateVulkanMatrixStrideMatrix(10.0f), 1);
		groupNode->SetStateValue(globalTailId, V1Float32(14.0f));
		groupNode->SetPolygonCullMode(IStateGroupNode::PolygonCullMode::None);
		groupNode->AddChildNode(renderableNode.get());

		// Assemble and bind the render tree.
		auto programNode = renderer.CreateProgramNode();
		REQUIRE(programNode->BindShaderProgram(shaderProgram.get()));
		programNode->AddChildNode(groupNode.get());
		auto renderPassNode = renderer.CreateRenderPassNode();
		renderPassNode->BindFrameBuffer(&frameBuffer);
		renderPassNode->SetAttachmentClearData(IFrameBuffer::AttachmentType::Color, 0, V4Float32(0.0f, 0.0f, 0.0f, 1.0f));
		renderPassNode->AddChildNode(programNode.get());
		renderer.SetRenderPasses(&renderPassNode, 1);

		// Capture the fullscreen green result produced only when every reflected write is correct. A stride failure turns
		// the entire image red, which is detected by the reference image comparison.
		auto frameBufferCapture = renderer.CreateFrameBufferOutput();
		frameBufferCapture->SetDetachAfterCapture(true);
		frameBuffer.AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
		DrawOneFrame();
		session.AddTestImageResult("VulkanMatrixStride", "A fullscreen green image showing that every global and reusable-buffer matrix component matched with independent reflected matrix and array strides.", std::move(frameBufferCapture), IImageDiff::Algorithm::BinaryCountExact, 0.99);

		// Remove the render tree after submitting the captured result for comparison.
		renderer.RemoveAllRenderPasses();
		return true;
	}

	bool RunStateAlignmentTestWithoutResourceArrays(ITestSession& session, IFrameBuffer& frameBuffer, bool useReflectedStateBufferLayout)
	{
		// Retrieve the renderer and compile the fallback shader program
		auto& renderer = session.Renderer();
		auto shaderProgram = renderer.CreateShaderProgram();
		REQUIRE(shaderProgram->LoadShaderStage(IShaderProgram::ShaderStage::Vertex, ShaderSourceInfoHLSL(VertexShader)));
		REQUIRE(shaderProgram->LoadShaderStage(IShaderProgram::ShaderStage::Fragment, ShaderSourceInfoHLSL(FragmentShaderWithoutResourceArrays)));
		REQUIRE(shaderProgram->CompileProgram());

		// Create distinct sequential values for the global uniforms and state buffer
		const AlignmentValues globalValues = CreateAlignmentValues(1000.0f);
		const AlignmentValues bufferValues = CreateAlignmentValues(2000.0f);

		// Create either the reflected or automatic state-buffer layout
		const auto stateBufferId = shaderProgram->GetStateBufferId("AlignmentStateBuffer");
		auto stateBufferLayout = renderer.CreateStateBufferLayout();
		if (useReflectedStateBufferLayout)
		{
			REQUIRE(shaderProgram->LoadStateBufferLayoutFromShader(stateBufferId, stateBufferLayout.get()));
		}
		else
		{
			REQUIRE(BuildAutomaticStateBufferLayout(*stateBufferLayout));
		}

		// Allocate the state buffer and populate every one of its fields
		auto stateBuffer = renderer.CreateStateBuffer();
		REQUIRE(stateBuffer->BindBufferLayout(stateBufferLayout.get()));
		stateBuffer->SetPageSettings(1);
		REQUIRE(stateBuffer->AllocateMemory());
		SetAlignmentValues(*stateBuffer, *stateBuffer, "buffer", bufferValues);

		// Create a fullscreen quad to execute the fragment shader
		std::vector<V4Float32> positionData;
		Geometry().CreateFullscreenQuad(0.5f, positionData);
		VertexAttribute<V4Float32> positionAttribute(positionData.size(), IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadNever, IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadOften);
		auto vertexBuffer = renderer.CreateVertexBuffer();
		REQUIRE(vertexBuffer->BindVertexAttribute(positionAttribute));
		REQUIRE(positionAttribute.SetInitialData(positionData));
		REQUIRE(vertexBuffer->AllocateMemory());

		// Create the renderable node and bind its state buffer
		auto renderableNode = renderer.CreateRenderableNode();
		REQUIRE(renderableNode->BindVertexAttribute(positionAttribute, shaderProgram->GetVertexAttributeId("position")));
		REQUIRE(renderableNode->SetPrimitiveMode(IRenderableNode::PrimitiveMode::Triangles));
		renderableNode->BindStateBuffer(stateBufferId, stateBuffer.get());

		// Create the state group and populate every loose uniform value
		auto groupNode = renderer.CreateStateGroupNode();
		SetAlignmentValues(*groupNode, *shaderProgram, "global", globalValues);
		groupNode->SetPolygonCullMode(IStateGroupNode::PolygonCullMode::None);
		groupNode->AddChildNode(renderableNode.get());

		// Assemble and bind the render tree
		auto programNode = renderer.CreateProgramNode();
		REQUIRE(programNode->BindShaderProgram(shaderProgram.get()));
		programNode->AddChildNode(groupNode.get());
		auto renderPassNode = renderer.CreateRenderPassNode();
		renderPassNode->BindFrameBuffer(&frameBuffer);
		renderPassNode->SetAttachmentClearData(IFrameBuffer::AttachmentType::Color, 0, V4Float32(0.0f, 0.0f, 0.0f, 1.0f));
		renderPassNode->AddChildNode(programNode.get());
		renderer.SetRenderPasses(&renderPassNode, 1);

		// Capture one status pixel for every supported loose-uniform and state-buffer comparison
		auto frameBufferCapture = renderer.CreateFrameBufferOutput();
		frameBufferCapture->SetDetachAfterCapture(true);
		const size_t supportedResultCount = AlignmentValueCount * 2;
		frameBufferCapture->SetFrameBufferCaptureRegion(V2UInt32(0, 0), V2UInt32(uint32_t(supportedResultCount), 1));
		frameBuffer.AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
		DrawOneFrame();

		// Read every individual shader comparison and mark unsupported structured-buffer comparisons as not applicable
		REQUIRE(frameBufferCapture->HasCapturedOutput());
		std::vector<V4UInt8> capturedPixels;
		REQUIRE(frameBufferCapture->ReadBufferData(capturedPixels));
		REQUIRE(capturedPixels.size() == supportedResultCount);
		std::vector<AlignmentResult> results(AlignmentVisualizationResultCount, AlignmentResult::NotApplicable);
		size_t failureCount = 0;
		size_t firstFailure = 0;
		for (size_t i = 0; i < capturedPixels.size(); ++i)
		{
			const auto& pixel = capturedPixels[i];
			if ((pixel.X() == 0) && (pixel.Y() == 255) && (pixel.Z() == 0) && (pixel.W() == 255))
			{
				results[i] = AlignmentResult::Passed;
			}
			else
			{
				results[i] = AlignmentResult::Failed;
				if (failureCount++ == 0)
				{
					firstFailure = i;
				}
			}
		}

		// Draw the shared 280-value image before reporting the detailed manual result
		renderer.RemoveAllRenderPasses();
		const std::string resultPrefix = useReflectedStateBufferLayout ? "Reflected" : "Automatic";
		REQUIRE(RenderAlignmentVisualization(session, frameBuffer, results, 20, resultPrefix + "BufferAlignmentVisualization", "A 20 by 14 grid shows all 280 individual alignment comparisons; unsupported structured-buffer comparisons are green."));
		if (failureCount != 0)
		{
			const std::string valueGroup = firstFailure < AlignmentValueCount ? "loose-uniform" : "state-buffer";
			const size_t valueIndex = firstFailure % AlignmentValueCount;
			session.AddTestFailure(resultPrefix + "StateAlignment", "Every supported loose-uniform and state-buffer scalar, vector component, matrix element, and array element must match in the shader.", "The " + valueGroup + " value " + std::to_string(valueIndex) + " was the first of " + std::to_string(failureCount) + " failures.");
			return false;
		}
		session.AddTestSuccess(useReflectedStateBufferLayout ? "ReflectedStateAlignment" : "AutomaticStateAlignment", "Every supported loose-uniform and state-buffer scalar, vector component, matrix element, and array element matched in the shader.");
		return true;
	}

	bool RunAlignmentTest(ITestSession& session, IFrameBuffer& frameBuffer, bool useReflectedStateBufferLayout)
	{
		// Retrieve the renderer and compile the full resource-array shader program
		auto& renderer = session.Renderer();
		auto shaderProgram = renderer.CreateShaderProgram();
		REQUIRE(shaderProgram->LoadShaderStage(IShaderProgram::ShaderStage::Vertex, ShaderSourceInfoHLSL(VertexShader)));
		REQUIRE(shaderProgram->LoadShaderStage(IShaderProgram::ShaderStage::Fragment, ShaderSourceInfoHLSL(FragmentShader)));
		REQUIRE(shaderProgram->CompileProgram());

		// Create a distinct sequential value range for every tested data source
		const AlignmentValues globalValues = CreateAlignmentValues(1000.0f);
		const AlignmentValues bufferValues = CreateAlignmentValues(2000.0f);
		const AlignmentValues structuredValues = CreateAlignmentValues(3000.0f);
		const AlignmentValues generatedStructuredValues = CreateAlignmentValues(4000.0f);

		// Select the structured-buffer ABI used by the active renderer family
		StructuredAlignmentLayout structuredLayout = StructuredAlignmentLayout::Compact;
		if (session.ApiFamily() == IRendererPlugin::ApiFamily::OpenGL)
		{
			structuredLayout = StructuredAlignmentLayout::Std430;
		}
		else if (session.ApiFamily() == IRendererPlugin::ApiFamily::Vulkan)
		{
			structuredLayout = StructuredAlignmentLayout::VulkanHlsl;
		}
		const size_t structuredEntrySize = GetStructuredAlignmentEntrySize(structuredLayout);

		// Pack and allocate the structured input buffer with data in a non-zero entry
		std::array<AlignmentValues, StructuredBufferEntryCount> structuredInputData{};
		structuredInputData[StructuredBufferTestEntry] = structuredValues;
		const auto packedStructuredInputData = PackStructuredAlignmentValues(structuredInputData, structuredLayout);
		auto structuredInputBuffer = renderer.CreateDataArray();
		structuredInputBuffer->SetBufferLayout(structuredEntrySize, structuredInputData.size());
		REQUIRE(structuredInputBuffer->SetInitialData(packedStructuredInputData.data(), packedStructuredInputData.size()));
		REQUIRE(structuredInputBuffer->AllocateMemory());

		// Allocate and attach a capture target to the writable structured buffer
		auto structuredOutputBuffer = renderer.CreateDataArray();
		structuredOutputBuffer->SetBufferLayout(structuredEntrySize, StructuredBufferEntryCount);
		const std::vector<uint8_t> initialStructuredOutputData(structuredEntrySize * StructuredBufferEntryCount, 0);
		REQUIRE(structuredOutputBuffer->SetInitialData(initialStructuredOutputData.data(), initialStructuredOutputData.size()));
		REQUIRE(structuredOutputBuffer->AllocateMemory());
		auto structuredOutputCapture = renderer.CreateDataArrayOutput();
		structuredOutputCapture->SetDetachAfterCapture(true);
		structuredOutputBuffer->AddOutputCaptureTarget(structuredOutputCapture.get());

		// Allocate the flat GPU result buffer used to record every shader read
		auto alignmentResultBuffer = renderer.CreateTexelArray();
		alignmentResultBuffer->SetBufferLayout(ITexelArray::ImageFormat::R, ITexelArray::DataFormat::Float32, AlignmentResultCount);
		REQUIRE(alignmentResultBuffer->AllocateMemory());
		auto alignmentResultCapture = renderer.CreateTexelArrayOutput();
		alignmentResultCapture->SetDetachAfterCapture(true);
		alignmentResultBuffer->AddOutputCaptureTarget(alignmentResultCapture.get());

		// Create either the reflected or automatic state-buffer layout
		const auto stateBufferId = shaderProgram->GetStateBufferId("AlignmentStateBuffer");
		auto stateBufferLayout = renderer.CreateStateBufferLayout();
		if (useReflectedStateBufferLayout)
		{
			REQUIRE(shaderProgram->LoadStateBufferLayoutFromShader(stateBufferId, stateBufferLayout.get()));
		}
		else
		{
			REQUIRE(BuildAutomaticStateBufferLayout(*stateBufferLayout));
		}

		// Allocate the state buffer and populate every one of its fields
		auto stateBuffer = renderer.CreateStateBuffer();
		REQUIRE(stateBuffer->BindBufferLayout(stateBufferLayout.get()));
		stateBuffer->SetPageSettings(1);
		REQUIRE(stateBuffer->AllocateMemory());
		SetAlignmentValues(*stateBuffer, *stateBuffer, "buffer", bufferValues);

		// Create a fullscreen quad to execute the fragment shader
		std::vector<V4Float32> positionData;
		Geometry().CreateFullscreenQuad(0.5f, positionData);
		VertexAttribute<V4Float32> positionAttribute(positionData.size(), IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadNever, IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadOften);
		auto vertexBuffer = renderer.CreateVertexBuffer();
		REQUIRE(vertexBuffer->BindVertexAttribute(positionAttribute));
		REQUIRE(positionAttribute.SetInitialData(positionData));
		REQUIRE(vertexBuffer->AllocateMemory());

		// Create the renderable node and bind its state buffer
		auto renderableNode = renderer.CreateRenderableNode();
		REQUIRE(renderableNode->BindVertexAttribute(positionAttribute, shaderProgram->GetVertexAttributeId("position")));
		REQUIRE(renderableNode->SetPrimitiveMode(IRenderableNode::PrimitiveMode::Triangles));
		renderableNode->BindStateBuffer(stateBufferId, stateBuffer.get());

		// Create the state group and bind every loose value and resource buffer
		auto groupNode = renderer.CreateStateGroupNode();
		SetAlignmentValues(*groupNode, *shaderProgram, "global", globalValues);
		groupNode->BindResourceArray(shaderProgram->GetResourceArrayId("structuredInput"), structuredInputBuffer.get());
		groupNode->BindResourceArray(shaderProgram->GetResourceArrayId("structuredOutput"), structuredOutputBuffer.get());
		groupNode->BindResourceArray(shaderProgram->GetResourceArrayId("alignmentResults"), alignmentResultBuffer.get());
		groupNode->SetPolygonCullMode(IStateGroupNode::PolygonCullMode::None);
		groupNode->AddChildNode(renderableNode.get());

		// Create the program node for the alignment shader
		auto programNode = renderer.CreateProgramNode();
		REQUIRE(programNode->BindShaderProgram(shaderProgram.get()));
		programNode->AddChildNode(groupNode.get());

		// Create and bind the render pass
		auto renderPassNode = renderer.CreateRenderPassNode();
		renderPassNode->BindFrameBuffer(&frameBuffer);
		renderPassNode->SetAttachmentClearData(IFrameBuffer::AttachmentType::Color, 0, V4Float32(0.0f, 0.0f, 0.0f, 1.0f));
		renderPassNode->AddChildNode(programNode.get());
		renderer.SetRenderPasses(&renderPassNode, 1);

		// Execute one frame to populate both output buffers
		DrawOneFrame();

		// Read the flat GPU results containing the loose, state-buffer, and structured-buffer values
		REQUIRE(alignmentResultCapture->HasCapturedOutput());
		REQUIRE(alignmentResultCapture->GetEntryCount() == AlignmentResultCount);
		std::vector<V1Float32> capturedResults;
		REQUIRE(alignmentResultCapture->ReadBufferData(capturedResults));
		REQUIRE(capturedResults.size() == AlignmentResultCount);

		// Build the expected flat sequence in the same order written by the shader
		std::vector<float> expectedResults;
		expectedResults.reserve(AlignmentResultCount);
		AppendExpectedValues(expectedResults, globalValues);
		AppendExpectedValues(expectedResults, bufferValues);
		AppendExpectedValues(expectedResults, structuredValues);
		REQUIRE(expectedResults.size() == capturedResults.size());

		// Verify every component read by the shader without stopping before later values are classified
		std::vector<AlignmentResult> results(AlignmentVisualizationResultCount, AlignmentResult::Pending);
		size_t failureCount = 0;
		std::string firstFailure;
		for (size_t i = 0; i < expectedResults.size(); ++i)
		{
			const size_t groupIndex = i / AlignmentValueCount;
			const std::string valueGroup = groupIndex == 0 ? "loose-uniform" : (groupIndex == 1 ? "state-buffer" : "structured-input");
			RecordAlignmentComparison(results, i, capturedResults[i].X(), expectedResults[i], valueGroup, i % AlignmentValueCount, failureCount, firstFailure);
		}

		// Capture and unpack the non-zero structured output entry written by the shader
		REQUIRE(structuredOutputCapture->HasCapturedOutput());
		REQUIRE(structuredOutputCapture->GetEntryCount() == StructuredBufferEntryCount);
		std::vector<uint8_t> capturedStructuredOutput(structuredEntrySize * StructuredBufferEntryCount);
		REQUIRE(structuredOutputCapture->ReadBufferData(capturedStructuredOutput.data(), capturedStructuredOutput.size()));
		std::vector<float> expectedStructuredOutput;
		AppendExpectedValues(expectedStructuredOutput, generatedStructuredValues);
		const auto actualStructuredOutput = UnpackStructuredAlignmentValues(capturedStructuredOutput, StructuredBufferOutputTestEntry, structuredLayout);
		REQUIRE(expectedStructuredOutput.size() == actualStructuredOutput.size());

		// Verify every scalar, vector component, matrix element, and array entry written by the shader
		for (size_t i = 0; i < expectedStructuredOutput.size(); ++i)
		{
			RecordAlignmentComparison(results, AlignmentResultCount + i, actualStructuredOutput[i], expectedStructuredOutput[i], "structured-output", i, failureCount, firstFailure);
		}

		// Draw the shared 280-value image before reporting the detailed manual result
		renderer.RemoveAllRenderPasses();
		const std::string resultPrefix = useReflectedStateBufferLayout ? "Reflected" : "Automatic";
		REQUIRE(RenderAlignmentVisualization(session, frameBuffer, results, 20, resultPrefix + "BufferAlignmentVisualization", "A 20 by 14 grid shows all 280 individual loose-uniform, state-buffer, structured-input, and structured-output comparisons."));
		if (failureCount != 0)
		{
			session.AddTestFailure(resultPrefix + "BufferAlignment", "Every scalar, vector component, matrix element, and array element must match after GPU reads and writes.", firstFailure + " There were " + std::to_string(failureCount) + " failures in total.");
			return false;
		}
		session.AddTestSuccess(resultPrefix + "BufferAlignment", "Every scalar, vector component, matrix element, and array element matched after GPU reads from loose uniforms, a state buffer, and a structured buffer, and after GPU writes to a structured buffer.");
		return true;
	}
};

} // namespace

DEFINE_UNIT_TEST_WITH_BASE("Shader/BufferLayoutAlignment", BufferLayoutAlignmentTestBase)
{
	// Retrieve our important objects
	auto& renderer = session.Renderer();
	auto& uiThread = session.UIThread();
	auto& testWindowInfo = *session.TestWindowPlatformInfo();

	// Define the framebuffer
	auto frameBuffer = renderer.CreateFrameBuffer();
	frameBuffer->DefineViewportRegion(V2UInt32(0, 0), session.TestWindowSize());
	REQUIRE(uiThread.InvokeSync([&] { return frameBuffer->BindWindow(testWindowInfo, IFrameBuffer::WindowDepthStencilMode::None); }));

	// Verify driver-selected matrix strides on OpenGL renderers
	if (session.ApiFamily() == IRendererPlugin::ApiFamily::OpenGL)
	{
		REQUIRE(RunOpenGLReflectedMatrixStrideTest(session, *frameBuffer));
	}
	else
	{
		session.AddTestSkipped("OpenGLReflectedMatrixStride", "Driver-selected native GLSL uniform-block packing is only applicable to OpenGL renderers.");
	}

	// Verify explicit SPIR-V matrix and array strides on Vulkan renderers
	if (session.ApiFamily() == IRendererPlugin::ApiFamily::Vulkan)
	{
		REQUIRE(RunVulkanMatrixStrideTest(session, *frameBuffer));
	}
	else
	{
		session.AddTestSkipped("VulkanMatrixStride", "Explicit SPIR-V matrix-stride reflection is only applicable to the Vulkan renderer.");
	}

	// Run the complete path when structured buffers are supported, otherwise run the loose-uniform and state-buffer
	// subset.
	if (session.Device().IsFeatureSupported(IGraphicsDevice::Feature::ResourceArrays))
	{
		REQUIRE(RunAlignmentTest(session, *frameBuffer, true));
		REQUIRE(RunAlignmentTest(session, *frameBuffer, false));
	}
	else
	{
		REQUIRE(RunStateAlignmentTestWithoutResourceArrays(session, *frameBuffer, true));
		REQUIRE(RunStateAlignmentTestWithoutResourceArrays(session, *frameBuffer, false));
	}
	return true;
}

} // namespace cobalt::graphics::testing
