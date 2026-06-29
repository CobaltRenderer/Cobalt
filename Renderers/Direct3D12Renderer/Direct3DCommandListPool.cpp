// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "Direct3DCommandListPool.h"
#include <numeric>
namespace cobalt::graphics {

//----------------------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------------------
Direct3DCommandListPool::Direct3DCommandListPool(cobalt::logging::ILogger* log, ID3D12Device* device, ID3D12CommandQueue* commandQueue, D3DX12Residency::ResidencyManager* residencyManager, uint32_t initialPoolSize)
: _log(log), _device(device), _commandQueue(commandQueue), _residencyManager(residencyManager)
{
	// Determine the initial pool size
	initialPoolSize = (initialPoolSize == 0 ? DefaultPoolSize : initialPoolSize);

	// Create the initial pool structure
	CommandListEntry defaultEntry;
	defaultEntry.allocatedObjects = false;
	_commandLists.resize(initialPoolSize, defaultEntry);
	_freeSlots.resize(initialPoolSize);
	std::iota(_freeSlots.begin(), _freeSlots.end(), 0);
}

//----------------------------------------------------------------------------------------
Direct3DCommandListPool::~Direct3DCommandListPool()
{
	// Release our allocated objects
	for (CommandListEntry& entry : _commandLists)
	{
		if (entry.allocatedObjects)
		{
			_residencyManager->DestroyResidencySet(entry.residencySet);
			entry.residencySet = nullptr;
			entry.commandList.Reset();
			entry.commandAllocator.Reset();
		}
	}
}

//----------------------------------------------------------------------------------------
// Allocation methods
//----------------------------------------------------------------------------------------
CommandListHandle Direct3DCommandListPool::AllocateCommandList(bool submitOnFree)
{
	// If there are no more available free slots in the pool, grow the pool to allow for more entries.
	std::unique_lock<std::mutex> lock(_accessMutex);
	if (_freeSlots.empty())
	{
		size_t initialSize = _commandLists.size();
		CommandListEntry defaultEntry;
		defaultEntry.allocatedObjects = false;
		_commandLists.resize(initialSize * 2, defaultEntry);
		std::iota(_freeSlots.begin(), _freeSlots.end(), (uint32_t)initialSize);
	}

	// Retrieve the next available slot in the pool
	uint32_t slotNo = _freeSlots.back();
	_freeSlots.pop_back();
	_dirtySlots.insert(slotNo);
	lock.unlock();

	// Retrieve the command list and residency set for this slot, creating or initializing them as necessary.
	ID3D12GraphicsCommandList* commandList = nullptr;
	D3DX12Residency::ResidencySet* residencySet = nullptr;
	CommandListEntry& commandListEntry = _commandLists[slotNo];
	if (!commandListEntry.allocatedObjects)
	{
		// Create the command allocator
		HRESULT createCommandAllocatorReturn = _device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_COPY, IID_PPV_ARGS(&commandListEntry.commandAllocator));
		if (FAILED(createCommandAllocatorReturn))
		{
			_log->Error("CreateCommandAllocator failed in command list pool with error code {0}", createCommandAllocatorReturn);
		}

		// Create a new command list
		HRESULT createCommandListReturn = _device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_COPY, commandListEntry.commandAllocator.Get(), nullptr, IID_PPV_ARGS(&commandListEntry.commandList));
		if (FAILED(createCommandListReturn))
		{
			_log->Error("CreateCommandList failed in command list pool with error code {0}", createCommandListReturn);
		}
		commandList = commandListEntry.commandList.Get();

		// Create a new residency set
		residencySet = _residencyManager->CreateResidencySet();
		commandListEntry.residencySet = residencySet;
		commandListEntry.allocatedObjects = true;
	}
	else
	{
		commandList = commandListEntry.commandList.Get();
		commandList->Reset(commandListEntry.commandAllocator.Get(), nullptr);
		residencySet = commandListEntry.residencySet;
	}

	// Open the residency set. Note that there's no explicit reset for this object, but they can be reused after being
	// closed.
	if (FAILED(residencySet->Open()))
	{
		_log->Error("Failed to open residency set in command list pool");
	}

	// Return a command list handle to the caller
	return CommandListHandle(this, commandList, residencySet, slotNo, submitOnFree);
}

//----------------------------------------------------------------------------------------
void Direct3DCommandListPool::FreeCommandList(uint32_t allocationID, ID3D12GraphicsCommandList* commandList, D3DX12Residency::ResidencySet* residencySet, bool submitCommandList)
{
	// Close the residency set
	if (FAILED(residencySet->Close()))
	{
		_log->Error("Failed to close residency set in command list pool");
	}

	// Close the command list
	if (FAILED(commandList->Close()))
	{
		_log->Error("Failed to close command list in command list pool");
	}

	// Submit the command list if requested
	if (submitCommandList)
	{
		ID3D12CommandList* commandListPointer = commandList;
		_residencyManager->ExecuteCommandLists(_commandQueue, &commandListPointer, &residencySet, 1);
	}

	// Mark that the specified command list slot is free
	std::unique_lock<std::mutex> lock(_accessMutex);
	_freeSlots.push_back(allocationID);
}

//----------------------------------------------------------------------------------------
void Direct3DCommandListPool::ResetDirtyCommandAllocators()
{
	// Reset the command allocators for all dirty slots
	for (uint32_t slotNo : _dirtySlots)
	{
		if (FAILED(_commandLists[slotNo].commandAllocator->Reset()))
		{
			_log->Error("Failed to reset command allocator from pool with slot no {0}", slotNo);
		}
	}
	_dirtySlots.clear();
}

} // namespace cobalt::graphics
