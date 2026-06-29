// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include "CommandListHandle.h"
#include <Cobalt/LoggingInterface/LoggingInterface.pkg>
#include <mutex>
#include <unordered_set>
#include <vector>
namespace cobalt::graphics {

class Direct3DCommandListPool
{
public:
	// Friend declarations
	friend class CommandListHandle;

public:
	// Constructors
	Direct3DCommandListPool(cobalt::logging::ILogger* log, ID3D12Device* device, ID3D12CommandQueue* commandQueue, D3DX12Residency::ResidencyManager* residencyManager, uint32_t initialPoolSize = 0);
	~Direct3DCommandListPool();

	// Allocation methods
	CommandListHandle AllocateCommandList(bool submitOnFree = true);
	void ResetDirtyCommandAllocators();

private:
	// Constants
	static const uint32_t DefaultPoolSize = 100;

	// Structures
	struct CommandListEntry
	{
		bool allocatedObjects = false;
		Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocator;
		Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList;
		D3DX12Residency::ResidencySet* residencySet = nullptr;
	};

private:
	// Allocation methods
	void FreeCommandList(uint32_t allocationID, ID3D12GraphicsCommandList* commandList, D3DX12Residency::ResidencySet* residencySet, bool submitCommandList);

private:
	cobalt::logging::ILogger* _log;
	mutable std::mutex _accessMutex;
	ID3D12Device* _device;
	ID3D12CommandQueue* _commandQueue;
	D3DX12Residency::ResidencyManager* _residencyManager;
	std::vector<CommandListEntry> _commandLists;
	std::vector<uint32_t> _freeSlots;
	std::unordered_set<uint32_t> _dirtySlots;
};

} // namespace cobalt::graphics
