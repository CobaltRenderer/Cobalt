// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include "Direct3DHeaders.h"
#include <cstdint>
namespace cobalt::graphics {
class Direct3DHeap;

class DescriptorHandle
{
public:
	// Constructors
	DescriptorHandle(Direct3DHeap* heap, const CD3DX12_CPU_DESCRIPTOR_HANDLE& nativeCPUHandle, const CD3DX12_GPU_DESCRIPTOR_HANDLE& nativeGPUHandle, uint32_t heapID, uint32_t allocationID);
	DescriptorHandle(Direct3DHeap* heap, const CD3DX12_CPU_DESCRIPTOR_HANDLE& nativeCPUHandle, uint32_t heapID, uint32_t allocationID);
	DescriptorHandle(DescriptorHandle&& source) noexcept;
	DescriptorHandle(const DescriptorHandle& source) = delete;
	DescriptorHandle& operator=(const DescriptorHandle& source) = delete;
	~DescriptorHandle();

	// Assignment operators
	DescriptorHandle& operator=(DescriptorHandle&& source) noexcept;

	// Handle methods
	const CD3DX12_CPU_DESCRIPTOR_HANDLE& GetNativeCPUHandle() const;
	const CD3DX12_GPU_DESCRIPTOR_HANDLE& GetNativeGPUHandle() const;

private:
	// Handle methods
	void FreeHandle();

private:
	Direct3DHeap* _heap = {};
	bool _descriptorAllocated = false;
	bool _gpuHandlePresent = false;
	uint32_t _heapID = 0;
	uint32_t _allocationID = 0;
	CD3DX12_CPU_DESCRIPTOR_HANDLE _nativeCPUHandle = {};
	CD3DX12_GPU_DESCRIPTOR_HANDLE _nativeGPUHandle = {};
};

} // namespace cobalt::graphics
