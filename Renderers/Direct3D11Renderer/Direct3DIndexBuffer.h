// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include "Direct3DHeaders.h"
#include <Cobalt/LoggingInterface/LoggingInterface.pkg>
#include <Cobalt/RendererInterface/RendererInterface.pkg>
#include <atomic>
#include <mutex>
namespace cobalt::graphics {
class Direct3DRenderer;
class Direct3DTransferBatch;
class Direct3DTexelArray;

class Direct3DIndexBuffer : public IIndexBuffer
{
public:
	// Structures
	struct IndexAttributeInfo
	{
		IIndexAttribute::DataType dataType;
		size_t dataTypeByteSize;
		size_t indexCount;
		IIndexAttribute::PerformanceHint performanceHintCpu;
		IIndexAttribute::PerformanceHint performanceHintGpu;
		IIndexAttribute::DataPersistenceFlags dataPersistenceFlags;
		size_t bufferOffsetInBytes;
		size_t bufferStartPosInBytes;
	};

public:
	// Constructors
	Direct3DIndexBuffer(cobalt::logging::ILogger* log, Direct3DRenderer* renderer);
	~Direct3DIndexBuffer();

	// Initialization methods
	void Delete() override;
	SuccessToken AllocateMemory() override;
	SuccessToken AllocateMemoryWithAlias(ITexelArray* texelArray) override;
	bool IsAllocated() const;

	// Binding methods
	SuccessToken BindIndexAttribute(IIndexAttribute& indexAttribute) override;
	SuccessToken BindIndexAttributeManualLayout(IIndexAttribute& indexAttribute, size_t bufferOffsetInBytes, size_t bufferStrideInBytes) override;
	const IndexAttributeInfo* GetIndexAttributeInfo(size_t attributeIndex) const;

	// Data methods
	SuccessToken SetRawInitialData(const uint8_t* data, size_t dataSizeInBytes) override;
	SuccessToken QueueRawDataUpdate(const uint8_t* data, size_t dataSizeInBytes, size_t bufferOffsetInBytes, ITransferBatch* transferBatch) override;

	// Build state methods
	void MigrateBuildStateToDrawState();
	void CompletePendingDataWrites(ID3D11Device1* device, ID3D11DeviceContext1* context);
	ID3D11Buffer* GetNativeBuffer() const;

protected:
	// Data methods
	SuccessToken SetInitialData(const uint8_t* data, size_t entryCount, size_t entryStrideInBytes) override;
	SuccessToken QueueDataUpdate(const uint8_t* data, size_t entryCount, size_t initialIndexNo, size_t entryStrideInBytes, ITransferBatch* transferBatch) override;

private:
	// Structures
	struct PendingWrite
	{
		PendingWrite(const IndexAttributeInfo& attributeInfo, size_t initialBufferPosInBytes, size_t dataSizeInBytes, Direct3DTransferBatch* transferBatch)
		: attributeInfo(attributeInfo), entryCount(dataSizeInBytes), initialIndexNo(initialBufferPosInBytes), entryStrideInBytes(1), rawDataWrite(true), transferBatch(transferBatch)
		{}
		PendingWrite(const IndexAttributeInfo& attributeInfo, size_t entryCount, size_t initialIndexNo, size_t entryStrideInBytes, Direct3DTransferBatch* transferBatch)
		: attributeInfo(attributeInfo), entryCount(entryCount), initialIndexNo(initialIndexNo), entryStrideInBytes(entryStrideInBytes), rawDataWrite(false), transferBatch(transferBatch)
		{}

		bool rawDataWrite;
		const IndexAttributeInfo& attributeInfo;
		size_t entryCount;
		size_t initialIndexNo;
		size_t entryStrideInBytes;
		std::vector<uint8_t> data;
		Direct3DTransferBatch* transferBatch;
	};
	struct MutableState
	{
		std::vector<PendingWrite> pendingWrites;
	};

private:
	// Initialization methods
	bool AllocateMemoryInternal(Direct3DTexelArray* texelArray);
	void ReleaseMemory();

	// Build state methods
	bool CreateNativeBuffer(const uint8_t* initialData, bool aliasAsTexelArray);
	bool CompletePendingDataWrite(const PendingWrite& pendingWrite, ID3D11Device1* device, ID3D11DeviceContext1* context, void* mappedBuffer);
	void FlagBuildStateModified();

private:
	cobalt::logging::ILogger* _log;
	Direct3DRenderer* _renderer = nullptr;
	IndexAttributeInfo _indexAttributeInfo = {};
	bool _indexAttributeAdded = false;
	bool _indexBufferCreated = false;
	bool _nativeBufferCreationPending = false;
	Microsoft::WRL::ComPtr<ID3D11Buffer> _indexBuffer;
	size_t _totalBufferSizeInBytes = 0;
	bool _usingDynamicBuffer = false;
	bool _allowDiscardOnPartialWrite = false;
	bool _initialDataSet = false;
	bool _manualBufferLayout = false;
	const uint8_t* _initialData = nullptr;
	size_t _initialDataEntryCount = 0;
	size_t _initialDataEntryStrideInBytes = 0;
	size_t _initialDataEntrySizeInBytes = 0;
	std::vector<uint8_t> _initialDataBuffer;
	UINT _cpuFlags = {};
	D3D11_USAGE _usageType = D3D11_USAGE_DEFAULT;
	std::mutex _buildStateMutex;
	std::atomic_flag _stateModified = ATOMIC_FLAG_INIT;
	uint32_t _buildIndex;
	uint32_t _drawIndex;
	MutableState _state[2];
};

} // namespace cobalt::graphics
