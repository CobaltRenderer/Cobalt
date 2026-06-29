// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include "Direct3DHeaders.h"
#include <Cobalt/LoggingInterface/LoggingInterface.pkg>
#include <Cobalt/RendererInterface/RendererInterface.pkg>
#include <atomic>
#include <mutex>
#include <vector>
namespace cobalt::graphics {
class Direct3DRenderer;
class Direct3DTransferBatch;
class Direct3DTexelArray;

class Direct3DVertexBuffer : public IVertexBuffer
{
public:
	// Structures
	struct VertexAttributeInfo
	{
		IVertexAttribute::DataType dataType;
		size_t dataTypeByteSize;
		size_t elementCount;
		size_t vertexCount;
		IVertexAttribute::PerformanceHint performanceHintCpu;
		IVertexAttribute::PerformanceHint performanceHintGpu;
		IVertexAttribute::DataPersistenceFlags dataPersistenceFlags;
		size_t bufferBaseStartAddress;
		size_t bufferOffsetInBytes;
		size_t vertexPaddingAtStart;
		size_t vertexPaddingAtEnd;
		size_t bufferStartPosInBytes;
		size_t bufferStrideInBytes;
		bool initialDataSet;
		const uint8_t* initialData;
		size_t initialDataEntryStrideInBytes;
	};

public:
	// Constructors
	Direct3DVertexBuffer(cobalt::logging::ILogger* log, Direct3DRenderer* renderer);
	~Direct3DVertexBuffer();

	// Initialization methods
	void Delete() override;
	SuccessToken AllocateMemory() override;
	SuccessToken AllocateMemoryWithAlias(ITexelArray* texelArray) override;
	bool IsAllocated() const;

	// Binding methods
	SuccessToken BindVertexAttribute(IVertexAttribute& vertexAttribute) override;
	SuccessToken BindVertexAttributeManualLayout(IVertexAttribute& vertexAttribute, size_t bufferOffsetInBytes, size_t bufferStrideInBytes) override;
	const VertexAttributeInfo* GetVertexAttributeInfo(size_t attributeIndex) const;

	// Data methods
	SuccessToken SetRawInitialData(const uint8_t* data, size_t dataSizeInBytes) override;
	SuccessToken QueueRawDataUpdate(const uint8_t* data, size_t dataSizeInBytes, size_t bufferOffsetInBytes, ITransferBatch* transferBatch) override;

	// Build state methods
	void MigrateBuildStateToDrawState();
	void CompletePendingDataWrites(ID3D11Device1* device, ID3D11DeviceContext1* context);
	ID3D11Buffer* GetNativeBuffer() const;

protected:
	// Data methods
	SuccessToken SetInitialData(size_t attributeIndex, const uint8_t* data, size_t entryCount, size_t entryStrideInBytes) override;
	SuccessToken QueueDataUpdate(size_t attributeIndex, const uint8_t* data, size_t entryCount, size_t initialVertexNo, size_t entryStrideInBytes, ITransferBatch* transferBatch) override;

private:
	// Structures
	struct PendingWrite
	{
		PendingWrite(const VertexAttributeInfo& attributeInfo, size_t initialBufferPosInBytes, size_t dataSizeInBytes, Direct3DTransferBatch* transferBatch)
		: attributeInfo(attributeInfo), entryCount(dataSizeInBytes), initialVertexNo(initialBufferPosInBytes), entryStrideInBytes(1), rawDataWrite(true), transferBatch(transferBatch)
		{}
		PendingWrite(const VertexAttributeInfo& attributeInfo, size_t entryCount, size_t initialVertexNo, size_t entryStrideInBytes, Direct3DTransferBatch* transferBatch)
		: attributeInfo(attributeInfo), entryCount(entryCount), initialVertexNo(initialVertexNo), entryStrideInBytes(entryStrideInBytes), rawDataWrite(false), transferBatch(transferBatch)
		{}

		bool rawDataWrite;
		const VertexAttributeInfo& attributeInfo;
		size_t entryCount;
		size_t initialVertexNo;
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

	// Binding methods
	bool BindVertexAttributeInternal(IVertexAttribute& vertexAttribute, bool manualLayout, size_t bufferOffsetInBytes, size_t bufferStrideInBytes);

	// Build state methods
	bool CreateNativeBuffer(bool aliasAsTexelArray);
	bool CompletePendingDataWrite(const PendingWrite& pendingWrite, ID3D11Device1* device, ID3D11DeviceContext1* context, void* mappedBuffer);
	void FlagBuildStateModified();

private:
	cobalt::logging::ILogger* _log;
	Direct3DRenderer* _renderer;
	std::vector<VertexAttributeInfo> _vertexAttributeInfo;
	size_t _vertexAttributeCount;
	size_t _totalBufferSizeInBytes = 0;
	bool _vertexBufferCreated;
	bool _nativeBufferCreationPending;
	bool _bufferInterleaved = false;
	size_t _interleavedVertexSizeInBytes;
	UINT _cpuFlags = {};
	D3D11_USAGE _usageType = {};
	Microsoft::WRL::ComPtr<ID3D11Buffer> _vertexBuffer;
	bool _usingDynamicBuffer = false;
	bool _allowDiscardOnPartialWrite = false;
	bool _rawInitialDataSet = false;
	bool _manualBufferLayout = false;
	std::vector<uint8_t> _initialDataBuffer;
	std::mutex _buildStateMutex;
	std::atomic_flag _stateModified = ATOMIC_FLAG_INIT;
	uint32_t _buildIndex;
	uint32_t _drawIndex;
	MutableState _state[2];
};

} // namespace cobalt::graphics
