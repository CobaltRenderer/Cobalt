// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include "Deleter.h"
#include "IStateBufferLayout.h"
#include "MatrixTypes.h"
#include "SuccessToken.h"
#include "Tokens.h"
#include "VectorTypes.h"
#include <Cobalt/Marshalling/Marshalling.pkg>
#include <memory>
#include <string>
namespace cobalt { namespace graphics {
using namespace cobalt::marshalling::operators;

class IStateBuffer
{
public:
	// Enumerations
	enum class PerformanceHint : uint32_t
	{
		Default = 0x00000000,
		ReadNever = 0x00000001,
		ReadRarely = 0x00000002,
		ReadOften = 0x00000004,
		ReadFlagsMask = 0x000000FF,
		WriteNever = 0x00000100,
		WriteRarely = 0x00000200,
		WriteOften = 0x00000400,
		WriteFlagsMask = 0x0000FF00,
	};

	// Typedefs
	typedef std::unique_ptr<IStateBuffer, Deleter<IStateBuffer>> unique_ptr;

public:
	// Initialization methods
	virtual void Delete() = 0;
	virtual SuccessToken AllocateMemory() = 0;

	// Usage methods
	virtual void SetPerformanceHints(PerformanceHint performanceHintCpu, PerformanceHint performanceHintGpu) = 0;

	// Format methods
	virtual void SetManualPageSize(size_t pageSizeInBytes) = 0;
	virtual SuccessToken BindBufferLayout(IStateBufferLayout* stateBufferLayout) = 0;
	virtual void SetPageSettings(uint32_t initialPageCount, bool allowDynamicResize = false) = 0;
	virtual SuccessToken ResizePageCount(uint32_t pageCount) = 0;

	// State value methods
	virtual StateValueId GetStateValueId(const Marshal::In<std::string>& name) const = 0;
	template<class T>
	inline void SetStateValue(StateValueId stateId, const T& value);
	template<class T, class... IndexTs>
	inline void SetStateValue(StateValueId stateId, const T& value, size_t firstArrayIndex, IndexTs... additionalArrayIndices);
	template<class T>
	inline void SetStateValue(StateValueId stateId, const T& value, const size_t* arrayIndices, size_t arrayIndexCount);
	template<class T>
	inline void SetStateValueForPage(uint32_t pageNo, StateValueId stateId, const T& value);
	template<class T, class... IndexTs>
	inline void SetStateValueForPage(uint32_t pageNo, StateValueId stateId, const T& value, size_t firstArrayIndex, IndexTs... additionalArrayIndices);
	template<class T>
	inline void SetStateValueForPage(uint32_t pageNo, StateValueId stateId, const T& value, const size_t* arrayIndices, size_t arrayIndexCount);
	virtual void SetRawPageData(uint32_t pageNo, const uint8_t* data, size_t dataSizeInBytes, size_t dataOffsetInBytes = 0) = 0;

protected:
	// Constructors
	~IStateBuffer() = default;

	// State value methods
	virtual void SetStateValueInternal(uint32_t pageNo, StateValueId stateId, bool value, const size_t* arrayIndices, size_t arrayIndexCount) = 0;
	virtual void SetStateValueInternal(uint32_t pageNo, StateValueId stateId, const V1Int8& value, const size_t* arrayIndices, size_t arrayIndexCount) = 0;
	virtual void SetStateValueInternal(uint32_t pageNo, StateValueId stateId, const V1Int16& value, const size_t* arrayIndices, size_t arrayIndexCount) = 0;
	virtual void SetStateValueInternal(uint32_t pageNo, StateValueId stateId, const V1Int32& value, const size_t* arrayIndices, size_t arrayIndexCount) = 0;
	virtual void SetStateValueInternal(uint32_t pageNo, StateValueId stateId, const V1UInt8& value, const size_t* arrayIndices, size_t arrayIndexCount) = 0;
	virtual void SetStateValueInternal(uint32_t pageNo, StateValueId stateId, const V1UInt16& value, const size_t* arrayIndices, size_t arrayIndexCount) = 0;
	virtual void SetStateValueInternal(uint32_t pageNo, StateValueId stateId, const V1UInt32& value, const size_t* arrayIndices, size_t arrayIndexCount) = 0;
	virtual void SetStateValueInternal(uint32_t pageNo, StateValueId stateId, const V1Float32& value, const size_t* arrayIndices, size_t arrayIndexCount) = 0;
	virtual void SetStateValueInternal(uint32_t pageNo, StateValueId stateId, const V1Float64& value, const size_t* arrayIndices, size_t arrayIndexCount) = 0;
	virtual void SetStateValueInternal(uint32_t pageNo, StateValueId stateId, const V2Int8& value, const size_t* arrayIndices, size_t arrayIndexCount) = 0;
	virtual void SetStateValueInternal(uint32_t pageNo, StateValueId stateId, const V2Int16& value, const size_t* arrayIndices, size_t arrayIndexCount) = 0;
	virtual void SetStateValueInternal(uint32_t pageNo, StateValueId stateId, const V2Int32& value, const size_t* arrayIndices, size_t arrayIndexCount) = 0;
	virtual void SetStateValueInternal(uint32_t pageNo, StateValueId stateId, const V2UInt8& value, const size_t* arrayIndices, size_t arrayIndexCount) = 0;
	virtual void SetStateValueInternal(uint32_t pageNo, StateValueId stateId, const V2UInt16& value, const size_t* arrayIndices, size_t arrayIndexCount) = 0;
	virtual void SetStateValueInternal(uint32_t pageNo, StateValueId stateId, const V2UInt32& value, const size_t* arrayIndices, size_t arrayIndexCount) = 0;
	virtual void SetStateValueInternal(uint32_t pageNo, StateValueId stateId, const V2Float32& value, const size_t* arrayIndices, size_t arrayIndexCount) = 0;
	virtual void SetStateValueInternal(uint32_t pageNo, StateValueId stateId, const V2Float64& value, const size_t* arrayIndices, size_t arrayIndexCount) = 0;
	virtual void SetStateValueInternal(uint32_t pageNo, StateValueId stateId, const V3Int8& value, const size_t* arrayIndices, size_t arrayIndexCount) = 0;
	virtual void SetStateValueInternal(uint32_t pageNo, StateValueId stateId, const V3Int16& value, const size_t* arrayIndices, size_t arrayIndexCount) = 0;
	virtual void SetStateValueInternal(uint32_t pageNo, StateValueId stateId, const V3Int32& value, const size_t* arrayIndices, size_t arrayIndexCount) = 0;
	virtual void SetStateValueInternal(uint32_t pageNo, StateValueId stateId, const V3UInt8& value, const size_t* arrayIndices, size_t arrayIndexCount) = 0;
	virtual void SetStateValueInternal(uint32_t pageNo, StateValueId stateId, const V3UInt16& value, const size_t* arrayIndices, size_t arrayIndexCount) = 0;
	virtual void SetStateValueInternal(uint32_t pageNo, StateValueId stateId, const V3UInt32& value, const size_t* arrayIndices, size_t arrayIndexCount) = 0;
	virtual void SetStateValueInternal(uint32_t pageNo, StateValueId stateId, const V3Float32& value, const size_t* arrayIndices, size_t arrayIndexCount) = 0;
	virtual void SetStateValueInternal(uint32_t pageNo, StateValueId stateId, const V3Float64& value, const size_t* arrayIndices, size_t arrayIndexCount) = 0;
	virtual void SetStateValueInternal(uint32_t pageNo, StateValueId stateId, const V4Int8& value, const size_t* arrayIndices, size_t arrayIndexCount) = 0;
	virtual void SetStateValueInternal(uint32_t pageNo, StateValueId stateId, const V4Int16& value, const size_t* arrayIndices, size_t arrayIndexCount) = 0;
	virtual void SetStateValueInternal(uint32_t pageNo, StateValueId stateId, const V4Int32& value, const size_t* arrayIndices, size_t arrayIndexCount) = 0;
	virtual void SetStateValueInternal(uint32_t pageNo, StateValueId stateId, const V4UInt8& value, const size_t* arrayIndices, size_t arrayIndexCount) = 0;
	virtual void SetStateValueInternal(uint32_t pageNo, StateValueId stateId, const V4UInt16& value, const size_t* arrayIndices, size_t arrayIndexCount) = 0;
	virtual void SetStateValueInternal(uint32_t pageNo, StateValueId stateId, const V4UInt32& value, const size_t* arrayIndices, size_t arrayIndexCount) = 0;
	virtual void SetStateValueInternal(uint32_t pageNo, StateValueId stateId, const V4Float32& value, const size_t* arrayIndices, size_t arrayIndexCount) = 0;
	virtual void SetStateValueInternal(uint32_t pageNo, StateValueId stateId, const V4Float64& value, const size_t* arrayIndices, size_t arrayIndexCount) = 0;
	virtual void SetStateValueInternal(uint32_t pageNo, StateValueId stateId, const M2Float32& value, const size_t* arrayIndices, size_t arrayIndexCount) = 0;
	virtual void SetStateValueInternal(uint32_t pageNo, StateValueId stateId, const M3Float32& value, const size_t* arrayIndices, size_t arrayIndexCount) = 0;
	virtual void SetStateValueInternal(uint32_t pageNo, StateValueId stateId, const M4Float32& value, const size_t* arrayIndices, size_t arrayIndexCount) = 0;
};

}} // namespace cobalt::graphics
#include "IStateBuffer.inl"
