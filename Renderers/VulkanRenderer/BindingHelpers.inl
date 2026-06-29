// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "VulkanRenderer.h"
#include "VulkanShaderProgram.h"
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
void ConstantStateValueInfo<ValueType>::ApplyValue(VulkanShaderProgram* program) const
{
	program->SetConstantValue(_stateId, reinterpret_cast<const uint8_t*>(_value.data()), sizeof(_value), _arrayIndices.data(), _arrayIndices.size());
}

//----------------------------------------------------------------------------------------
template<>
inline void ConstantStateValueInfo<bool>::ApplyValue(VulkanShaderProgram* program) const
{
	int val = (int)_value;
	program->SetConstantValue(_stateId, reinterpret_cast<const uint8_t*>(&val), sizeof(val), _arrayIndices.data(), _arrayIndices.size());
}

//----------------------------------------------------------------------------------------
template<>
inline void ConstantStateValueInfo<M2Float32>::ApplyValue(VulkanShaderProgram* program) const
{
	program->SetConstantValue(_stateId, reinterpret_cast<const uint8_t*>(_value.data()), sizeof(_value), _arrayIndices.data(), _arrayIndices.size());
}

//----------------------------------------------------------------------------------------
template<>
inline void ConstantStateValueInfo<M3Float32>::ApplyValue(VulkanShaderProgram* program) const
{
	program->SetConstantValue(_stateId, reinterpret_cast<const uint8_t*>(_value.data()), sizeof(_value), _arrayIndices.data(), _arrayIndices.size());
}

//----------------------------------------------------------------------------------------
template<>
inline void ConstantStateValueInfo<M4Float32>::ApplyValue(VulkanShaderProgram* program) const
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
void StateValueInfo<ValueType>::ApplyValue(VulkanShaderProgram* program) const
{
	program->UpdateStateValue(_stateId, reinterpret_cast<const uint8_t*>(_value.data()), sizeof(_value), _arrayIndices.data(), _arrayIndices.size());
}

//----------------------------------------------------------------------------------------
template<>
inline void StateValueInfo<bool>::ApplyValue(VulkanShaderProgram* program) const
{
	int val = (int)_value;
	program->UpdateStateValue(_stateId, reinterpret_cast<const uint8_t*>(&val), sizeof(val), _arrayIndices.data(), _arrayIndices.size());
}

//----------------------------------------------------------------------------------------
template<>
inline void StateValueInfo<M2Float32>::ApplyValue(VulkanShaderProgram* program) const
{
	program->UpdateStateValue(_stateId, reinterpret_cast<const uint8_t*>(_value.data()), sizeof(_value), _arrayIndices.data(), _arrayIndices.size());
}

//----------------------------------------------------------------------------------------
template<>
inline void StateValueInfo<M3Float32>::ApplyValue(VulkanShaderProgram* program) const
{
	program->UpdateStateValue(_stateId, reinterpret_cast<const uint8_t*>(_value.data()), sizeof(_value), _arrayIndices.data(), _arrayIndices.size());
}

//----------------------------------------------------------------------------------------
template<>
inline void StateValueInfo<M4Float32>::ApplyValue(VulkanShaderProgram* program) const
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
void TextureBindingInfo<T>::BindTexture(const VulkanRenderer* renderer, VulkanShaderProgram* program, size_t setIndex)
{
	// Obtain the bind points for this resource
	if (!_retrievedBindPoints)
	{
		// Retrieve the bind points for the target resource
		program->GetBindPointsForTexture(_textureId, _bindPoints, _bindPointCount);

		// Populate the image layout
		_imageInfo.imageLayout = _texture->GetDefaultImageLayout();
		_imageInfo.imageView = _texture->GetImageView();

		// Populate a default descriptor set write
		VkWriteDescriptorSet writeDescriptorSetDefault = {};
		writeDescriptorSetDefault.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writeDescriptorSetDefault.pNext = nullptr;
		writeDescriptorSetDefault.dstBinding = 0xFFFFFFFF;
		writeDescriptorSetDefault.dstArrayElement = 0;
		writeDescriptorSetDefault.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
		writeDescriptorSetDefault.descriptorCount = 1;
		writeDescriptorSetDefault.pBufferInfo = nullptr;
		writeDescriptorSetDefault.pImageInfo = &_imageInfo;
		writeDescriptorSetDefault.pTexelBufferView = nullptr;

		// Prepare the descriptor set writes for the target resource
		_descriptorWrites.reserve(_bindPointCount);
		for (size_t i = 0; i < _bindPointCount; ++i)
		{
			if (_bindPoints[i].descriptorType != VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE)
			{
				continue;
			}

			auto& descriptorWrite = _descriptorWrites.emplace_back(writeDescriptorSetDefault);
			descriptorWrite.dstBinding = _bindPoints[i].bindPoint;
			descriptorWrite.descriptorType = _bindPoints[i].descriptorType;
		}

		// Flag that we've now retrieved the bind points for this resource
		_retrievedBindPoints = true;
	}

	// Bind the target resource to the descriptor set
	auto descriptorSet = program->GetDescriptorSet(setIndex);
	for (auto& descriptorWrite : _descriptorWrites)
	{
		descriptorWrite.dstSet = descriptorSet;
	}
	vkUpdateDescriptorSets(renderer->GetDevice(), (uint32_t)_descriptorWrites.size(), _descriptorWrites.data(), 0, nullptr);
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
void TextureBindingWithCombinedSamplerInfo<TextureType, SamplerType>::BindTexture(const VulkanRenderer* renderer, VulkanShaderProgram* program, size_t setIndex)
{
	// Obtain the bind points for this resource
	if (!_retrievedBindPoints)
	{
		// Retrieve the bind points for the target resource
		program->GetBindPointsForTexture(_textureId, _bindPoints, _bindPointCount);

		// Populate the image view and info
		_imageInfo.imageLayout = _texture->GetDefaultImageLayout();
		_imageInfo.imageView = _texture->GetImageView();
		_imageInfo.sampler = _sampler->GetNativeSampler();

		// Populate a default descriptor set write
		VkWriteDescriptorSet writeDescriptorSetDefault = {};
		writeDescriptorSetDefault.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writeDescriptorSetDefault.pNext = nullptr;
		writeDescriptorSetDefault.dstBinding = 0xFFFFFFFF;
		writeDescriptorSetDefault.dstArrayElement = 0;
		writeDescriptorSetDefault.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		writeDescriptorSetDefault.descriptorCount = 1;
		writeDescriptorSetDefault.pBufferInfo = nullptr;
		writeDescriptorSetDefault.pImageInfo = &_imageInfo;
		writeDescriptorSetDefault.pTexelBufferView = nullptr;

		// Prepare the descriptor set writes for the target resource
		_descriptorWrites.reserve(_bindPointCount);
		for (size_t i = 0; i < _bindPointCount; ++i)
		{
			auto& descriptorWrite = _descriptorWrites.emplace_back(writeDescriptorSetDefault);
			descriptorWrite.dstBinding = _bindPoints[i].bindPoint;
			descriptorWrite.descriptorType = _bindPoints[i].descriptorType;
		}

		// Flag that we've now retrieved the bind points for this resource
		_retrievedBindPoints = true;
	}
	else
	{
		// Update the sampler in case it's changed
		_imageInfo.sampler = _sampler->GetNativeSampler();
	}

	// Bind the target resource to the descriptor set
	auto descriptorSet = program->GetDescriptorSet(setIndex);
	for (auto& descriptorWrite : _descriptorWrites)
	{
		descriptorWrite.dstSet = descriptorSet;
	}
	vkUpdateDescriptorSets(renderer->GetDevice(), (uint32_t)_descriptorWrites.size(), _descriptorWrites.data(), 0, nullptr);
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
void SamplerBindingInfo<T>::BindSampler(const VulkanRenderer* renderer, VulkanShaderProgram* program, size_t setIndex)
{
	// Retrieve the binding point if required
	if (!_retrievedBindPoints)
	{
		// Retrieve the bind point for the target resource
		program->GetBindPointsForSampler(_samplerId, _bindPoints, _bindPointCount);

		// Populate the image info
		_imageInfo.sampler = _sampler->GetNativeSampler();

		// Populate a default descriptor set write
		VkWriteDescriptorSet writeDescriptorSetDefault = {};
		writeDescriptorSetDefault.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writeDescriptorSetDefault.pNext = nullptr;
		writeDescriptorSetDefault.dstBinding = 0xFFFFFFFF;
		writeDescriptorSetDefault.dstArrayElement = 0;
		writeDescriptorSetDefault.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
		writeDescriptorSetDefault.descriptorCount = 1;
		writeDescriptorSetDefault.pBufferInfo = nullptr;
		writeDescriptorSetDefault.pImageInfo = &_imageInfo;
		writeDescriptorSetDefault.pTexelBufferView = nullptr;

		// Prepare the descriptor set writes for the target resource
		_descriptorWrites.resize(_bindPointCount, writeDescriptorSetDefault);
		for (size_t i = 0; i < _bindPointCount; ++i)
		{
			_descriptorWrites[i].dstBinding = _bindPoints[i].bindPoint;
		}

		// Flag that we've now retrieved the bind points for this resource
		_retrievedBindPoints = true;
	}
	else
	{
		// Update the sampler in case it's changed
		_imageInfo.sampler = _sampler->GetNativeSampler();
	}

	// Bind the target resource to the descriptor set
	auto descriptorSet = program->GetDescriptorSet(setIndex);
	for (auto& descriptorWrite : _descriptorWrites)
	{
		descriptorWrite.dstSet = descriptorSet;
	}
	vkUpdateDescriptorSets(renderer->GetDevice(), (uint32_t)_descriptorWrites.size(), _descriptorWrites.data(), 0, nullptr);
}

} // namespace cobalt::graphics
