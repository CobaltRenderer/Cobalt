// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "BindingHelpers.h"
#include "Direct3DDataArray.h"
#include "Direct3DTexelArray.h"
#include <Internal/RendererSupport/KnownDynamicCast.h>
namespace cobalt::graphics {

//----------------------------------------------------------------------------------------
// StateBufferBindingInfo methods
//----------------------------------------------------------------------------------------
StateBufferBindingInfo::StateBufferBindingInfo(StateBufferId stateBufferId, uint32_t stateBufferPageNo, IStateBuffer* stateBuffer)
: _stateBufferId(stateBufferId), _stateBufferPageNo(stateBufferPageNo), _stateBuffer(stateBuffer)
{}

//----------------------------------------------------------------------------------------
StateBufferId StateBufferBindingInfo::GetStateBufferId() const
{
	return _stateBufferId;
}

//----------------------------------------------------------------------------------------
void StateBufferBindingInfo::BindStateBuffer(const Direct3DRenderer* renderer, const Direct3DShaderProgram* program, ID3D12GraphicsCommandList* commandList, bool computeShaderBinding)
{
	program->BindConstantBuffer(commandList, _stateBuffer, _stateBufferId, _stateBufferPageNo, computeShaderBinding);
}

//----------------------------------------------------------------------------------------
void StateBufferBindingInfo::UnbindStateBuffer(const Direct3DRenderer* renderer, const Direct3DShaderProgram* program, ID3D12GraphicsCommandList* commandList, bool computeShaderBinding)
{
	program->UnbindConstantBuffer(commandList, _stateBuffer, _stateBufferId, _stateBufferPageNo, computeShaderBinding);
}

//----------------------------------------------------------------------------------------
// ResourceArrayBindingInfo methods
//----------------------------------------------------------------------------------------
ResourceArrayBindingInfo::ResourceArrayBindingInfo(ResourceArrayId resourceArrayId, IDataArray* resourceBuffer, bool resetCounter)
: _resourceArrayId(resourceArrayId), _dataArray(KnownDynamicCast<Direct3DDataArray*>(resourceBuffer)), _resetCounter(resetCounter), _isDataArrayBinding(true)
{}

//----------------------------------------------------------------------------------------
ResourceArrayBindingInfo::ResourceArrayBindingInfo(ResourceArrayId resourceArrayId, ITexelArray* resourceBuffer)
: _resourceArrayId(resourceArrayId), _texelArray(KnownDynamicCast<Direct3DTexelArray*>(resourceBuffer)), _isDataArrayBinding(false)
{}

//----------------------------------------------------------------------------------------
ResourceArrayId ResourceArrayBindingInfo::GetResourceArrayId() const
{
	return _resourceArrayId;
}

//----------------------------------------------------------------------------------------
void ResourceArrayBindingInfo::BindResourceArray(const Direct3DRenderer* renderer, const Direct3DShaderProgram* program, ID3D12GraphicsCommandList* commandList, bool performReset, bool computeShaderBinding)
{
	// Obtain the bind points for this resource
	if (!_retrievedBindPoints)
	{
		program->GetBindPointsForResourceArray(_resourceArrayId, _bindPoints, _bindPointCount);
		_retrievedBindPoints = true;
	}

	// Bind this resource to each defined bind point
	for (size_t i = 0; i < _bindPointCount; ++i)
	{
		// Retrieve the next bind point
		const auto& bindPoint = _bindPoints[i];

		// Bind the buffer
		if (bindPoint.hasReadOnlyBinding)
		{
			const auto& readOnlyDescriptorHandle = (_isDataArrayBinding ? _dataArray->GetReadOnlyGPUDescriptorHandle(commandList, bindPoint.type) : _texelArray->GetReadOnlyGPUDescriptorHandle(commandList, bindPoint.type));
			if (computeShaderBinding)
			{
				commandList->SetComputeRootDescriptorTable(bindPoint.readOnlyBindPoint.rootParameterIndex, readOnlyDescriptorHandle);
			}
			else
			{
				commandList->SetGraphicsRootDescriptorTable(bindPoint.readOnlyBindPoint.rootParameterIndex, readOnlyDescriptorHandle);
			}
		}
		if (bindPoint.hasReadWriteBinding)
		{
			const auto& readWriteDescriptorHandle = (_isDataArrayBinding ? _dataArray->GetReadWriteGPUDescriptorHandle(commandList, bindPoint.type) : _texelArray->GetReadWriteGPUDescriptorHandle(commandList, bindPoint.type));
			if (computeShaderBinding)
			{
				commandList->SetComputeRootDescriptorTable(bindPoint.readWriteBindPoint.rootParameterIndex, readWriteDescriptorHandle);
			}
			else
			{
				commandList->SetGraphicsRootDescriptorTable(bindPoint.readWriteBindPoint.rootParameterIndex, readWriteDescriptorHandle);
			}
		}
	}

	// Reset the attached counter value if required
	if (_isDataArrayBinding && performReset && _resetCounter && _dataArray->HasCounter())
	{
		_dataArray->ResetCounter(commandList);
	}

	// Ensure the target buffer has been added as a current resource buffer
	if (_isDataArrayBinding)
	{
		_dataArray->AddAsCurrentBuffer();
	}
	else
	{
		_texelArray->AddAsCurrentBuffer();
	}
}

//----------------------------------------------------------------------------------------
void ResourceArrayBindingInfo::UnbindResourceArray(const Direct3DRenderer* renderer, const Direct3DShaderProgram* program, ID3D12GraphicsCommandList* commandList, bool computeShaderBinding)
{
	for (size_t i = 0; i < _bindPointCount; ++i)
	{
		const auto& bindPoint = _bindPoints[i];
		if (bindPoint.hasReadOnlyBinding)
		{
			if (computeShaderBinding)
			{
				commandList->SetComputeRootDescriptorTable(bindPoint.readOnlyBindPoint.rootParameterIndex, CD3DX12_GPU_DESCRIPTOR_HANDLE(CD3DX12_DEFAULT()));
			}
			else
			{
				commandList->SetGraphicsRootDescriptorTable(bindPoint.readOnlyBindPoint.rootParameterIndex, CD3DX12_GPU_DESCRIPTOR_HANDLE(CD3DX12_DEFAULT()));
			}
		}
		if (bindPoint.hasReadWriteBinding)
		{
			if (computeShaderBinding)
			{
				commandList->SetComputeRootDescriptorTable(bindPoint.readWriteBindPoint.rootParameterIndex, CD3DX12_GPU_DESCRIPTOR_HANDLE(CD3DX12_DEFAULT()));
			}
			else
			{
				commandList->SetGraphicsRootDescriptorTable(bindPoint.readWriteBindPoint.rootParameterIndex, CD3DX12_GPU_DESCRIPTOR_HANDLE(CD3DX12_DEFAULT()));
			}
		}
	}
}

//----------------------------------------------------------------------------------------
bool ResourceArrayBindingInfo::HasCaptureTargets() const
{
	return (_isDataArrayBinding ? _dataArray->HasCaptureTargets() : _texelArray->HasCaptureTargets());
}

} // namespace cobalt::graphics
