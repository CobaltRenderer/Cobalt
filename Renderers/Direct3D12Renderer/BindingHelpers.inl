// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "Direct3DShaderProgram.h"
namespace cobalt::graphics {

//----------------------------------------------------------------------------------------
IConstantStateValueInfo::~IConstantStateValueInfo() = default;

//----------------------------------------------------------------------------------------
IStateValueInfo::~IStateValueInfo() = default;

//----------------------------------------------------------------------------------------
ITextureBindingInfo::~ITextureBindingInfo() = default;

//----------------------------------------------------------------------------------------
ISamplerBindingInfo::~ISamplerBindingInfo() = default;

//----------------------------------------------------------------------------------------
// ConstantStateValueInfo methods
//----------------------------------------------------------------------------------------
template<class ValueType>
ConstantStateValueInfo<ValueType>::ConstantStateValueInfo(StateValueId stateId, const ValueType& value, const size_t* arrayIndices, size_t arrayIndexCount)
: _stateId(stateId), _value(value), _arrayIndices(arrayIndices, arrayIndices + arrayIndexCount)
{}

//----------------------------------------------------------------------------------------
template<class ValueType>
StateValueId ConstantStateValueInfo<ValueType>::GetAttributeId() const
{
	return _stateId;
}

//----------------------------------------------------------------------------------------
template<class ValueType>
size_t ConstantStateValueInfo<ValueType>::GetArrayIndexCount() const
{
	return _arrayIndices.size();
}

//----------------------------------------------------------------------------------------
template<class ValueType>
const size_t* ConstantStateValueInfo<ValueType>::GetArrayIndices() const
{
	return _arrayIndices.data();
}

//----------------------------------------------------------------------------------------
template<class ValueType>
void ConstantStateValueInfo<ValueType>::ApplyValue(Direct3DShaderProgram* program) const
{
	program->SetConstantValue(_stateId, reinterpret_cast<const uint8_t*>(_value.data()), sizeof(_value), _arrayIndices.data(), _arrayIndices.size());
}

//----------------------------------------------------------------------------------------
template<>
inline void ConstantStateValueInfo<bool>::ApplyValue(Direct3DShaderProgram* program) const
{
	int val = (int)_value;
	program->SetConstantValue(_stateId, reinterpret_cast<const uint8_t*>(&val), sizeof(val), _arrayIndices.data(), _arrayIndices.size());
}

//----------------------------------------------------------------------------------------
template<>
inline void ConstantStateValueInfo<M2Float32>::ApplyValue(Direct3DShaderProgram* program) const
{
	program->SetConstantValue(_stateId, reinterpret_cast<const uint8_t*>(_value.data()), sizeof(_value), _arrayIndices.data(), _arrayIndices.size());
}

//----------------------------------------------------------------------------------------
template<>
inline void ConstantStateValueInfo<M3Float32>::ApplyValue(Direct3DShaderProgram* program) const
{
	program->SetConstantValue(_stateId, reinterpret_cast<const uint8_t*>(_value.data()), sizeof(_value), _arrayIndices.data(), _arrayIndices.size());
}

//----------------------------------------------------------------------------------------
template<>
inline void ConstantStateValueInfo<M4Float32>::ApplyValue(Direct3DShaderProgram* program) const
{
	program->SetConstantValue(_stateId, reinterpret_cast<const uint8_t*>(_value.data()), sizeof(_value), _arrayIndices.data(), _arrayIndices.size());
}

//----------------------------------------------------------------------------------------
// StateValueInfo methods
//----------------------------------------------------------------------------------------
template<class ValueType>
StateValueInfo<ValueType>::StateValueInfo(StateValueId stateId, const ValueType& value, const size_t* arrayIndices, size_t arrayIndexCount)
: _stateId(stateId), _value(value), _arrayIndices(arrayIndices, arrayIndices + arrayIndexCount)
{}

//----------------------------------------------------------------------------------------
template<class ValueType>
StateValueId StateValueInfo<ValueType>::GetAttributeId() const
{
	return _stateId;
}

//----------------------------------------------------------------------------------------
template<class ValueType>
size_t StateValueInfo<ValueType>::GetArrayIndexCount() const
{
	return _arrayIndices.size();
}

//----------------------------------------------------------------------------------------
template<class ValueType>
const size_t* StateValueInfo<ValueType>::GetArrayIndices() const
{
	return _arrayIndices.data();
}

//----------------------------------------------------------------------------------------
template<class ValueType>
void StateValueInfo<ValueType>::ApplyValue(Direct3DShaderProgram* program) const
{
	program->UpdateStateValue(_stateId, reinterpret_cast<const uint8_t*>(_value.data()), sizeof(_value), _arrayIndices.data(), _arrayIndices.size());
}

//----------------------------------------------------------------------------------------
template<>
inline void StateValueInfo<bool>::ApplyValue(Direct3DShaderProgram* program) const
{
	int val = (int)_value;
	program->UpdateStateValue(_stateId, reinterpret_cast<const uint8_t*>(&val), sizeof(val), _arrayIndices.data(), _arrayIndices.size());
}

//----------------------------------------------------------------------------------------
template<>
inline void StateValueInfo<M2Float32>::ApplyValue(Direct3DShaderProgram* program) const
{
	program->UpdateStateValue(_stateId, reinterpret_cast<const uint8_t*>(_value.data()), sizeof(_value), _arrayIndices.data(), _arrayIndices.size());
}

//----------------------------------------------------------------------------------------
template<>
inline void StateValueInfo<M3Float32>::ApplyValue(Direct3DShaderProgram* program) const
{
	program->UpdateStateValue(_stateId, reinterpret_cast<const uint8_t*>(_value.data()), sizeof(_value), _arrayIndices.data(), _arrayIndices.size());
}

//----------------------------------------------------------------------------------------
template<>
inline void StateValueInfo<M4Float32>::ApplyValue(Direct3DShaderProgram* program) const
{
	program->UpdateStateValue(_stateId, reinterpret_cast<const uint8_t*>(_value.data()), sizeof(_value), _arrayIndices.data(), _arrayIndices.size());
}

//----------------------------------------------------------------------------------------
// TextureBindingInfo methods
//----------------------------------------------------------------------------------------
template<class T>
TextureBindingInfo<T>::TextureBindingInfo(TextureId textureId, T* texture)
: _textureId(textureId), _texture(texture)
{}

//----------------------------------------------------------------------------------------
template<class T>
TextureId TextureBindingInfo<T>::GetTextureId() const
{
	return _textureId;
}

//----------------------------------------------------------------------------------------
template<class T>
void TextureBindingInfo<T>::BindTexture(const Direct3DRenderer* renderer, const Direct3DShaderProgram* program, ID3D12GraphicsCommandList* commandList, bool computeShaderBinding)
{
	// Obtain the bind points for this resource
	if (!_retrievedBindPoints)
	{
		program->GetBindPointsForTexture(_textureId, _bindPoints, _bindPointCount);
		_retrievedBindPoints = true;
	}

	// Bind this resource to each defined bind point
	for (size_t i = 0; i < _bindPointCount; ++i)
	{
		const auto& bindPoint = _bindPoints[i];
		if (computeShaderBinding)
		{
			commandList->SetComputeRootDescriptorTable(bindPoint.bindPoint.rootParameterIndex, _texture->GetGPUDescriptorHandle());
		}
		else
		{
			commandList->SetGraphicsRootDescriptorTable(bindPoint.bindPoint.rootParameterIndex, _texture->GetGPUDescriptorHandle());
		}
	}
}

//----------------------------------------------------------------------------------------
template<class T>
void TextureBindingInfo<T>::UnbindTexture(const Direct3DRenderer* renderer, const Direct3DShaderProgram* program, ID3D12GraphicsCommandList* commandList, bool computeShaderBinding)
{
	for (size_t i = 0; i < _bindPointCount; ++i)
	{
		const auto& bindPoint = _bindPoints[i];
		if (computeShaderBinding)
		{
			commandList->SetComputeRootDescriptorTable(bindPoint.bindPoint.rootParameterIndex, CD3DX12_GPU_DESCRIPTOR_HANDLE(CD3DX12_DEFAULT()));
		}
		else
		{
			commandList->SetGraphicsRootDescriptorTable(bindPoint.bindPoint.rootParameterIndex, CD3DX12_GPU_DESCRIPTOR_HANDLE(CD3DX12_DEFAULT()));
		}
	}
}

//----------------------------------------------------------------------------------------
// TextureBindingWithCombinedSamplerInfo methods
//----------------------------------------------------------------------------------------
template<class TextureType, class SamplerType>
TextureBindingWithCombinedSamplerInfo<TextureType, SamplerType>::TextureBindingWithCombinedSamplerInfo(TextureId textureId, TextureType* texture, SamplerType* sampler)
: _textureId(textureId), _texture(texture), _sampler(sampler)
{}

//----------------------------------------------------------------------------------------
template<class TextureType, class SamplerType>
TextureId TextureBindingWithCombinedSamplerInfo<TextureType, SamplerType>::GetTextureId() const
{
	return _textureId;
}

//----------------------------------------------------------------------------------------
template<class TextureType, class SamplerType>
void TextureBindingWithCombinedSamplerInfo<TextureType, SamplerType>::BindTexture(const Direct3DRenderer* renderer, const Direct3DShaderProgram* program, ID3D12GraphicsCommandList* commandList, bool computeShaderBinding)
{
	// Obtain the bind points for this resource
	if (!_retrievedBindingPoints)
	{
		program->GetBindPointsForTexture(_textureId, _bindPoints, _bindPointCount);
		_retrievedBindingPoints = true;
	}

	// Bind this resource to each defined bind point
	for (size_t i = 0; i < _bindPointCount; ++i)
	{
		const auto& bindPoint = _bindPoints[i];
		if (computeShaderBinding)
		{
			commandList->SetComputeRootDescriptorTable(bindPoint.bindPoint.rootParameterIndex, _texture->GetGPUDescriptorHandle());
			if (bindPoint.hasLinkedSampler)
			{
				commandList->SetComputeRootDescriptorTable(bindPoint.combinedSamplerBindPoint.rootParameterIndex, _sampler->GetGPUDescriptorHandle());
			}
		}
		else
		{
			commandList->SetGraphicsRootDescriptorTable(bindPoint.bindPoint.rootParameterIndex, _texture->GetGPUDescriptorHandle());
			if (bindPoint.hasLinkedSampler)
			{
				commandList->SetGraphicsRootDescriptorTable(bindPoint.combinedSamplerBindPoint.rootParameterIndex, _sampler->GetGPUDescriptorHandle());
			}
		}
	}
}

//----------------------------------------------------------------------------------------
template<class TextureType, class SamplerType>
void TextureBindingWithCombinedSamplerInfo<TextureType, SamplerType>::UnbindTexture(const Direct3DRenderer* renderer, const Direct3DShaderProgram* program, ID3D12GraphicsCommandList* commandList, bool computeShaderBinding)
{
	for (size_t i = 0; i < _bindPointCount; ++i)
	{
		const auto& bindPoint = _bindPoints[i];
		if (computeShaderBinding)
		{
			commandList->SetComputeRootDescriptorTable(bindPoint.bindPoint.rootParameterIndex, CD3DX12_GPU_DESCRIPTOR_HANDLE(CD3DX12_DEFAULT()));
			if (bindPoint.hasLinkedSampler)
			{
				commandList->SetComputeRootDescriptorTable(bindPoint.combinedSamplerBindPoint.rootParameterIndex, CD3DX12_GPU_DESCRIPTOR_HANDLE(CD3DX12_DEFAULT()));
			}
		}
		else
		{
			commandList->SetGraphicsRootDescriptorTable(bindPoint.bindPoint.rootParameterIndex, CD3DX12_GPU_DESCRIPTOR_HANDLE(CD3DX12_DEFAULT()));
			if (bindPoint.hasLinkedSampler)
			{
				commandList->SetGraphicsRootDescriptorTable(bindPoint.combinedSamplerBindPoint.rootParameterIndex, CD3DX12_GPU_DESCRIPTOR_HANDLE(CD3DX12_DEFAULT()));
			}
		}
	}
}

//----------------------------------------------------------------------------------------
// SamplerBindingInfo methods
//----------------------------------------------------------------------------------------
template<class T>
SamplerBindingInfo<T>::SamplerBindingInfo(SamplerId samplerId, T* sampler)
: _samplerId(samplerId), _sampler(sampler)
{}

//----------------------------------------------------------------------------------------
template<class T>
SamplerId SamplerBindingInfo<T>::GetSamplerId() const
{
	return _samplerId;
}

//----------------------------------------------------------------------------------------
template<class T>
void SamplerBindingInfo<T>::BindSampler(const Direct3DRenderer* renderer, const Direct3DShaderProgram* program, ID3D12GraphicsCommandList* commandList, bool computeShaderBinding)
{
	// Obtain the bind points for this resource
	if (!_retrievedBindPoints)
	{
		program->GetBindPointsForSampler(_samplerId, _bindPoints, _bindPointCount);
		_retrievedBindPoints = true;
	}

	// Bind this resource to each defined bind point
	for (size_t i = 0; i < _bindPointCount; ++i)
	{
		const auto& bindPoint = _bindPoints[i];
		if (computeShaderBinding)
		{
			commandList->SetComputeRootDescriptorTable(bindPoint.bindPoint.rootParameterIndex, _sampler->GetGPUDescriptorHandle());
		}
		else
		{
			commandList->SetGraphicsRootDescriptorTable(bindPoint.bindPoint.rootParameterIndex, _sampler->GetGPUDescriptorHandle());
		}
	}
}

//----------------------------------------------------------------------------------------
template<class T>
void SamplerBindingInfo<T>::UnbindSampler(const Direct3DRenderer* renderer, const Direct3DShaderProgram* program, ID3D12GraphicsCommandList* commandList, bool computeShaderBinding)
{
	for (size_t i = 0; i < _bindPointCount; ++i)
	{
		const auto& bindPoint = _bindPoints[i];
		if (computeShaderBinding)
		{
			commandList->SetComputeRootDescriptorTable(bindPoint.bindPoint.rootParameterIndex, CD3DX12_GPU_DESCRIPTOR_HANDLE(CD3DX12_DEFAULT()));
		}
		else
		{
			commandList->SetGraphicsRootDescriptorTable(bindPoint.bindPoint.rootParameterIndex, CD3DX12_GPU_DESCRIPTOR_HANDLE(CD3DX12_DEFAULT()));
		}
	}
}

} // namespace cobalt::graphics
