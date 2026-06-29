// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include "DescriptorHandle.h"
#include "Direct3DHeaders.h"
#include "Direct3DHeap.h"
#include <Cobalt/LoggingInterface/LoggingInterface.pkg>
#include <memory>
#include <vector>
namespace cobalt::graphics {

class Direct3DHeapManager
{
public:
	// Enumerations
	enum class ResourceType
	{
		ConstantBufferView,
		ShaderResourceView,
		UnorderedAccessView,
		RenderTargetView,
		DepthStencilView,
		Sampler,
	};

public:
	// Constructors
	Direct3DHeapManager(cobalt::logging::ILogger* log, ID3D12Device* device, uint32_t pageSize = 0);

	// Allocation methods
	void PreAllocateHeapForType(ResourceType type);
	std::unique_ptr<DescriptorHandle> AllocateDescriptor(ResourceType type);

	// Heap object methods
	const std::vector<ID3D12DescriptorHeap*>& GetShaderVisibleDescriptorHeaps() const;
	void NotifyDescriptorHeapAdded(ID3D12DescriptorHeap* descriptorHeap, bool heapIsShaderVisible);

private:
	// Constants
	static const uint32_t HeapCount = 4;
	static const uint32_t DefaultPageSize = 100;

private:
	// Helper methods
	constexpr static uint32_t ResourceTypeToHeapIndex(ResourceType type);

private:
	cobalt::logging::ILogger* _log;
	ID3D12Device* _device;
	std::vector<Direct3DHeap> _heaps;
	std::vector<ID3D12DescriptorHeap*> _shaderVisibleDescriptorHeaps;
	uint32_t _pageSize;
};

} // namespace cobalt::graphics
