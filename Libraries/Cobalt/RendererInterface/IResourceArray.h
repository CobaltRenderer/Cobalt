// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include <cstdint>
namespace cobalt { namespace graphics {

class IResourceArray
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
	enum class DataPersistenceFlags
	{
		PersistAlways = 0x00000000,
		InvalidateExistingDataOnWrite = 0x000000001,
		InvalidateExistingDataAfterDrawComplete = 0x000000002,
	};

public:
	// Usage methods
	virtual void SetPerformanceHints(PerformanceHint performanceHintCpu, PerformanceHint performanceHintGpu) = 0;
	virtual void SetDataPersistenceFlags(DataPersistenceFlags dataPersistenceFlags) = 0;

protected:
	// Constructors
	~IResourceArray() = default;
};

}} // namespace cobalt::graphics
#include "IResourceArray.inl"
