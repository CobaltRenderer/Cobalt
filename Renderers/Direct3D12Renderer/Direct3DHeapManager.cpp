// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "Direct3DHeapManager.h"
#include "DescriptorHandle.h"
#include "Direct3DHeap.h"
#include "Direct3DRenderer.h"
namespace cobalt::graphics {

//----------------------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------------------
Direct3DHeapManager::Direct3DHeapManager(cobalt::logging::ILogger* log, ID3D12Device* device, uint32_t pageSize)
: _log(log), _device(device)
{
	// Determine the page size to use for each heap
	_pageSize = (pageSize == 0) ? DefaultPageSize : pageSize;

	// Create each heap
	//##FIX## So descriptor heaps are annoying. You can only bind one shader-visible heap of each type at a time, and
	//changing heap bindings can be very expensive, to the point where it's not recommended to do it mid-frame at all.
	//Some reading from the webgpu team discussing bindings, and the issue of DX12 descriptor heap management in
	//particular:
	//https://github.com/gpuweb/gpuweb/issues/19
	//To work around problems with large numbers of renderables, we had planned to use heap pages here, but the limit of
	//only being able to bind one heap of any given type makes that not work in practice. There are only two apparent
	//options here:
	//1. Pre-allocate massive heaps up-front and hope they're large enough for anything you ever encounter
	//2. Keep two heaps that grow up to hardware limits, and copy from CPU-side staging heaps into these shader
	//   accessible heaps during rendering. If the current heaps can't fit all the content, grow them.
	//We're going with the first option here for the time being. Reportedly, the minimum sizes for our heaps are a
	//million entries for SRV/UAV/CBV heaps, and 2048 entries for sampler heaps. We're going to run with initial heap
	//sizes to match, which really should be enough if the API is being used in a logical manner. If we want to support
	//infinite descriptor counts in the future though, doing this properly would involve something like the following:
	//1. Allocate all "fixed" descriptor handles (tied to resource views like textures and SSBO) in non-shader
	//   accessible heap, which can use paging happily.
	//2. Mark this fixed descriptor heap as "dirty" when anything new gets allocated from it
	//3. Maintain one shader-accessible descriptor heap of both types (SRV/UAV/CBV, and samplers), which we start with
	//   an initial max size.
	//4. At the start of each frame, if the fixed descriptor heap is "dirty", copy it into the corresponding shader
	//   accessible descriptor heap
	//5. When building the paged global constant buffer state entries, attempt to fill the shader-accessible descriptor
	//   heaps. If we hit the max size, stop writing, but continue to calculate the required size.
	//6. If we ran out of space, allocate a new shader-accessible descriptor heap which is large enough (plus some
	//   reserved extra), copy the fixed descriptor handles in again, and run through the global constant buffer state
	//   entries again to fill it. Since the shader-accessible heaps won't shrink, this would allow them to grow to
	//   size, without preallocating massive arrays.
	//7. If the required total descriptor heap size exceeds hardware limits for a single descriptor heap, spin up
	//   "pages" for the shader accessible heaps, and drop a "marker" on the last renderable which uses the old heap.
	//8. During rendering, if we reach a marker for a descriptor heap transition, change the bound heaps at that point.
	//   This is reportedly very expensive, hence why we only do this if we absolutely need to.
	_heaps.reserve(HeapCount);
	_heaps.emplace_back(_log, _device, this, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1000000);
	_heaps.emplace_back(_log, _device, this, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, _pageSize);
	_heaps.emplace_back(_log, _device, this, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, _pageSize);
	_heaps.emplace_back(_log, _device, this, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, 2048);
}

//----------------------------------------------------------------------------------------
// Allocation methods
//----------------------------------------------------------------------------------------
void Direct3DHeapManager::PreAllocateHeapForType(ResourceType type)
{
	_heaps[ResourceTypeToHeapIndex(type)].EnsureHeapExists();
}

//----------------------------------------------------------------------------------------
std::unique_ptr<DescriptorHandle> Direct3DHeapManager::AllocateDescriptor(ResourceType type)
{
	return _heaps[ResourceTypeToHeapIndex(type)].AllocateDescriptor();
}

//----------------------------------------------------------------------------------------
// Heap object methods
//----------------------------------------------------------------------------------------
const std::vector<ID3D12DescriptorHeap*>& Direct3DHeapManager::GetShaderVisibleDescriptorHeaps() const
{
	return _shaderVisibleDescriptorHeaps;
}

//----------------------------------------------------------------------------------------
void Direct3DHeapManager::NotifyDescriptorHeapAdded(ID3D12DescriptorHeap* descriptorHeap, bool heapIsShaderVisible)
{
	if (heapIsShaderVisible)
	{
		_shaderVisibleDescriptorHeaps.push_back(descriptorHeap);
	}
}

//----------------------------------------------------------------------------------------
// Helper methods
//----------------------------------------------------------------------------------------
constexpr uint32_t Direct3DHeapManager::ResourceTypeToHeapIndex(ResourceType type)
{
	switch (type)
	{
	case ResourceType::ConstantBufferView:
	case ResourceType::ShaderResourceView:
	case ResourceType::UnorderedAccessView:
		return 0;
	case ResourceType::RenderTargetView:
		return 1;
	case ResourceType::DepthStencilView:
		return 2;
	case ResourceType::Sampler:
		return 3;
	}
	return 0;
}

} // namespace cobalt::graphics
