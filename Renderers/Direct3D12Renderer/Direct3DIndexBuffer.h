// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include "BufferWrapper.h"
#include "Direct3DCommandListPool.h"
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
	size_t CompletePendingDataWrites(ID3D12GraphicsCommandList* commandList, D3DX12Residency::ResidencySet* residencySet);
	ID3D12Resource* GetNativeBuffer() const;
	D3DX12Residency::ManagedObject* GetResidencyObject() const;
	bool HasBufferAlias() const;
	BufferWrapper* GetBufferWrapper();

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
		ID3D12Resource* uploadBuffer = nullptr;
		size_t targetBufferPos = 0;
		size_t uploadBufferSizeInBytes = 0;
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

	// Data methods
	bool QueueDataUpdateInternal(PendingWrite& pendingWrite, const uint8_t* data, size_t uploadBufferSizeInBytes, size_t bufferStartPosInBytes, Direct3DTransferBatch* transferBatch);

	// Build state methods
	bool CreateNativeBuffer(bool aliasAsTexelArray);
	size_t CompletePendingDataWritesInternal(std::vector<PendingWrite>& pendingWrites, ID3D12GraphicsCommandList* commandList, D3DX12Residency::ResidencySet* residencySet, bool performDrawStateTransition);
	void WriteDataToMappedBuffer(size_t entryCount, size_t entrySizeInBytes, size_t entryStrideInBytes, const uint8_t* data, uint8_t* mappedMemory);
	void CompletePendingDataWrite(ID3D12GraphicsCommandList* commandList, D3DX12Residency::ResidencySet* residencySet, const PendingWrite& pendingWrite);
	void FlagBuildStateModified();

private:
	cobalt::logging::ILogger* _log;
	Direct3DRenderer* _renderer = nullptr;
	IndexAttributeInfo _indexAttributeInfo = {};
	bool _indexAttributeAdded = false;
	bool _indexBufferCreated = false;
	Microsoft::WRL::ComPtr<ID3D12Resource> _indexBuffer;
	mutable D3DX12Residency::ManagedObject _residencyObject;
	BufferWrapper _bufferWrapper;
	bool _hasBufferAlias = false;
	bool _managedObjectTracked = false;
	size_t _totalBufferSizeInBytes = 0;
	bool _initialDataSet = false;
	bool _manualBufferLayout = false;
	const uint8_t* _initialData = nullptr;
	size_t _initialDataEntryCount = 0;
	size_t _initialDataEntryStrideInBytes = 0;
	size_t _initialDataEntrySizeInBytes = 0;
	std::mutex _buildStateMutex;
	std::atomic_flag _stateModified = ATOMIC_FLAG_INIT;
	uint32_t _buildIndex;
	uint32_t _drawIndex;
	MutableState _state[2];
};

} // namespace cobalt::graphics
