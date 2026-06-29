// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "ProgramNode.h"
#include <Cobalt/RendererInterface/RendererInterface.pkg>

using namespace cobalt::graphics;

//----------------------------------------------------------------------------------------
// Child node methods
//----------------------------------------------------------------------------------------
void Cobalt_ProgramNode_AddChildNode(Cobalt_ProgramNode programNode, Cobalt_StateGroupNode childNode)
{
	auto _this = reinterpret_cast<IProgramNode*>(programNode);
	auto _node = reinterpret_cast<IStateGroupNode*>(childNode);

	_this->AddChildNode(_node);
}

//----------------------------------------------------------------------------------------
void Cobalt_ProgramNode_AddChildNodes(Cobalt_ProgramNode programNode, const Cobalt_StateGroupNode* childNodes, size_t childNodeCount)
{
	auto _this = reinterpret_cast<IProgramNode*>(programNode);
	auto _nodes = reinterpret_cast<IStateGroupNode* const*>(childNodes);

	_this->AddChildNodes(_nodes, childNodeCount);
}

//----------------------------------------------------------------------------------------
void Cobalt_ProgramNode_RemoveChildNode(Cobalt_ProgramNode programNode, Cobalt_StateGroupNode childNode)
{
	auto _this = reinterpret_cast<IProgramNode*>(programNode);
	auto _node = reinterpret_cast<IStateGroupNode*>(childNode);

	_this->RemoveChildNode(_node);
}

//----------------------------------------------------------------------------------------
void Cobalt_ProgramNode_RemoveChildNodes(Cobalt_ProgramNode programNode, const Cobalt_StateGroupNode* childNodes, size_t childNodeCount)
{
	auto _this = reinterpret_cast<IProgramNode*>(programNode);
	auto _nodes = reinterpret_cast<IStateGroupNode* const*>(childNodes);

	_this->RemoveChildNodes(_nodes, childNodeCount);
}

//----------------------------------------------------------------------------------------
void Cobalt_ProgramNode_RemoveAllChildNodes(Cobalt_ProgramNode programNode)
{
	auto _this = reinterpret_cast<IProgramNode*>(programNode);

	_this->RemoveAllChildNodes();
}

//----------------------------------------------------------------------------------------
void Cobalt_ProgramNode_SetChildNodes(Cobalt_ProgramNode programNode, const Cobalt_StateGroupNode* childNodes, size_t childNodeCount)
{
	auto _this = reinterpret_cast<IProgramNode*>(programNode);
	auto _nodes = reinterpret_cast<IStateGroupNode* const*>(childNodes);

	_this->SetChildNodes(_nodes, childNodeCount);
}

//----------------------------------------------------------------------------------------
// Shader program methods
//----------------------------------------------------------------------------------------
Cobalt_Result Cobalt_ProgramNode_BindShaderProgram(Cobalt_ProgramNode programNode, Cobalt_ShaderProgram shaderProgram)
{
	auto _this = reinterpret_cast<IProgramNode*>(programNode);
	auto _shader = reinterpret_cast<IShaderProgram*>(shaderProgram);

	return _this->BindShaderProgram(_shader) ? COBALT_SUCCESS : COBALT_FAILURE;
}

//----------------------------------------------------------------------------------------
// State value methods
//----------------------------------------------------------------------------------------
void Cobalt_ProgramNode_SetConstantStateValueBool(Cobalt_ProgramNode programNode, uint32_t stateId, char value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	auto _this = reinterpret_cast<IProgramNode*>(programNode);

	_this->SetConstantStateValue((StateValueId)stateId, value != 0, arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
void Cobalt_ProgramNode_SetConstantStateValueV1Int8(Cobalt_ProgramNode programNode, uint32_t stateId, int8_t value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	auto _this = reinterpret_cast<IProgramNode*>(programNode);

	_this->SetConstantStateValue((StateValueId)stateId, V1Int8(value), arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
void Cobalt_ProgramNode_SetConstantStateValueV1Int16(Cobalt_ProgramNode programNode, uint32_t stateId, int16_t value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	auto _this = reinterpret_cast<IProgramNode*>(programNode);

	_this->SetConstantStateValue((StateValueId)stateId, V1Int16(value), arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
void Cobalt_ProgramNode_SetConstantStateValueV1Int32(Cobalt_ProgramNode programNode, uint32_t stateId, int32_t value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	auto _this = reinterpret_cast<IProgramNode*>(programNode);

	_this->SetConstantStateValue((StateValueId)stateId, V1Int32(value), arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
void Cobalt_ProgramNode_SetConstantStateValueV1UInt8(Cobalt_ProgramNode programNode, uint32_t stateId, uint8_t value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	auto _this = reinterpret_cast<IProgramNode*>(programNode);

	_this->SetConstantStateValue((StateValueId)stateId, V1UInt8(value), arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
void Cobalt_ProgramNode_SetConstantStateValueV1UInt16(Cobalt_ProgramNode programNode, uint32_t stateId, uint16_t value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	auto _this = reinterpret_cast<IProgramNode*>(programNode);

	_this->SetConstantStateValue((StateValueId)stateId, V1UInt16(value), arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
void Cobalt_ProgramNode_SetConstantStateValueV1UInt32(Cobalt_ProgramNode programNode, uint32_t stateId, uint32_t value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	auto _this = reinterpret_cast<IProgramNode*>(programNode);

	_this->SetConstantStateValue((StateValueId)stateId, V1UInt32(value), arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
void Cobalt_ProgramNode_SetConstantStateValueV1Float32(Cobalt_ProgramNode programNode, uint32_t stateId, float value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	auto _this = reinterpret_cast<IProgramNode*>(programNode);

	_this->SetConstantStateValue((StateValueId)stateId, V1Float32(value), arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
void Cobalt_ProgramNode_SetConstantStateValueV1Float64(Cobalt_ProgramNode programNode, uint32_t stateId, double value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	auto _this = reinterpret_cast<IProgramNode*>(programNode);

	_this->SetConstantStateValue((StateValueId)stateId, V1Float64(value), arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
void Cobalt_ProgramNode_SetConstantStateValueV2Int8(Cobalt_ProgramNode programNode, uint32_t stateId, const int8_t value[2], const size_t* arrayIndices, size_t arrayIndexCount)
{
	auto _this = reinterpret_cast<IProgramNode*>(programNode);

	_this->SetConstantStateValue((StateValueId)stateId, V2Int8(value[0], value[1]), arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
void Cobalt_ProgramNode_SetConstantStateValueV2Int16(Cobalt_ProgramNode programNode, uint32_t stateId, const int16_t value[2], const size_t* arrayIndices, size_t arrayIndexCount)
{
	auto _this = reinterpret_cast<IProgramNode*>(programNode);

	_this->SetConstantStateValue((StateValueId)stateId, V2Int16(value[0], value[1]), arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
void Cobalt_ProgramNode_SetConstantStateValueV2Int32(Cobalt_ProgramNode programNode, uint32_t stateId, const int32_t value[2], const size_t* arrayIndices, size_t arrayIndexCount)
{
	auto _this = reinterpret_cast<IProgramNode*>(programNode);

	_this->SetConstantStateValue((StateValueId)stateId, V2Int32(value[0], value[1]), arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
void Cobalt_ProgramNode_SetConstantStateValueV2UInt8(Cobalt_ProgramNode programNode, uint32_t stateId, const uint8_t value[2], const size_t* arrayIndices, size_t arrayIndexCount)
{
	auto _this = reinterpret_cast<IProgramNode*>(programNode);

	_this->SetConstantStateValue((StateValueId)stateId, V2UInt8(value[0], value[1]), arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
void Cobalt_ProgramNode_SetConstantStateValueV2UInt16(Cobalt_ProgramNode programNode, uint32_t stateId, const uint16_t value[2], const size_t* arrayIndices, size_t arrayIndexCount)
{
	auto _this = reinterpret_cast<IProgramNode*>(programNode);

	_this->SetConstantStateValue((StateValueId)stateId, V2UInt16(value[0], value[1]), arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
void Cobalt_ProgramNode_SetConstantStateValueV2UInt32(Cobalt_ProgramNode programNode, uint32_t stateId, const uint32_t value[2], const size_t* arrayIndices, size_t arrayIndexCount)
{
	auto _this = reinterpret_cast<IProgramNode*>(programNode);

	_this->SetConstantStateValue((StateValueId)stateId, V2UInt32(value[0], value[1]), arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
void Cobalt_ProgramNode_SetConstantStateValueV2Float32(Cobalt_ProgramNode programNode, uint32_t stateId, const float value[2], const size_t* arrayIndices, size_t arrayIndexCount)
{
	auto _this = reinterpret_cast<IProgramNode*>(programNode);

	_this->SetConstantStateValue((StateValueId)stateId, V2Float32(value[0], value[1]), arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
void Cobalt_ProgramNode_SetConstantStateValueV2Float64(Cobalt_ProgramNode programNode, uint32_t stateId, const double value[2], const size_t* arrayIndices, size_t arrayIndexCount)
{
	auto _this = reinterpret_cast<IProgramNode*>(programNode);

	_this->SetConstantStateValue((StateValueId)stateId, V2Float64(value[0], value[1]), arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
void Cobalt_ProgramNode_SetConstantStateValueV3Int8(Cobalt_ProgramNode programNode, uint32_t stateId, const int8_t value[3], const size_t* arrayIndices, size_t arrayIndexCount)
{
	auto _this = reinterpret_cast<IProgramNode*>(programNode);

	_this->SetConstantStateValue((StateValueId)stateId, V3Int8(value[0], value[1], value[2]), arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
void Cobalt_ProgramNode_SetConstantStateValueV3Int16(Cobalt_ProgramNode programNode, uint32_t stateId, const int16_t value[3], const size_t* arrayIndices, size_t arrayIndexCount)
{
	auto _this = reinterpret_cast<IProgramNode*>(programNode);

	_this->SetConstantStateValue((StateValueId)stateId, V3Int16(value[0], value[1], value[2]), arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
void Cobalt_ProgramNode_SetConstantStateValueV3Int32(Cobalt_ProgramNode programNode, uint32_t stateId, const int32_t value[3], const size_t* arrayIndices, size_t arrayIndexCount)
{
	auto _this = reinterpret_cast<IProgramNode*>(programNode);

	_this->SetConstantStateValue((StateValueId)stateId, V3Int32(value[0], value[1], value[2]), arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
void Cobalt_ProgramNode_SetConstantStateValueV3UInt8(Cobalt_ProgramNode programNode, uint32_t stateId, const uint8_t value[3], const size_t* arrayIndices, size_t arrayIndexCount)
{
	auto _this = reinterpret_cast<IProgramNode*>(programNode);

	_this->SetConstantStateValue((StateValueId)stateId, V3UInt8(value[0], value[1], value[2]), arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
void Cobalt_ProgramNode_SetConstantStateValueV3UInt16(Cobalt_ProgramNode programNode, uint32_t stateId, const uint16_t value[3], const size_t* arrayIndices, size_t arrayIndexCount)
{
	auto _this = reinterpret_cast<IProgramNode*>(programNode);

	_this->SetConstantStateValue((StateValueId)stateId, V3UInt16(value[0], value[1], value[2]), arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
void Cobalt_ProgramNode_SetConstantStateValueV3UInt32(Cobalt_ProgramNode programNode, uint32_t stateId, const uint32_t value[3], const size_t* arrayIndices, size_t arrayIndexCount)
{
	auto _this = reinterpret_cast<IProgramNode*>(programNode);

	_this->SetConstantStateValue((StateValueId)stateId, V3UInt32(value[0], value[1], value[2]), arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
void Cobalt_ProgramNode_SetConstantStateValueV3Float32(Cobalt_ProgramNode programNode, uint32_t stateId, const float value[3], const size_t* arrayIndices, size_t arrayIndexCount)
{
	auto _this = reinterpret_cast<IProgramNode*>(programNode);

	_this->SetConstantStateValue((StateValueId)stateId, V3Float32(value[0], value[1], value[2]), arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
void Cobalt_ProgramNode_SetConstantStateValueV3Float64(Cobalt_ProgramNode programNode, uint32_t stateId, const double value[3], const size_t* arrayIndices, size_t arrayIndexCount)
{
	auto _this = reinterpret_cast<IProgramNode*>(programNode);

	_this->SetConstantStateValue((StateValueId)stateId, V3Float64(value[0], value[1], value[2]), arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
void Cobalt_ProgramNode_SetConstantStateValueV4Int8(Cobalt_ProgramNode programNode, uint32_t stateId, const int8_t value[4], const size_t* arrayIndices, size_t arrayIndexCount)
{
	auto _this = reinterpret_cast<IProgramNode*>(programNode);

	_this->SetConstantStateValue((StateValueId)stateId, V4Int8(value[0], value[1], value[2], value[3]), arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
void Cobalt_ProgramNode_SetConstantStateValueV4Int16(Cobalt_ProgramNode programNode, uint32_t stateId, const int16_t value[4], const size_t* arrayIndices, size_t arrayIndexCount)
{
	auto _this = reinterpret_cast<IProgramNode*>(programNode);

	_this->SetConstantStateValue((StateValueId)stateId, V4Int16(value[0], value[1], value[2], value[3]), arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
void Cobalt_ProgramNode_SetConstantStateValueV4Int32(Cobalt_ProgramNode programNode, uint32_t stateId, const int32_t value[4], const size_t* arrayIndices, size_t arrayIndexCount)
{
	auto _this = reinterpret_cast<IProgramNode*>(programNode);

	_this->SetConstantStateValue((StateValueId)stateId, V4Int32(value[0], value[1], value[2], value[3]), arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
void Cobalt_ProgramNode_SetConstantStateValueV4UInt8(Cobalt_ProgramNode programNode, uint32_t stateId, const uint8_t value[4], const size_t* arrayIndices, size_t arrayIndexCount)
{
	auto _this = reinterpret_cast<IProgramNode*>(programNode);

	_this->SetConstantStateValue((StateValueId)stateId, V4UInt8(value[0], value[1], value[2], value[3]), arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
void Cobalt_ProgramNode_SetConstantStateValueV4UInt16(Cobalt_ProgramNode programNode, uint32_t stateId, const uint16_t value[4], const size_t* arrayIndices, size_t arrayIndexCount)
{
	auto _this = reinterpret_cast<IProgramNode*>(programNode);

	_this->SetConstantStateValue((StateValueId)stateId, V4UInt16(value[0], value[1], value[2], value[3]), arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
void Cobalt_ProgramNode_SetConstantStateValueV4UInt32(Cobalt_ProgramNode programNode, uint32_t stateId, const uint32_t value[4], const size_t* arrayIndices, size_t arrayIndexCount)
{
	auto _this = reinterpret_cast<IProgramNode*>(programNode);

	_this->SetConstantStateValue((StateValueId)stateId, V4UInt32(value[0], value[1], value[2], value[3]), arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
void Cobalt_ProgramNode_SetConstantStateValueV4Float32(Cobalt_ProgramNode programNode, uint32_t stateId, const float value[4], const size_t* arrayIndices, size_t arrayIndexCount)
{
	auto _this = reinterpret_cast<IProgramNode*>(programNode);

	_this->SetConstantStateValue((StateValueId)stateId, V4Float32(value[0], value[1], value[2], value[3]), arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
void Cobalt_ProgramNode_SetConstantStateValueV4Float64(Cobalt_ProgramNode programNode, uint32_t stateId, const double value[4], const size_t* arrayIndices, size_t arrayIndexCount)
{
	auto _this = reinterpret_cast<IProgramNode*>(programNode);

	_this->SetConstantStateValue((StateValueId)stateId, V4Float64(value[0], value[1], value[2], value[3]), arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
void Cobalt_ProgramNode_SetConstantStateValueM2Float32(Cobalt_ProgramNode programNode, uint32_t stateId, const float value[4], const size_t* arrayIndices, size_t arrayIndexCount)
{
	auto _this = reinterpret_cast<IProgramNode*>(programNode);

	_this->SetConstantStateValue((StateValueId)stateId, *reinterpret_cast<const M2Float32*>(value), arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
void Cobalt_ProgramNode_SetConstantStateValueM3Float32(Cobalt_ProgramNode programNode, uint32_t stateId, const float value[9], const size_t* arrayIndices, size_t arrayIndexCount)
{
	auto _this = reinterpret_cast<IProgramNode*>(programNode);

	_this->SetConstantStateValue((StateValueId)stateId, *reinterpret_cast<const M3Float32*>(value), arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
void Cobalt_ProgramNode_SetConstantStateValueM4Float32(Cobalt_ProgramNode programNode, uint32_t stateId, const float value[16], const size_t* arrayIndices, size_t arrayIndexCount)
{
	auto _this = reinterpret_cast<IProgramNode*>(programNode);

	_this->SetConstantStateValue((StateValueId)stateId, *reinterpret_cast<const M4Float32*>(value), arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
void Cobalt_ProgramNode_ResetConstantStateValue(Cobalt_ProgramNode programNode, uint32_t stateId, const size_t* arrayIndices, size_t arrayIndexCount)
{
	auto _this = reinterpret_cast<IProgramNode*>(programNode);

	_this->ResetConstantStateValue((StateValueId)stateId, arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
// Debug methods
//----------------------------------------------------------------------------------------
void Cobalt_ProgramNode_SetDebugName(Cobalt_ProgramNode programNode, const char* name, size_t nameLength)
{
	auto _this = reinterpret_cast<IProgramNode*>(programNode);
	auto _name = std::string(name, nameLength);

	_this->SetDebugName(_name);
}

//----------------------------------------------------------------------------------------
// Initialization methods
//----------------------------------------------------------------------------------------
void Cobalt_ProgramNode_Delete(Cobalt_ProgramNode programNode)
{
	auto _this = reinterpret_cast<IProgramNode*>(programNode);

	_this->Delete();
}
