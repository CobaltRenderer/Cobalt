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
void TextureBindingInfo<T>::BindTexture(ID3D11Device1* device, ID3D11DeviceContext1* context, Direct3DShaderProgram* program)
{
	// Obtain the bind points and shader resource view for this resource
	if (!_retrievedBindPoints)
	{
		_resourceView = _texture->GetShaderResourceView(device);
		program->GetBindPointsForTexture(_textureId, _bindPoints, _bindPointCount);
		_retrievedBindPoints = true;
	}

	// Bind this resource to each defined bind point
	for (size_t i = 0; i < _bindPointCount; ++i)
	{
		const auto& bindPoint = _bindPoints[i];
		switch (bindPoint.stage)
		{
		case IShaderProgram::ShaderStage::Vertex:
			context->VSSetShaderResources(bindPoint.bindPoint, 1, &_resourceView);
			break;
		case IShaderProgram::ShaderStage::Fragment:
			context->PSSetShaderResources(bindPoint.bindPoint, 1, &_resourceView);
			break;
		case IShaderProgram::ShaderStage::Geometry:
			context->GSSetShaderResources(bindPoint.bindPoint, 1, &_resourceView);
			break;
		case IShaderProgram::ShaderStage::Compute:
			context->CSSetShaderResources(bindPoint.bindPoint, 1, &_resourceView);
			break;
		}
	}
}

//----------------------------------------------------------------------------------------
template<class T>
void TextureBindingInfo<T>::UnbindTexture(ID3D11Device1* device, ID3D11DeviceContext1* context, Direct3DShaderProgram* program)
{
	ID3D11ShaderResourceView* shaderResourceView = nullptr;
	for (size_t i = 0; i < _bindPointCount; ++i)
	{
		const auto& bindPoint = _bindPoints[i];
		switch (bindPoint.stage)
		{
		case IShaderProgram::ShaderStage::Vertex:
			context->VSSetShaderResources(bindPoint.bindPoint, 1, &shaderResourceView);
			break;
		case IShaderProgram::ShaderStage::Fragment:
			context->PSSetShaderResources(bindPoint.bindPoint, 1, &shaderResourceView);
			break;
		case IShaderProgram::ShaderStage::Geometry:
			context->GSSetShaderResources(bindPoint.bindPoint, 1, &shaderResourceView);
			break;
		case IShaderProgram::ShaderStage::Compute:
			context->CSSetShaderResources(bindPoint.bindPoint, 1, &shaderResourceView);
			break;
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
void TextureBindingWithCombinedSamplerInfo<TextureType, SamplerType>::BindTexture(ID3D11Device1* device, ID3D11DeviceContext1* context, Direct3DShaderProgram* program)
{
	// Obtain the bind points and shader resource view for this resource
	if (!_retrievedBindingPoints)
	{
		_resourceView = _texture->GetShaderResourceView(device);
		program->GetBindPointsForTexture(_textureId, _bindPoints, _bindPointCount);
		_retrievedBindingPoints = true;
	}

	// Retrieve the current combined sampler state
	ID3D11SamplerState* samplerState = _sampler->GetSamplerState();
	if (samplerState == nullptr)
	{
		return;
	}

	// Bind this resource to each defined bind point
	for (size_t i = 0; i < _bindPointCount; ++i)
	{
		const auto& bindPoint = _bindPoints[i];

		if (bindPoint.combinedSamplerBindPoint != 0xFFFFFFFF)
		{
			// It's possible for a combined sampler to not have a sampler binding point, the Direct3D shader compiler may optimise out the
			// sampler if the image pixels are read unsampled. Because of this, there may not be a sampler in the interface, leaving
			// the sampler binding point empty.
			switch (bindPoint.stage)
			{
			case IShaderProgram::ShaderStage::Vertex:
				context->VSSetSamplers(bindPoint.combinedSamplerBindPoint, 1, &samplerState);
				break;
			case IShaderProgram::ShaderStage::Fragment:
				context->PSSetSamplers(bindPoint.combinedSamplerBindPoint, 1, &samplerState);
				break;
			case IShaderProgram::ShaderStage::Geometry:
				context->GSSetSamplers(bindPoint.combinedSamplerBindPoint, 1, &samplerState);
				break;
			case IShaderProgram::ShaderStage::Compute:
				context->CSSetSamplers(bindPoint.combinedSamplerBindPoint, 1, &samplerState);
				break;
			}
		}

		switch (bindPoint.stage)
		{
		case IShaderProgram::ShaderStage::Vertex:
			context->VSSetShaderResources(bindPoint.bindPoint, 1, &_resourceView);
			break;
		case IShaderProgram::ShaderStage::Fragment:
			context->PSSetShaderResources(bindPoint.bindPoint, 1, &_resourceView);
			break;
		case IShaderProgram::ShaderStage::Geometry:
			context->GSSetShaderResources(bindPoint.bindPoint, 1, &_resourceView);
			break;
		case IShaderProgram::ShaderStage::Compute:
			context->CSSetShaderResources(bindPoint.bindPoint, 1, &_resourceView);
			break;
		}
	}
}

//----------------------------------------------------------------------------------------
template<class TextureType, class SamplerType>
void TextureBindingWithCombinedSamplerInfo<TextureType, SamplerType>::UnbindTexture(ID3D11Device1* device, ID3D11DeviceContext1* context, Direct3DShaderProgram* program)
{
	ID3D11ShaderResourceView* shaderResourceView = nullptr;
	ID3D11SamplerState* samplerState = nullptr;
	for (size_t i = 0; i < _bindPointCount; ++i)
	{
		const auto& bindPoint = _bindPoints[i];

		if (bindPoint.combinedSamplerBindPoint != (UINT)-1)
		{
			switch (bindPoint.stage)
			{
			case IShaderProgram::ShaderStage::Vertex:
				context->VSSetSamplers(bindPoint.combinedSamplerBindPoint, 1, &samplerState);
				break;
			case IShaderProgram::ShaderStage::Fragment:
				context->PSSetSamplers(bindPoint.combinedSamplerBindPoint, 1, &samplerState);
				break;
			case IShaderProgram::ShaderStage::Geometry:
				context->GSSetSamplers(bindPoint.combinedSamplerBindPoint, 1, &samplerState);
				break;
			case IShaderProgram::ShaderStage::Compute:
				context->CSSetSamplers(bindPoint.combinedSamplerBindPoint, 1, &samplerState);
				break;
			}
		}

		switch (bindPoint.stage)
		{
		case IShaderProgram::ShaderStage::Vertex:
			context->VSSetShaderResources(bindPoint.bindPoint, 1, &shaderResourceView);
			break;
		case IShaderProgram::ShaderStage::Fragment:
			context->PSSetShaderResources(bindPoint.bindPoint, 1, &shaderResourceView);
			break;
		case IShaderProgram::ShaderStage::Geometry:
			context->GSSetShaderResources(bindPoint.bindPoint, 1, &shaderResourceView);
			break;
		case IShaderProgram::ShaderStage::Compute:
			context->CSSetShaderResources(bindPoint.bindPoint, 1, &shaderResourceView);
			break;
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
void SamplerBindingInfo<T>::BindSampler(ID3D11Device1* device, ID3D11DeviceContext1* context, Direct3DShaderProgram* program)
{
	// Obtain the bind points for this resource
	if (!_retrievedBindPoints)
	{
		program->GetBindPointsForSampler(_samplerId, _bindPoints, _bindPointCount);
		_retrievedBindPoints = true;
	}

	// Retrieve the current sampler state
	ID3D11SamplerState* samplerState = _sampler->GetSamplerState();
	if (samplerState == nullptr)
	{
		return;
	}

	// Bind this resource to each defined bind point
	for (size_t i = 0; i < _bindPointCount; ++i)
	{
		const auto& bindPoint = _bindPoints[i];
		switch (bindPoint.stage)
		{
		case IShaderProgram::ShaderStage::Vertex:
			context->VSSetSamplers(bindPoint.bindPoint, 1, &samplerState);
			break;
		case IShaderProgram::ShaderStage::Fragment:
			context->PSSetSamplers(bindPoint.bindPoint, 1, &samplerState);
			break;
		case IShaderProgram::ShaderStage::Geometry:
			context->GSSetSamplers(bindPoint.bindPoint, 1, &samplerState);
			break;
		case IShaderProgram::ShaderStage::Compute:
			context->CSSetSamplers(bindPoint.bindPoint, 1, &samplerState);
			break;
		}
	}
}

//----------------------------------------------------------------------------------------
template<class T>
void SamplerBindingInfo<T>::UnbindSampler(ID3D11Device1* device, ID3D11DeviceContext1* context, Direct3DShaderProgram* program)
{
	ID3D11SamplerState* samplerState = nullptr;
	for (size_t i = 0; i < _bindPointCount; ++i)
	{
		const auto& bindPoint = _bindPoints[i];
		switch (bindPoint.stage)
		{
		case IShaderProgram::ShaderStage::Vertex:
			context->VSSetSamplers(bindPoint.bindPoint, 1, &samplerState);
			break;
		case IShaderProgram::ShaderStage::Fragment:
			context->PSSetSamplers(bindPoint.bindPoint, 1, &samplerState);
			break;
		case IShaderProgram::ShaderStage::Geometry:
			context->GSSetSamplers(bindPoint.bindPoint, 1, &samplerState);
			break;
		case IShaderProgram::ShaderStage::Compute:
			context->CSSetSamplers(bindPoint.bindPoint, 1, &samplerState);
			break;
		}
	}
}

} // namespace cobalt::graphics
