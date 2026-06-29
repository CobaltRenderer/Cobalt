// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "CommandListHandle.h"
#include "Direct3DCommandListPool.h"
namespace cobalt::graphics {

//----------------------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------------------
CommandListHandle::CommandListHandle(Direct3DCommandListPool* commandListPool, ID3D12GraphicsCommandList* commandList, D3DX12Residency::ResidencySet* residencySet, uint32_t allocationID, bool submitOnFree)
: _commandListPool(commandListPool), _commandList(commandList), _residencySet(residencySet), _allocationID(allocationID), _listAllocated(true), _submitOnFree(submitOnFree)
{}

//----------------------------------------------------------------------------------------
CommandListHandle::CommandListHandle(CommandListHandle&& source) noexcept
{
	*this = std::move(source);
}

//----------------------------------------------------------------------------------------
CommandListHandle::~CommandListHandle()
{
	FreeHandle();
}

//----------------------------------------------------------------------------------------
// Assignment operators
//----------------------------------------------------------------------------------------
CommandListHandle& CommandListHandle::operator=(CommandListHandle&& source) noexcept
{
	// Clear any descriptor handle we are currently holding
	FreeHandle();

	// Take ownership of the descriptor handle from the source object
	_commandListPool = source._commandListPool;
	_commandList = source._commandList;
	_allocationID = source._allocationID;
	_listAllocated = source._listAllocated;
	source._listAllocated = false;
	return *this;
}

//----------------------------------------------------------------------------------------
// Handle methods
//----------------------------------------------------------------------------------------
void CommandListHandle::SetSubmitOnFree(bool state)
{
	_submitOnFree = state;
}

//----------------------------------------------------------------------------------------
void CommandListHandle::SubmitCommandList()
{
	_submitOnFree = true;
	FreeHandle();
}

//----------------------------------------------------------------------------------------
ID3D12GraphicsCommandList* CommandListHandle::GetCommandList() const
{
	return _commandList;
}

//----------------------------------------------------------------------------------------
D3DX12Residency::ResidencySet* CommandListHandle::GetResidencySet() const
{
	return _residencySet;
}

//----------------------------------------------------------------------------------------
void CommandListHandle::FreeHandle()
{
	if (_listAllocated)
	{
		_commandListPool->FreeCommandList(_allocationID, _commandList, _residencySet, _submitOnFree);
		_listAllocated = false;
	}
}

} // namespace cobalt::graphics
