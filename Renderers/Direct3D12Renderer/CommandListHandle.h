// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include "Direct3DHeaders.h"
#include <Cobalt/Debug/Debug.pkg>
#include <dxgi1_4.h>
WARNINGS_PUSH_OFF
#ifdef _MSC_VER
#pragma warning(disable : 4555)
#pragma warning(disable : 5219)
#pragma warning(disable : 6031)
#endif
#include <d3dx12Residency.h>
WARNINGS_POP
#include <cstdint>
namespace cobalt::graphics {
class Direct3DCommandListPool;

class CommandListHandle
{
public:
	// Constructors
	CommandListHandle(Direct3DCommandListPool* commandListPool, ID3D12GraphicsCommandList* commandList, D3DX12Residency::ResidencySet* residencySet, uint32_t allocationID, bool submitOnFree);
	CommandListHandle(CommandListHandle&& source) noexcept;
	CommandListHandle(const CommandListHandle& source) = delete;
	CommandListHandle& operator=(const CommandListHandle& source) = delete;
	~CommandListHandle();

	// Assignment operators
	CommandListHandle& operator=(CommandListHandle&& source) noexcept;

	// Handle methods
	void SetSubmitOnFree(bool state);
	void SubmitCommandList();
	ID3D12GraphicsCommandList* GetCommandList() const;
	D3DX12Residency::ResidencySet* GetResidencySet() const;

private:
	// Handle methods
	void FreeHandle();

private:
	Direct3DCommandListPool* _commandListPool = {};
	bool _listAllocated = false;
	uint32_t _allocationID = 0;
	bool _submitOnFree = false;
	ID3D12GraphicsCommandList* _commandList = {};
	D3DX12Residency::ResidencySet* _residencySet = {};
};

} // namespace cobalt::graphics
