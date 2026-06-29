// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "DescriptorHandle.h"
#include "Direct3DHeap.h"
namespace cobalt::graphics {

//----------------------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------------------
DescriptorHandle::DescriptorHandle(Direct3DHeap* heap, const CD3DX12_CPU_DESCRIPTOR_HANDLE& nativeCPUHandle, const CD3DX12_GPU_DESCRIPTOR_HANDLE& nativeGPUHandle, uint32_t heapID, uint32_t allocationID)
: _heap(heap), _nativeCPUHandle(nativeCPUHandle), _nativeGPUHandle(nativeGPUHandle), _heapID(heapID), _allocationID(allocationID), _descriptorAllocated(true), _gpuHandlePresent(true)
{}

//----------------------------------------------------------------------------------------
DescriptorHandle::DescriptorHandle(Direct3DHeap* heap, const CD3DX12_CPU_DESCRIPTOR_HANDLE& nativeCPUHandle, uint32_t heapID, uint32_t allocationID)
: _heap(heap), _nativeCPUHandle(nativeCPUHandle), _heapID(heapID), _allocationID(allocationID), _descriptorAllocated(true), _gpuHandlePresent(false)
{
	_nativeGPUHandle = {0};
}

//----------------------------------------------------------------------------------------
DescriptorHandle::DescriptorHandle(DescriptorHandle&& source) noexcept
{
	*this = std::move(source);
}

//----------------------------------------------------------------------------------------
DescriptorHandle::~DescriptorHandle()
{
	FreeHandle();
}

//----------------------------------------------------------------------------------------
// Assignment operators
//----------------------------------------------------------------------------------------
DescriptorHandle& DescriptorHandle::operator=(DescriptorHandle&& source) noexcept
{
	// Clear any descriptor handle we are currently holding
	FreeHandle();

	// Take ownership of the descriptor handle from the source object
	_heap = source._heap;
	_nativeCPUHandle = source._nativeCPUHandle;
	_nativeGPUHandle = source._nativeGPUHandle;
	_heapID = source._heapID;
	_allocationID = source._allocationID;
	_descriptorAllocated = source._descriptorAllocated;
	source._descriptorAllocated = false;
	return *this;
}

//----------------------------------------------------------------------------------------
// Handle methods
//----------------------------------------------------------------------------------------
const CD3DX12_CPU_DESCRIPTOR_HANDLE& DescriptorHandle::GetNativeCPUHandle() const
{
	return _nativeCPUHandle;
}

//----------------------------------------------------------------------------------------
const CD3DX12_GPU_DESCRIPTOR_HANDLE& DescriptorHandle::GetNativeGPUHandle() const
{
	return _nativeGPUHandle;
}

//----------------------------------------------------------------------------------------
void DescriptorHandle::FreeHandle()
{
	if (_descriptorAllocated)
	{
		_heap->FreeDescriptor(_heapID, _allocationID);
		_descriptorAllocated = false;
	}
}

} // namespace cobalt::graphics
