// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include <vector>
namespace cobalt::graphics {

class NativeHeapAllocationManager
{
public:
	// Constructors
	explicit NativeHeapAllocationManager(uint32_t heapSize);

	// Allocation methods
	bool TryAllocateSlot(uint32_t& slotNo);
	void FreeSlot(uint32_t slotNo);

private:
	std::vector<uint32_t> _freeSlots;
};

} // namespace cobalt::graphics
