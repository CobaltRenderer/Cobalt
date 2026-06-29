// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "StateBuffer.h"
#include <Cobalt/RendererInterface/RendererInterface.pkg>

using namespace cobalt::graphics;

//----------------------------------------------------------------------------------------
// Initialization methods
//----------------------------------------------------------------------------------------
Cobalt_Result Cobalt_StateBuffer_AllocateMemory(Cobalt_StateBuffer buffer)
{
	auto _this = reinterpret_cast<IStateBuffer*>(buffer);

	return _this->AllocateMemory() ? COBALT_SUCCESS : COBALT_FAILURE;
}

//----------------------------------------------------------------------------------------
// Usage methods
//----------------------------------------------------------------------------------------
void Cobalt_StateBuffer_SetPerformanceHints(Cobalt_StateBuffer buffer, Cobalt_StateBufferPerformanceHint performanceHintCpu, Cobalt_StateBufferPerformanceHint performanceHintGpu)
{
	auto _this = reinterpret_cast<IStateBuffer*>(buffer);

	_this->SetPerformanceHints((IStateBuffer::PerformanceHint)performanceHintCpu, (IStateBuffer::PerformanceHint)performanceHintGpu);
}

//----------------------------------------------------------------------------------------
// Format methods
//----------------------------------------------------------------------------------------
void Cobalt_StateBuffer_SetManualPageSize(Cobalt_StateBuffer buffer, size_t pageSizeInBytes)
{
	auto _this = reinterpret_cast<IStateBuffer*>(buffer);

	_this->SetManualPageSize(pageSizeInBytes);
}

//----------------------------------------------------------------------------------------
Cobalt_Result Cobalt_StateBuffer_BindBufferLayout(Cobalt_StateBuffer buffer, Cobalt_StateBufferLayout stateBufferLayout)
{
	auto _this = reinterpret_cast<IStateBuffer*>(buffer);

	return _this->BindBufferLayout(reinterpret_cast<IStateBufferLayout*>(stateBufferLayout)) ? COBALT_SUCCESS : COBALT_FAILURE;
}

//----------------------------------------------------------------------------------------
void Cobalt_StateBuffer_SetPageSettings(Cobalt_StateBuffer buffer, uint32_t initialPageCount, char allowDynamicResize)
{
	auto _this = reinterpret_cast<IStateBuffer*>(buffer);

	_this->SetPageSettings(initialPageCount, allowDynamicResize != 0);
}

//----------------------------------------------------------------------------------------
Cobalt_Result Cobalt_StateBuffer_ResizePageCount(Cobalt_StateBuffer buffer, uint32_t pageCount)
{
	auto _this = reinterpret_cast<IStateBuffer*>(buffer);

	return _this->ResizePageCount(pageCount) ? COBALT_SUCCESS : COBALT_FAILURE;
}

//----------------------------------------------------------------------------------------
uint32_t Cobalt_StateBuffer_GetStateValueId(Cobalt_StateBuffer buffer, const char* name, size_t nameLength)
{
	auto _this = reinterpret_cast<IStateBuffer*>(buffer);
	auto _name = std::string(name, nameLength);

	return (uint32_t)_this->GetStateValueId(_name);
}

//----------------------------------------------------------------------------------------
// State value methods
//----------------------------------------------------------------------------------------
void Cobalt_StateBuffer_SetStateValueBool(Cobalt_StateBuffer buffer, uint32_t stateId, char value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	auto _this = reinterpret_cast<IStateBuffer*>(buffer);

	_this->SetStateValue((StateValueId)stateId, value != 0, arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
void Cobalt_StateBuffer_SetStateValueV1Int8(Cobalt_StateBuffer buffer, uint32_t stateId, int8_t value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	auto _this = reinterpret_cast<IStateBuffer*>(buffer);

	_this->SetStateValue((StateValueId)stateId, V1Int8(value), arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
void Cobalt_StateBuffer_SetStateValueV1Int16(Cobalt_StateBuffer buffer, uint32_t stateId, int16_t value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	auto _this = reinterpret_cast<IStateBuffer*>(buffer);

	_this->SetStateValue((StateValueId)stateId, V1Int16(value), arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
void Cobalt_StateBuffer_SetStateValueV1Int32(Cobalt_StateBuffer buffer, uint32_t stateId, int32_t value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	auto _this = reinterpret_cast<IStateBuffer*>(buffer);

	_this->SetStateValue((StateValueId)stateId, V1Int32(value), arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
void Cobalt_StateBuffer_SetStateValueV1UInt8(Cobalt_StateBuffer buffer, uint32_t stateId, uint8_t value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	auto _this = reinterpret_cast<IStateBuffer*>(buffer);

	_this->SetStateValue((StateValueId)stateId, V1UInt8(value), arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
void Cobalt_StateBuffer_SetStateValueV1UInt16(Cobalt_StateBuffer buffer, uint32_t stateId, uint16_t value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	auto _this = reinterpret_cast<IStateBuffer*>(buffer);

	_this->SetStateValue((StateValueId)stateId, V1UInt16(value), arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
void Cobalt_StateBuffer_SetStateValueV1UInt32(Cobalt_StateBuffer buffer, uint32_t stateId, uint32_t value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	auto _this = reinterpret_cast<IStateBuffer*>(buffer);

	_this->SetStateValue((StateValueId)stateId, V1UInt32(value), arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
void Cobalt_StateBuffer_SetStateValueV1Float32(Cobalt_StateBuffer buffer, uint32_t stateId, float value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	auto _this = reinterpret_cast<IStateBuffer*>(buffer);

	_this->SetStateValue((StateValueId)stateId, V1Float32(value), arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
void Cobalt_StateBuffer_SetStateValueV1Float64(Cobalt_StateBuffer buffer, uint32_t stateId, double value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	auto _this = reinterpret_cast<IStateBuffer*>(buffer);

	_this->SetStateValue((StateValueId)stateId, V1Float64(value), arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
void Cobalt_StateBuffer_SetStateValueV2Int8(Cobalt_StateBuffer buffer, uint32_t stateId, const int8_t value[2], const size_t* arrayIndices, size_t arrayIndexCount)
{
	auto _this = reinterpret_cast<IStateBuffer*>(buffer);

	_this->SetStateValue((StateValueId)stateId, V2Int8(value[0], value[1]), arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
void Cobalt_StateBuffer_SetStateValueV2Int16(Cobalt_StateBuffer buffer, uint32_t stateId, const int16_t value[2], const size_t* arrayIndices, size_t arrayIndexCount)
{
	auto _this = reinterpret_cast<IStateBuffer*>(buffer);

	_this->SetStateValue((StateValueId)stateId, V2Int16(value[0], value[1]), arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
void Cobalt_StateBuffer_SetStateValueV2Int32(Cobalt_StateBuffer buffer, uint32_t stateId, const int32_t value[2], const size_t* arrayIndices, size_t arrayIndexCount)
{
	auto _this = reinterpret_cast<IStateBuffer*>(buffer);

	_this->SetStateValue((StateValueId)stateId, V2Int32(value[0], value[1]), arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
void Cobalt_StateBuffer_SetStateValueV2UInt8(Cobalt_StateBuffer buffer, uint32_t stateId, const uint8_t value[2], const size_t* arrayIndices, size_t arrayIndexCount)
{
	auto _this = reinterpret_cast<IStateBuffer*>(buffer);

	_this->SetStateValue((StateValueId)stateId, V2UInt8(value[0], value[1]), arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
void Cobalt_StateBuffer_SetStateValueV2UInt16(Cobalt_StateBuffer buffer, uint32_t stateId, const uint16_t value[2], const size_t* arrayIndices, size_t arrayIndexCount)
{
	auto _this = reinterpret_cast<IStateBuffer*>(buffer);

	_this->SetStateValue((StateValueId)stateId, V2UInt16(value[0], value[1]), arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
void Cobalt_StateBuffer_SetStateValueV2UInt32(Cobalt_StateBuffer buffer, uint32_t stateId, const uint32_t value[2], const size_t* arrayIndices, size_t arrayIndexCount)
{
	auto _this = reinterpret_cast<IStateBuffer*>(buffer);

	_this->SetStateValue((StateValueId)stateId, V2UInt32(value[0], value[1]), arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
void Cobalt_StateBuffer_SetStateValueV2Float32(Cobalt_StateBuffer buffer, uint32_t stateId, const float value[2], const size_t* arrayIndices, size_t arrayIndexCount)
{
	auto _this = reinterpret_cast<IStateBuffer*>(buffer);

	_this->SetStateValue((StateValueId)stateId, V2Float32(value[0], value[1]), arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
void Cobalt_StateBuffer_SetStateValueV2Float64(Cobalt_StateBuffer buffer, uint32_t stateId, const double value[2], const size_t* arrayIndices, size_t arrayIndexCount)
{
	auto _this = reinterpret_cast<IStateBuffer*>(buffer);

	_this->SetStateValue((StateValueId)stateId, V2Float64(value[0], value[1]), arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
void Cobalt_StateBuffer_SetStateValueV3Int8(Cobalt_StateBuffer buffer, uint32_t stateId, const int8_t value[3], const size_t* arrayIndices, size_t arrayIndexCount)
{
	auto _this = reinterpret_cast<IStateBuffer*>(buffer);

	_this->SetStateValue((StateValueId)stateId, V3Int8(value[0], value[1], value[2]), arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
void Cobalt_StateBuffer_SetStateValueV3Int16(Cobalt_StateBuffer buffer, uint32_t stateId, const int16_t value[3], const size_t* arrayIndices, size_t arrayIndexCount)
{
	auto _this = reinterpret_cast<IStateBuffer*>(buffer);

	_this->SetStateValue((StateValueId)stateId, V3Int16(value[0], value[1], value[2]), arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
void Cobalt_StateBuffer_SetStateValueV3Int32(Cobalt_StateBuffer buffer, uint32_t stateId, const int32_t value[3], const size_t* arrayIndices, size_t arrayIndexCount)
{
	auto _this = reinterpret_cast<IStateBuffer*>(buffer);

	_this->SetStateValue((StateValueId)stateId, V3Int32(value[0], value[1], value[2]), arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
void Cobalt_StateBuffer_SetStateValueV3UInt8(Cobalt_StateBuffer buffer, uint32_t stateId, const uint8_t value[3], const size_t* arrayIndices, size_t arrayIndexCount)
{
	auto _this = reinterpret_cast<IStateBuffer*>(buffer);

	_this->SetStateValue((StateValueId)stateId, V3UInt8(value[0], value[1], value[2]), arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
void Cobalt_StateBuffer_SetStateValueV3UInt16(Cobalt_StateBuffer buffer, uint32_t stateId, const uint16_t value[3], const size_t* arrayIndices, size_t arrayIndexCount)
{
	auto _this = reinterpret_cast<IStateBuffer*>(buffer);

	_this->SetStateValue((StateValueId)stateId, V3UInt16(value[0], value[1], value[2]), arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
void Cobalt_StateBuffer_SetStateValueV3UInt32(Cobalt_StateBuffer buffer, uint32_t stateId, const uint32_t value[3], const size_t* arrayIndices, size_t arrayIndexCount)
{
	auto _this = reinterpret_cast<IStateBuffer*>(buffer);

	_this->SetStateValue((StateValueId)stateId, V3UInt32(value[0], value[1], value[2]), arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
void Cobalt_StateBuffer_SetStateValueV3Float32(Cobalt_StateBuffer buffer, uint32_t stateId, const float value[3], const size_t* arrayIndices, size_t arrayIndexCount)
{
	auto _this = reinterpret_cast<IStateBuffer*>(buffer);

	_this->SetStateValue((StateValueId)stateId, V3Float32(value[0], value[1], value[2]), arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
void Cobalt_StateBuffer_SetStateValueV3Float64(Cobalt_StateBuffer buffer, uint32_t stateId, const double value[3], const size_t* arrayIndices, size_t arrayIndexCount)
{
	auto _this = reinterpret_cast<IStateBuffer*>(buffer);

	_this->SetStateValue((StateValueId)stateId, V3Float64(value[0], value[1], value[2]), arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
void Cobalt_StateBuffer_SetStateValueV4Int8(Cobalt_StateBuffer buffer, uint32_t stateId, const int8_t value[4], const size_t* arrayIndices, size_t arrayIndexCount)
{
	auto _this = reinterpret_cast<IStateBuffer*>(buffer);

	_this->SetStateValue((StateValueId)stateId, V4Int8(value[0], value[1], value[2], value[3]), arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
void Cobalt_StateBuffer_SetStateValueV4Int16(Cobalt_StateBuffer buffer, uint32_t stateId, const int16_t value[4], const size_t* arrayIndices, size_t arrayIndexCount)
{
	auto _this = reinterpret_cast<IStateBuffer*>(buffer);

	_this->SetStateValue((StateValueId)stateId, V4Int16(value[0], value[1], value[2], value[3]), arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
void Cobalt_StateBuffer_SetStateValueV4Int32(Cobalt_StateBuffer buffer, uint32_t stateId, const int32_t value[4], const size_t* arrayIndices, size_t arrayIndexCount)
{
	auto _this = reinterpret_cast<IStateBuffer*>(buffer);

	_this->SetStateValue((StateValueId)stateId, V4Int32(value[0], value[1], value[2], value[3]), arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
void Cobalt_StateBuffer_SetStateValueV4UInt8(Cobalt_StateBuffer buffer, uint32_t stateId, const uint8_t value[4], const size_t* arrayIndices, size_t arrayIndexCount)
{
	auto _this = reinterpret_cast<IStateBuffer*>(buffer);

	_this->SetStateValue((StateValueId)stateId, V4UInt8(value[0], value[1], value[2], value[3]), arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
void Cobalt_StateBuffer_SetStateValueV4UInt16(Cobalt_StateBuffer buffer, uint32_t stateId, const uint16_t value[4], const size_t* arrayIndices, size_t arrayIndexCount)
{
	auto _this = reinterpret_cast<IStateBuffer*>(buffer);

	_this->SetStateValue((StateValueId)stateId, V4UInt16(value[0], value[1], value[2], value[3]), arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
void Cobalt_StateBuffer_SetStateValueV4UInt32(Cobalt_StateBuffer buffer, uint32_t stateId, const uint32_t value[4], const size_t* arrayIndices, size_t arrayIndexCount)
{
	auto _this = reinterpret_cast<IStateBuffer*>(buffer);

	_this->SetStateValue((StateValueId)stateId, V4UInt32(value[0], value[1], value[2], value[3]), arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
void Cobalt_StateBuffer_SetStateValueV4Float32(Cobalt_StateBuffer buffer, uint32_t stateId, const float value[4], const size_t* arrayIndices, size_t arrayIndexCount)
{
	auto _this = reinterpret_cast<IStateBuffer*>(buffer);

	_this->SetStateValue((StateValueId)stateId, V4Float32(value[0], value[1], value[2], value[3]), arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
void Cobalt_StateBuffer_SetStateValueV4Float64(Cobalt_StateBuffer buffer, uint32_t stateId, const double value[4], const size_t* arrayIndices, size_t arrayIndexCount)
{
	auto _this = reinterpret_cast<IStateBuffer*>(buffer);

	_this->SetStateValue((StateValueId)stateId, V4Float64(value[0], value[1], value[2], value[3]), arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
void Cobalt_StateBuffer_SetStateValueM2Float32(Cobalt_StateBuffer buffer, uint32_t stateId, const float value[4], const size_t* arrayIndices, size_t arrayIndexCount)
{
	auto _this = reinterpret_cast<IStateBuffer*>(buffer);

	_this->SetStateValue((StateValueId)stateId, *reinterpret_cast<const M2Float32*>(value), arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
void Cobalt_StateBuffer_SetStateValueM3Float32(Cobalt_StateBuffer buffer, uint32_t stateId, const float value[9], const size_t* arrayIndices, size_t arrayIndexCount)
{
	auto _this = reinterpret_cast<IStateBuffer*>(buffer);

	_this->SetStateValue((StateValueId)stateId, *reinterpret_cast<const M3Float32*>(value), arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
void Cobalt_StateBuffer_SetStateValueM4Float32(Cobalt_StateBuffer buffer, uint32_t stateId, const float value[16], const size_t* arrayIndices, size_t arrayIndexCount)
{
	auto _this = reinterpret_cast<IStateBuffer*>(buffer);

	_this->SetStateValue((StateValueId)stateId, *reinterpret_cast<const M4Float32*>(value), arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
void Cobalt_StateBuffer_SetStateValueForPageBool(Cobalt_StateBuffer buffer, uint32_t pageNo, uint32_t stateId, char value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	auto _this = reinterpret_cast<IStateBuffer*>(buffer);

	_this->SetStateValueForPage(pageNo, (StateValueId)stateId, value != 0, arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
void Cobalt_StateBuffer_SetStateValueForPageV1Int8(Cobalt_StateBuffer buffer, uint32_t pageNo, uint32_t stateId, int8_t value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	auto _this = reinterpret_cast<IStateBuffer*>(buffer);

	_this->SetStateValueForPage(pageNo, (StateValueId)stateId, V1Int8(value), arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
void Cobalt_StateBuffer_SetStateValueForPageV1Int16(Cobalt_StateBuffer buffer, uint32_t pageNo, uint32_t stateId, int16_t value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	auto _this = reinterpret_cast<IStateBuffer*>(buffer);

	_this->SetStateValueForPage(pageNo, (StateValueId)stateId, V1Int16(value), arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
void Cobalt_StateBuffer_SetStateValueForPageV1Int32(Cobalt_StateBuffer buffer, uint32_t pageNo, uint32_t stateId, int32_t value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	auto _this = reinterpret_cast<IStateBuffer*>(buffer);

	_this->SetStateValueForPage(pageNo, (StateValueId)stateId, V1Int32(value), arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
void Cobalt_StateBuffer_SetStateValueForPageV1UInt8(Cobalt_StateBuffer buffer, uint32_t pageNo, uint32_t stateId, uint8_t value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	auto _this = reinterpret_cast<IStateBuffer*>(buffer);

	_this->SetStateValueForPage(pageNo, (StateValueId)stateId, V1UInt8(value), arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
void Cobalt_StateBuffer_SetStateValueForPageV1UInt16(Cobalt_StateBuffer buffer, uint32_t pageNo, uint32_t stateId, uint16_t value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	auto _this = reinterpret_cast<IStateBuffer*>(buffer);

	_this->SetStateValueForPage(pageNo, (StateValueId)stateId, V1UInt16(value), arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
void Cobalt_StateBuffer_SetStateValueForPageV1UInt32(Cobalt_StateBuffer buffer, uint32_t pageNo, uint32_t stateId, uint32_t value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	auto _this = reinterpret_cast<IStateBuffer*>(buffer);

	_this->SetStateValueForPage(pageNo, (StateValueId)stateId, V1UInt32(value), arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
void Cobalt_StateBuffer_SetStateValueForPageV1Float32(Cobalt_StateBuffer buffer, uint32_t pageNo, uint32_t stateId, float value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	auto _this = reinterpret_cast<IStateBuffer*>(buffer);

	_this->SetStateValueForPage(pageNo, (StateValueId)stateId, V1Float32(value), arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
void Cobalt_StateBuffer_SetStateValueForPageV1Float64(Cobalt_StateBuffer buffer, uint32_t pageNo, uint32_t stateId, double value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	auto _this = reinterpret_cast<IStateBuffer*>(buffer);

	_this->SetStateValueForPage(pageNo, (StateValueId)stateId, V1Float64(value), arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
void Cobalt_StateBuffer_SetStateValueForPageV2Int8(Cobalt_StateBuffer buffer, uint32_t pageNo, uint32_t stateId, const int8_t value[2], const size_t* arrayIndices, size_t arrayIndexCount)
{
	auto _this = reinterpret_cast<IStateBuffer*>(buffer);

	_this->SetStateValueForPage(pageNo, (StateValueId)stateId, V2Int8(value[0], value[1]), arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
void Cobalt_StateBuffer_SetStateValueForPageV2Int16(Cobalt_StateBuffer buffer, uint32_t pageNo, uint32_t stateId, const int16_t value[2], const size_t* arrayIndices, size_t arrayIndexCount)
{
	auto _this = reinterpret_cast<IStateBuffer*>(buffer);

	_this->SetStateValueForPage(pageNo, (StateValueId)stateId, V2Int16(value[0], value[1]), arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
void Cobalt_StateBuffer_SetStateValueForPageV2Int32(Cobalt_StateBuffer buffer, uint32_t pageNo, uint32_t stateId, const int32_t value[2], const size_t* arrayIndices, size_t arrayIndexCount)
{
	auto _this = reinterpret_cast<IStateBuffer*>(buffer);

	_this->SetStateValueForPage(pageNo, (StateValueId)stateId, V2Int32(value[0], value[1]), arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
void Cobalt_StateBuffer_SetStateValueForPageV2UInt8(Cobalt_StateBuffer buffer, uint32_t pageNo, uint32_t stateId, const uint8_t value[2], const size_t* arrayIndices, size_t arrayIndexCount)
{
	auto _this = reinterpret_cast<IStateBuffer*>(buffer);

	_this->SetStateValueForPage(pageNo, (StateValueId)stateId, V2UInt8(value[0], value[1]), arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
void Cobalt_StateBuffer_SetStateValueForPageV2UInt16(Cobalt_StateBuffer buffer, uint32_t pageNo, uint32_t stateId, const uint16_t value[2], const size_t* arrayIndices, size_t arrayIndexCount)
{
	auto _this = reinterpret_cast<IStateBuffer*>(buffer);

	_this->SetStateValueForPage(pageNo, (StateValueId)stateId, V2UInt16(value[0], value[1]), arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
void Cobalt_StateBuffer_SetStateValueForPageV2UInt32(Cobalt_StateBuffer buffer, uint32_t pageNo, uint32_t stateId, const uint32_t value[2], const size_t* arrayIndices, size_t arrayIndexCount)
{
	auto _this = reinterpret_cast<IStateBuffer*>(buffer);

	_this->SetStateValueForPage(pageNo, (StateValueId)stateId, V2UInt32(value[0], value[1]), arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
void Cobalt_StateBuffer_SetStateValueForPageV2Float32(Cobalt_StateBuffer buffer, uint32_t pageNo, uint32_t stateId, const float value[2], const size_t* arrayIndices, size_t arrayIndexCount)
{
	auto _this = reinterpret_cast<IStateBuffer*>(buffer);

	_this->SetStateValueForPage(pageNo, (StateValueId)stateId, V2Float32(value[0], value[1]), arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
void Cobalt_StateBuffer_SetStateValueForPageV2Float64(Cobalt_StateBuffer buffer, uint32_t pageNo, uint32_t stateId, const double value[2], const size_t* arrayIndices, size_t arrayIndexCount)
{
	auto _this = reinterpret_cast<IStateBuffer*>(buffer);

	_this->SetStateValueForPage(pageNo, (StateValueId)stateId, V2Float64(value[0], value[1]), arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
void Cobalt_StateBuffer_SetStateValueForPageV3Int8(Cobalt_StateBuffer buffer, uint32_t pageNo, uint32_t stateId, const int8_t value[3], const size_t* arrayIndices, size_t arrayIndexCount)
{
	auto _this = reinterpret_cast<IStateBuffer*>(buffer);

	_this->SetStateValueForPage(pageNo, (StateValueId)stateId, V3Int8(value[0], value[1], value[2]), arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
void Cobalt_StateBuffer_SetStateValueForPageV3Int16(Cobalt_StateBuffer buffer, uint32_t pageNo, uint32_t stateId, const int16_t value[3], const size_t* arrayIndices, size_t arrayIndexCount)
{
	auto _this = reinterpret_cast<IStateBuffer*>(buffer);

	_this->SetStateValueForPage(pageNo, (StateValueId)stateId, V3Int16(value[0], value[1], value[2]), arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
void Cobalt_StateBuffer_SetStateValueForPageV3Int32(Cobalt_StateBuffer buffer, uint32_t pageNo, uint32_t stateId, const int32_t value[3], const size_t* arrayIndices, size_t arrayIndexCount)
{
	auto _this = reinterpret_cast<IStateBuffer*>(buffer);

	_this->SetStateValueForPage(pageNo, (StateValueId)stateId, V3Int32(value[0], value[1], value[2]), arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
void Cobalt_StateBuffer_SetStateValueForPageV3UInt8(Cobalt_StateBuffer buffer, uint32_t pageNo, uint32_t stateId, const uint8_t value[3], const size_t* arrayIndices, size_t arrayIndexCount)
{
	auto _this = reinterpret_cast<IStateBuffer*>(buffer);

	_this->SetStateValueForPage(pageNo, (StateValueId)stateId, V3UInt8(value[0], value[1], value[2]), arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
void Cobalt_StateBuffer_SetStateValueForPageV3UInt16(Cobalt_StateBuffer buffer, uint32_t pageNo, uint32_t stateId, const uint16_t value[3], const size_t* arrayIndices, size_t arrayIndexCount)
{
	auto _this = reinterpret_cast<IStateBuffer*>(buffer);

	_this->SetStateValueForPage(pageNo, (StateValueId)stateId, V3UInt16(value[0], value[1], value[2]), arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
void Cobalt_StateBuffer_SetStateValueForPageV3UInt32(Cobalt_StateBuffer buffer, uint32_t pageNo, uint32_t stateId, const uint32_t value[3], const size_t* arrayIndices, size_t arrayIndexCount)
{
	auto _this = reinterpret_cast<IStateBuffer*>(buffer);

	_this->SetStateValueForPage(pageNo, (StateValueId)stateId, V3UInt32(value[0], value[1], value[2]), arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
void Cobalt_StateBuffer_SetStateValueForPageV3Float32(Cobalt_StateBuffer buffer, uint32_t pageNo, uint32_t stateId, const float value[3], const size_t* arrayIndices, size_t arrayIndexCount)
{
	auto _this = reinterpret_cast<IStateBuffer*>(buffer);

	_this->SetStateValueForPage(pageNo, (StateValueId)stateId, V3Float32(value[0], value[1], value[2]), arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
void Cobalt_StateBuffer_SetStateValueForPageV3Float64(Cobalt_StateBuffer buffer, uint32_t pageNo, uint32_t stateId, const double value[3], const size_t* arrayIndices, size_t arrayIndexCount)
{
	auto _this = reinterpret_cast<IStateBuffer*>(buffer);

	_this->SetStateValueForPage(pageNo, (StateValueId)stateId, V3Float64(value[0], value[1], value[2]), arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
void Cobalt_StateBuffer_SetStateValueForPageV4Int8(Cobalt_StateBuffer buffer, uint32_t pageNo, uint32_t stateId, const int8_t value[4], const size_t* arrayIndices, size_t arrayIndexCount)
{
	auto _this = reinterpret_cast<IStateBuffer*>(buffer);

	_this->SetStateValueForPage(pageNo, (StateValueId)stateId, V4Int8(value[0], value[1], value[2], value[3]), arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
void Cobalt_StateBuffer_SetStateValueForPageV4Int16(Cobalt_StateBuffer buffer, uint32_t pageNo, uint32_t stateId, const int16_t value[4], const size_t* arrayIndices, size_t arrayIndexCount)
{
	auto _this = reinterpret_cast<IStateBuffer*>(buffer);

	_this->SetStateValueForPage(pageNo, (StateValueId)stateId, V4Int16(value[0], value[1], value[2], value[3]), arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
void Cobalt_StateBuffer_SetStateValueForPageV4Int32(Cobalt_StateBuffer buffer, uint32_t pageNo, uint32_t stateId, const int32_t value[4], const size_t* arrayIndices, size_t arrayIndexCount)
{
	auto _this = reinterpret_cast<IStateBuffer*>(buffer);

	_this->SetStateValueForPage(pageNo, (StateValueId)stateId, V4Int32(value[0], value[1], value[2], value[3]), arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
void Cobalt_StateBuffer_SetStateValueForPageV4UInt8(Cobalt_StateBuffer buffer, uint32_t pageNo, uint32_t stateId, const uint8_t value[4], const size_t* arrayIndices, size_t arrayIndexCount)
{
	auto _this = reinterpret_cast<IStateBuffer*>(buffer);

	_this->SetStateValueForPage(pageNo, (StateValueId)stateId, V4UInt8(value[0], value[1], value[2], value[3]), arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
void Cobalt_StateBuffer_SetStateValueForPageV4UInt16(Cobalt_StateBuffer buffer, uint32_t pageNo, uint32_t stateId, const uint16_t value[4], const size_t* arrayIndices, size_t arrayIndexCount)
{
	auto _this = reinterpret_cast<IStateBuffer*>(buffer);

	_this->SetStateValueForPage(pageNo, (StateValueId)stateId, V4UInt16(value[0], value[1], value[2], value[3]), arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
void Cobalt_StateBuffer_SetStateValueForPageV4UInt32(Cobalt_StateBuffer buffer, uint32_t pageNo, uint32_t stateId, const uint32_t value[4], const size_t* arrayIndices, size_t arrayIndexCount)
{
	auto _this = reinterpret_cast<IStateBuffer*>(buffer);

	_this->SetStateValueForPage(pageNo, (StateValueId)stateId, V4UInt32(value[0], value[1], value[2], value[3]), arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
void Cobalt_StateBuffer_SetStateValueForPageV4Float32(Cobalt_StateBuffer buffer, uint32_t pageNo, uint32_t stateId, const float value[4], const size_t* arrayIndices, size_t arrayIndexCount)
{
	auto _this = reinterpret_cast<IStateBuffer*>(buffer);

	_this->SetStateValueForPage(pageNo, (StateValueId)stateId, V4Float32(value[0], value[1], value[2], value[3]), arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
void Cobalt_StateBuffer_SetStateValueForPageV4Float64(Cobalt_StateBuffer buffer, uint32_t pageNo, uint32_t stateId, const double value[4], const size_t* arrayIndices, size_t arrayIndexCount)
{
	auto _this = reinterpret_cast<IStateBuffer*>(buffer);

	_this->SetStateValueForPage(pageNo, (StateValueId)stateId, V4Float64(value[0], value[1], value[2], value[3]), arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
void Cobalt_StateBuffer_SetStateValueForPageM2Float32(Cobalt_StateBuffer buffer, uint32_t pageNo, uint32_t stateId, const float value[4], const size_t* arrayIndices, size_t arrayIndexCount)
{
	auto _this = reinterpret_cast<IStateBuffer*>(buffer);

	_this->SetStateValueForPage(pageNo, (StateValueId)stateId, *reinterpret_cast<const M2Float32*>(value), arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
void Cobalt_StateBuffer_SetStateValueForPageM3Float32(Cobalt_StateBuffer buffer, uint32_t pageNo, uint32_t stateId, const float value[9], const size_t* arrayIndices, size_t arrayIndexCount)
{
	auto _this = reinterpret_cast<IStateBuffer*>(buffer);

	_this->SetStateValueForPage(pageNo, (StateValueId)stateId, *reinterpret_cast<const M3Float32*>(value), arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
void Cobalt_StateBuffer_SetStateValueForPageM4Float32(Cobalt_StateBuffer buffer, uint32_t pageNo, uint32_t stateId, const float value[16], const size_t* arrayIndices, size_t arrayIndexCount)
{
	auto _this = reinterpret_cast<IStateBuffer*>(buffer);

	_this->SetStateValueForPage(pageNo, (StateValueId)stateId, *reinterpret_cast<const M4Float32*>(value), arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
void Cobalt_StateBuffer_SetRawPageData(Cobalt_StateBuffer buffer, uint32_t pageNo, const uint8_t* data, size_t dataSizeInBytes, size_t dataOffsetInBytes)
{
	auto _this = reinterpret_cast<IStateBuffer*>(buffer);

	_this->SetRawPageData(pageNo, data, dataSizeInBytes, dataOffsetInBytes);
}

//----------------------------------------------------------------------------------------
void Cobalt_StateBuffer_Delete(Cobalt_StateBuffer buffer)
{
	auto _this = reinterpret_cast<IStateBuffer*>(buffer);

	_this->Delete();
}
