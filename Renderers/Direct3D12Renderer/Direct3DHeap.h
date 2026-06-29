// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include "DescriptorHandle.h"
#include "Direct3DHeaders.h"
#include "Direct3DHeapManager.h"
#include "NativeHeapAllocationManager.h"
#include <Cobalt/LoggingInterface/LoggingInterface.pkg>
#include <memory>
#include <vector>
namespace cobalt::graphics {
class Direct3DRenderer;
class Direct3DHeapManager;

class Direct3DHeap
{
public:
	// Friend declarations
	friend class DescriptorHandle;

public:
	// Constructors
	Direct3DHeap(cobalt::logging::ILogger* log, ID3D12Device* device, Direct3DHeapManager* heapManager, D3D12_DESCRIPTOR_HEAP_TYPE heapType, uint32_t pageSize);

	// Allocation methods
	void EnsureHeapExists();
	std::unique_ptr<DescriptorHandle> AllocateDescriptor();

private:
	// Structures
	struct HeapEntry
	{
		explicit HeapEntry(uint32_t heapSize)
		: allocationManager(heapSize)
		{}

		NativeHeapAllocationManager allocationManager;
		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> nativeHeap;
		CD3DX12_CPU_DESCRIPTOR_HANDLE baseCpuHandle = {};
		CD3DX12_GPU_DESCRIPTOR_HANDLE baseGpuHandle = {};
	};

private:
	// Allocation methods
	void FreeDescriptor(uint32_t heapID, uint32_t allocationID);

private:
	cobalt::logging::ILogger* _log;
	ID3D12Device* _device;
	Direct3DHeapManager* _heapManager;
	D3D12_DESCRIPTOR_HEAP_TYPE _heapType;
	uint32_t _pageSize;
	uint32_t _descriptorSize;
	std::vector<HeapEntry> _heapEntries;
	uint32_t _firstHeapEntry;
	bool _heapIsShaderVisible = false;
};

} // namespace cobalt::graphics
