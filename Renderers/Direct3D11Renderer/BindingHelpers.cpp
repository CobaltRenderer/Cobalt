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
void StateBufferBindingInfo::BindStateBuffer(ID3D11Device1* device, ID3D11DeviceContext1* context, Direct3DShaderProgram* program)
{
	program->BindConstantBuffer(_stateBuffer, _stateBufferId, _stateBufferPageNo, context);
}

//----------------------------------------------------------------------------------------
void StateBufferBindingInfo::UnbindStateBuffer(ID3D11Device1* device, ID3D11DeviceContext1* context, Direct3DShaderProgram* program)
{
	program->UnbindConstantBuffer(_stateBuffer, _stateBufferId, _stateBufferPageNo, context);
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
void ResourceArrayBindingInfo::BindResourceArray(ID3D11Device1* device, ID3D11DeviceContext1* context, Direct3DShaderProgram* program, bool performReset, std::vector<ID3D11UnorderedAccessView*>& resourceBufferViews, std::vector<UINT>& resetValues, UINT& lowestBindPoint)
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

		// If a read-only view of the buffer is present, bind it immediately.
		if (bindPoint.hasReadOnlyBinding)
		{
			auto* resourceView = (_isDataArrayBinding ? _dataArray->GetReadOnlyView(bindPoint.type) : _texelArray->GetReadOnlyView(bindPoint.type));
			switch (bindPoint.stage)
			{
			case IShaderProgram::ShaderStage::Vertex:
				context->VSSetShaderResources(bindPoint.readOnlyBindPoint, 1, &resourceView);
				break;
			case IShaderProgram::ShaderStage::Fragment:
				context->PSSetShaderResources(bindPoint.readOnlyBindPoint, 1, &resourceView);
				break;
			case IShaderProgram::ShaderStage::Geometry:
				context->GSSetShaderResources(bindPoint.readOnlyBindPoint, 1, &resourceView);
				break;
			case IShaderProgram::ShaderStage::Compute:
				context->CSSetShaderResources(bindPoint.readOnlyBindPoint, 1, &resourceView);
				break;
			}
		}

		// If a read/write view of the buffer is present, add it to the list of buffers to bind.
		if (bindPoint.hasReadWriteBinding)
		{
			// Increase the size of the arrays if required
			if (bindPoint.readWriteBindPoint >= resourceBufferViews.size())
			{
				resourceBufferViews.resize(bindPoint.readWriteBindPoint + 1, nullptr);
				resetValues.resize(bindPoint.readWriteBindPoint + 1, 0xFFFFFFFF);
			}

			// Retrieve the view for this resource array
			resourceBufferViews[bindPoint.readWriteBindPoint] = (_isDataArrayBinding ? _dataArray->GetReadWriteView(bindPoint.type) : _texelArray->GetReadWriteView(bindPoint.type));

			// Retrieve the reset value for this resource array if applicable
			UINT resetValue = 0xFFFFFFFF;
			if (_isDataArrayBinding && performReset && _resetCounter && _dataArray->HasCounter())
			{
				resetValue = _dataArray->GetCounterResetValue();
			}
			resetValues[bindPoint.readWriteBindPoint] = resetValue;

			// Update the lowest binding point value
			lowestBindPoint = std::min(lowestBindPoint, bindPoint.readWriteBindPoint);
		}
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

} // namespace cobalt::graphics
