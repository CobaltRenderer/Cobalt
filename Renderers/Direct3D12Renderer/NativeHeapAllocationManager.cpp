// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "NativeHeapAllocationManager.h"
#include <numeric>
namespace cobalt::graphics {

//----------------------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------------------
NativeHeapAllocationManager::NativeHeapAllocationManager(uint32_t heapSize)
: _freeSlots(heapSize)
{
	// Fill our list of free slots with all available slot numbers
	std::iota(_freeSlots.begin(), _freeSlots.end(), 0);
}

//----------------------------------------------------------------------------------------
// Allocation methods
//----------------------------------------------------------------------------------------
bool NativeHeapAllocationManager::TryAllocateSlot(uint32_t& slotNo)
{
	// If there are no free slots, return false.
	if (_freeSlots.empty())
	{
		return false;
	}

	// Retrieve the next free slot and return it to the caller
	slotNo = _freeSlots.back();
	_freeSlots.pop_back();
	return true;
}

//----------------------------------------------------------------------------------------
void NativeHeapAllocationManager::FreeSlot(uint32_t slotNo)
{
	// Add the specified slot to the list of free slots
	_freeSlots.push_back(slotNo);
}

} // namespace cobalt::graphics
