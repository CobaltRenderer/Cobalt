// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "Direct3DHeap.h"
#include "DescriptorHandle.h"
#include "Direct3DHeapManager.h"
#include "Direct3DRenderer.h"
#include <algorithm>
namespace cobalt::graphics {

//----------------------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------------------
Direct3DHeap::Direct3DHeap(cobalt::logging::ILogger* log, ID3D12Device* device, Direct3DHeapManager* heapManager, D3D12_DESCRIPTOR_HEAP_TYPE heapType, uint32_t pageSize)
: _log(log), _device(device), _heapManager(heapManager), _heapType(heapType), _pageSize(pageSize), _firstHeapEntry(0)
{
	_heapIsShaderVisible = (_heapType == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) || (_heapType == D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
	_descriptorSize = _device->GetDescriptorHandleIncrementSize(_heapType);
}

//----------------------------------------------------------------------------------------
// Allocation methods
//----------------------------------------------------------------------------------------
void Direct3DHeap::EnsureHeapExists()
{
	if (_heapEntries.empty())
	{
		AllocateDescriptor();
	}
}

//----------------------------------------------------------------------------------------
std::unique_ptr<DescriptorHandle> Direct3DHeap::AllocateDescriptor()
{
	// Attempt to allocate a descriptor in an existing descriptor heap
	uint32_t heapEntryNo = _firstHeapEntry;
	while (heapEntryNo < (uint32_t)_heapEntries.size())
	{
		// Attempt to allocate a slot in the target heap
		HeapEntry& heapEntry = _heapEntries[heapEntryNo];
		uint32_t slotNo;
		if (!heapEntry.allocationManager.TryAllocateSlot(slotNo))
		{
			_firstHeapEntry = ++heapEntryNo;
			continue;
		}

		// Return a new descriptor handle to the caller
		CD3DX12_CPU_DESCRIPTOR_HANDLE cpuDescriptorHandle = heapEntry.baseCpuHandle;
		cpuDescriptorHandle.Offset(slotNo, _descriptorSize);
		if (_heapIsShaderVisible)
		{
			CD3DX12_GPU_DESCRIPTOR_HANDLE gpuDescriptorHandle = heapEntry.baseGpuHandle;
			gpuDescriptorHandle.Offset(slotNo, _descriptorSize);
			return std::make_unique<DescriptorHandle>(this, cpuDescriptorHandle, gpuDescriptorHandle, heapEntryNo, slotNo);
		}
		return std::make_unique<DescriptorHandle>(this, cpuDescriptorHandle, heapEntryNo, slotNo);
	}

	// Determine the flags to use when creating the new heap
	D3D12_DESCRIPTOR_HEAP_FLAGS heapFlags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	if (_heapIsShaderVisible)
	{
		heapFlags |= D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	}

	// Create a new descriptor heap entry
	HeapEntry heapEntry(_pageSize);
	D3D12_DESCRIPTOR_HEAP_DESC heapDescriptor = {};
	heapDescriptor.NumDescriptors = _pageSize;
	heapDescriptor.Type = _heapType;
	heapDescriptor.Flags = heapFlags;
	HRESULT createDescriptorHeapReturn = _device->CreateDescriptorHeap(&heapDescriptor, IID_PPV_ARGS(&heapEntry.nativeHeap));
	if (FAILED(createDescriptorHeapReturn))
	{
		_log->Error("CreateDescriptorHeap failed with error code {0}", createDescriptorHeapReturn);
		return nullptr;
	}
	heapEntry.baseCpuHandle = heapEntry.nativeHeap->GetCPUDescriptorHandleForHeapStart();
	if (_heapIsShaderVisible)
	{
		heapEntry.baseGpuHandle = heapEntry.nativeHeap->GetGPUDescriptorHandleForHeapStart();
	}

	// Notify the heap manager that we've added a new descriptor heap
	_heapManager->NotifyDescriptorHeapAdded(heapEntry.nativeHeap.Get(), _heapIsShaderVisible);

	// Attempt to allocate a slot in the new heap
	uint32_t slotNo;
	if (!heapEntry.allocationManager.TryAllocateSlot(slotNo))
	{
		_log->Error("TryAllocateSlot failed in newly allocated heap");
		return nullptr;
	}

	// Build the native descriptor handle object
	CD3DX12_CPU_DESCRIPTOR_HANDLE cpuDescriptorHandle = heapEntry.baseCpuHandle;
	cpuDescriptorHandle.Offset(slotNo, _descriptorSize);
	CD3DX12_GPU_DESCRIPTOR_HANDLE gpuDescriptorHandle;
	if (_heapIsShaderVisible)
	{
		gpuDescriptorHandle = heapEntry.baseGpuHandle;
		gpuDescriptorHandle.Offset(slotNo, _descriptorSize);
	}
	heapEntryNo = (uint32_t)_heapEntries.size();

	// Add the new descriptor heap to the list of descriptor heaps
	_heapEntries.push_back(std::move(heapEntry));

	// Return a new descriptor handle to the caller
	if (_heapIsShaderVisible)
	{
		return std::make_unique<DescriptorHandle>(this, cpuDescriptorHandle, gpuDescriptorHandle, heapEntryNo, slotNo);
	}
	return std::make_unique<DescriptorHandle>(this, cpuDescriptorHandle, heapEntryNo, slotNo);
}

//----------------------------------------------------------------------------------------
void Direct3DHeap::FreeDescriptor(uint32_t heapID, uint32_t allocationID)
{
	// Free the target slot
	_heapEntries[heapID].allocationManager.FreeSlot(allocationID);

	// Flag that there's at least one available heap entry in the target heap
	_firstHeapEntry = std::min(_firstHeapEntry, heapID);
}

} // namespace cobalt::graphics
