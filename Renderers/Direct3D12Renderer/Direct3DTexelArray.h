// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include "BufferWrapper.h"
#include "Direct3DHeaders.h"
// This needs to be after Direct3DHeaders.h so we have our defines for Windows.h set properly
#include "D3D12MemAlloc.h"
#include "DescriptorHandle.h"
#include "Direct3DCommandListPool.h"
#include <Cobalt/LoggingInterface/LoggingInterface.pkg>
#include <Cobalt/RendererInterface/RendererInterface.pkg>
#include <atomic>
#include <mutex>
#include <vector>
namespace cobalt::graphics {
class Direct3DRenderer;
class Direct3DTransferBatch;
class Direct3DTexelArrayOutput;

class Direct3DTexelArray : public ITexelArray
{
public:
	// Constructors
	Direct3DTexelArray(cobalt::logging::ILogger* log, Direct3DRenderer* renderer);
	~Direct3DTexelArray();

	// Initialization methods
	void Delete() override;
	SuccessToken AllocateMemory() override;
	bool AllocateAsAliasForBuffer(BufferWrapper* bufferWrapper, size_t bufferSizeInBytes);
	void SetBufferLayout(ImageFormat imageFormat, DataFormat dataFormat, size_t entryCount) override;

	// Usage methods
	void SetUsageFlags(UsageFlags usageFlags) override;
	void SetPerformanceHints(PerformanceHint performanceHintCpu, PerformanceHint performanceHintGpu) override;
	void SetDataPersistenceFlags(DataPersistenceFlags dataPersistenceFlags) override;

	// Initial data methods
	SuccessToken SetInitialData(const void* sourceBuffer, size_t sourceBufferSizeInBytes, SourceImageFormat imageFormat, SourceDataFormat dataFormat) override;

	// Data update methods
	SuccessToken QueueDataUpdate(const void* sourceBuffer, size_t sourceBufferSizeInBytes, SourceImageFormat imageFormat, SourceDataFormat dataFormat, size_t targetBufferOffset, ITransferBatch* transferBatch) override;

	// Data transfer methods
	SuccessToken QueueDataTransfer(ITexelArray* targetBuffer, size_t transferCount, size_t sourceBufferOffset, size_t targetBufferOffset, ITransferBatch* transferBatch) override;

	// Output capture methods
	bool HasCaptureTargets() const;
	void AddOutputCaptureTarget(ITexelArrayOutput* captureTarget) override;
	void RemoveOutputCaptureTarget(ITexelArrayOutput* captureTarget) override;
	void CaptureDataBufferOutput(ID3D12GraphicsCommandList* commandList);
	void CompleteCaptureDataBufferOutput();

	// Build state methods
	void MigrateBuildStateToDrawState();
	size_t CompletePendingDataWrites(ID3D12GraphicsCommandList* commandList, D3DX12Residency::ResidencySet* residencySet);
	size_t CompletePendingDataTransfers(ID3D12GraphicsCommandList* commandList, D3DX12Residency::ResidencySet* residencySet);
	const CD3DX12_GPU_DESCRIPTOR_HANDLE& GetReadOnlyGPUDescriptorHandle(ID3D12GraphicsCommandList* commandList, D3D_SHADER_INPUT_TYPE type);
	const CD3DX12_GPU_DESCRIPTOR_HANDLE& GetReadWriteGPUDescriptorHandle(ID3D12GraphicsCommandList* commandList, D3D_SHADER_INPUT_TYPE type);
	void AddAsCurrentBuffer();

private:
	// Structures
	struct PendingWrite
	{
		explicit PendingWrite(Direct3DTransferBatch* transferBatch)
		: transferBatch(transferBatch)
		{}

		ID3D12Resource* uploadBuffer = nullptr;
		size_t targetBufferPosInBytes = 0;
		size_t uploadBufferSizeInBytes = 0;
		std::vector<uint8_t> data;
		Direct3DTransferBatch* transferBatch;
	};
	struct PendingTransfer
	{
		explicit PendingTransfer(Direct3DTexelArray* targetBuffer, Direct3DTransferBatch* transferBatch)
		: targetBuffer(targetBuffer), transferBatch(transferBatch)
		{}

		Direct3DTexelArray* targetBuffer;
		Direct3DTransferBatch* transferBatch;
		size_t transferCountInBytes = 0;
		size_t sourceBufferPosInBytes = 0;
		size_t targetBufferPosInBytes = 0;
	};
	struct MutableState
	{
		std::vector<PendingWrite> pendingWrites;
		std::vector<PendingTransfer> pendingTransfers;
		std::vector<Direct3DTexelArrayOutput*> captureTargets;
	};

private:
	// Initialization methods
	void ReleaseMemory();

	// Data conversion methods
	bool ConvertDataFormat(const void* sourceBuffer, size_t sourceBufferSizeInBytes, SourceImageFormat sourceImageFormat, SourceDataFormat sourceDataFormat, ImageFormat targetImageFormat, DataFormat targetDataFormat, std::vector<uint8_t>& targetBuffer) const;

	// Format methods
	constexpr static bool GetFormatNative(ImageFormat requestedImageFormat, DataFormat requestedDataFormat, DXGI_FORMAT& nativeFormat);

	// Build state methods
	bool CreateNativeBuffer();
	size_t CompletePendingDataWritesInternal(std::vector<PendingWrite>& pendingWrites, ID3D12GraphicsCommandList* commandList, D3DX12Residency::ResidencySet* residencySet, bool performDrawStateTransition);
	void WriteDataToMappedBuffer(const PendingWrite& pendingWrite, const uint8_t* data, uint8_t* mappedMemory);
	void CompletePendingDataWrite(ID3D12GraphicsCommandList* commandList, D3DX12Residency::ResidencySet* residencySet, const PendingWrite& pendingWrite);
	void CompletePendingDataTransfer(ID3D12GraphicsCommandList* commandList, D3DX12Residency::ResidencySet* residencySet, const PendingTransfer& pendingTransfer);
	void FlagBuildStateModified();

private:
	cobalt::logging::ILogger* _log;
	mutable std::mutex _accessMutex;
	Direct3DRenderer* _renderer;
	bool _bufferLayoutSet = false;
	size_t _structureEntryCount = 0;
	size_t _structureStrideInBytes = 0;
	size_t _totalBufferSizeInBytes = 0;
	ImageFormat _imageFormat = {};
	DataFormat _dataFormat = {};
	DXGI_FORMAT _nativeFormat = {};
	bool _bufferCreated = false;
	Microsoft::WRL::ComPtr<ID3D12Resource> _buffer;
	Microsoft::WRL::ComPtr<ID3D12Resource> _captureDataStagingBuffer;
	D3D12MA::Allocation* _captureDataStagingBufferAllocation = nullptr;
	std::unique_ptr<DescriptorHandle> _readOnlyViewHandle;
	std::unique_ptr<DescriptorHandle> _readWriteViewHandle;
	BufferWrapper _bufferWrapperLocal;
	BufferWrapper* _bufferWrapper = nullptr;
	bool _createdReadOnlyView = false;
	bool _createdReadWriteView = false;
	bool _initialDataSet = false;
	bool _addedAsCurrent = false;
	const uint8_t* _initialData = nullptr;
	size_t _initialDataSizeInBytes = 0;
	SourceImageFormat _initialDataImageFormat = {};
	SourceDataFormat _initialDataDataFormat = {};
	UsageFlags _usageFlags = UsageFlags::Default;
	PerformanceHint _performanceHintCpu = PerformanceHint::Default;
	PerformanceHint _performanceHintGpu = PerformanceHint::Default;
	DataPersistenceFlags _dataPersistenceFlags = DataPersistenceFlags::PersistAlways;
	std::vector<uint8_t> _initialDataBuffer;
	std::mutex _buildStateMutex;
	std::atomic_flag _stateModified = ATOMIC_FLAG_INIT;
	uint32_t _buildIndex;
	uint32_t _drawIndex;
	MutableState _state[2];
};

} // namespace cobalt::graphics
