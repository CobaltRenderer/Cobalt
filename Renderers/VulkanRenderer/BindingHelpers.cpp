// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "BindingHelpers.h"
#include "VulkanDataArray.h"
#include "VulkanRenderer.h"
#include "VulkanStateBuffer.h"
#include "VulkanTexelArray.h"
#include <Internal/RendererSupport/KnownDynamicCast.h>
namespace cobalt::graphics {

//----------------------------------------------------------------------------------------
// StateBufferBindingInfo methods
//----------------------------------------------------------------------------------------
StateBufferBindingInfo::StateBufferBindingInfo(StateBufferId stateBufferId, uint32_t stateBufferPageNo, IStateBuffer* stateBuffer)
: _stateBufferId(stateBufferId), _stateBufferPageNo(stateBufferPageNo), _stateBuffer(KnownDynamicCast<VulkanStateBuffer*>(stateBuffer))
{}

//----------------------------------------------------------------------------------------
StateBufferId StateBufferBindingInfo::GetStateBufferId() const
{
	return _stateBufferId;
}

//----------------------------------------------------------------------------------------
void StateBufferBindingInfo::BindStateBuffer(const VulkanRenderer* renderer, VulkanShaderProgram* program, size_t setIndex)
{
	// Obtain the bind points for this resource
	if (!_retrievedBindPoints)
	{
		// Retrieve the bind points for the target resource
		program->GetBindPointsForStateBuffer(_stateBufferId, _bindPoints, _bindPointCount);

		// Populate the buffer info
		_bufferInfo.buffer = _stateBuffer->GetNativeBuffer(_stateBufferPageNo);
		_bufferInfo.offset = _stateBuffer->GetPageOffset(_stateBufferPageNo);
		_bufferInfo.range = _stateBuffer->GetPageSize();

		// Populate a default descriptor set write
		VkWriteDescriptorSet writeDescriptorSetDefault = {};
		writeDescriptorSetDefault.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writeDescriptorSetDefault.pNext = nullptr;
		writeDescriptorSetDefault.dstBinding = 0xFFFFFFFF;
		writeDescriptorSetDefault.dstArrayElement = 0;
		writeDescriptorSetDefault.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		writeDescriptorSetDefault.descriptorCount = 1;
		writeDescriptorSetDefault.pBufferInfo = &_bufferInfo;
		writeDescriptorSetDefault.pImageInfo = nullptr;
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

	// Bind the target resource to the descriptor set
	auto descriptorSet = program->GetDescriptorSet(setIndex);
	for (auto& descriptorWrite : _descriptorWrites)
	{
		descriptorWrite.dstSet = descriptorSet;
	}
	vkUpdateDescriptorSets(renderer->GetDevice(), (uint32_t)_descriptorWrites.size(), _descriptorWrites.data(), 0, nullptr);
}

//----------------------------------------------------------------------------------------
// ResourceArrayBindingInfo methods
//----------------------------------------------------------------------------------------
ResourceArrayBindingInfo::ResourceArrayBindingInfo(ResourceArrayId resourceArrayId, IDataArray* resourceBuffer, bool resetCounter)
: _resourceArrayId(resourceArrayId), _dataArray(KnownDynamicCast<VulkanDataArray*>(resourceBuffer)), _resetCounter(resetCounter), _isDataArrayBinding(true)
{}

//----------------------------------------------------------------------------------------
ResourceArrayBindingInfo::ResourceArrayBindingInfo(ResourceArrayId resourceArrayId, ITexelArray* resourceBuffer)
: _resourceArrayId(resourceArrayId), _texelArray(KnownDynamicCast<VulkanTexelArray*>(resourceBuffer)), _isDataArrayBinding(false)
{}

//----------------------------------------------------------------------------------------
ResourceArrayId ResourceArrayBindingInfo::GetResourceArrayId() const
{
	return _resourceArrayId;
}

//----------------------------------------------------------------------------------------
void ResourceArrayBindingInfo::BindResourceArray(const VulkanRenderer* renderer, VulkanShaderProgram* program, size_t setIndex, VkCommandBuffer commandBuffer, bool performReset)
{
	// Retrieve the binding point if required
	bool hasCounter = (_isDataArrayBinding && _dataArray->HasCounter());
	if (!_retrievedBindPoints)
	{
		// Retrieve the bind points for the target resource
		program->GetBindPointsForResourceArray(_resourceArrayId, _bindPoints, _bindPointCount);

		// Populate the buffer view and info
		_bufferView = (!_isDataArrayBinding ? _texelArray->GetBufferView() : VK_NULL_HANDLE);
		_bufferInfo.buffer = (_isDataArrayBinding ? _dataArray->GetNativeBuffer() : _texelArray->GetNativeBuffer());
		_bufferInfo.offset = 0;
		_bufferInfo.range = (_isDataArrayBinding ? _dataArray->GetBufferSizeInBytes() : _texelArray->GetBufferSizeInBytes());
		if (hasCounter)
		{
			_counterBufferInfo.buffer = _dataArray->GetNativeCounterBuffer();
			_counterBufferInfo.offset = 0;
			_counterBufferInfo.range = 4;
		}

		// Populate a default descriptor set write
		VkWriteDescriptorSet writeDescriptorSetDefault = {};
		writeDescriptorSetDefault.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writeDescriptorSetDefault.pNext = nullptr;
		writeDescriptorSetDefault.dstBinding = 0xFFFFFFFF;
		writeDescriptorSetDefault.dstArrayElement = 0;
		writeDescriptorSetDefault.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		writeDescriptorSetDefault.descriptorCount = 1;
		writeDescriptorSetDefault.pBufferInfo = (_isDataArrayBinding ? &_bufferInfo : nullptr);
		writeDescriptorSetDefault.pImageInfo = nullptr;
		writeDescriptorSetDefault.pTexelBufferView = (!_isDataArrayBinding ? &_bufferView : nullptr);

		// Prepare the descriptor set writes for the target resource
		_descriptorWrites.resize(_bindPointCount * (hasCounter ? 2 : 1), writeDescriptorSetDefault);
		size_t descriptorWriteIndex = 0;
		for (size_t i = 0; i < _bindPointCount; ++i)
		{
			// Retrieve the next bind point
			const auto& bindPoint = _bindPoints[i];

			auto& descriptorWrite = _descriptorWrites[descriptorWriteIndex++];
			descriptorWrite.dstBinding = bindPoint.bindPoint;
			descriptorWrite.descriptorType = (_isDataArrayBinding ? VK_DESCRIPTOR_TYPE_STORAGE_BUFFER : (bindPoint.writeable ? VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER : VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER));

			if (hasCounter && (bindPoint.counterBindPoint != 0xFFFFFFFF))
			{
				auto& counterDescriptorWrite = _descriptorWrites[descriptorWriteIndex++];
				counterDescriptorWrite.dstBinding = bindPoint.counterBindPoint;
				counterDescriptorWrite.descriptorType = (_isDataArrayBinding ? VK_DESCRIPTOR_TYPE_STORAGE_BUFFER : (bindPoint.writeable ? VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER : VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER));
				counterDescriptorWrite.pBufferInfo = &_counterBufferInfo;
				counterDescriptorWrite.pTexelBufferView = nullptr;
			}
		}
		_descriptorWrites.resize(descriptorWriteIndex);

		// Flag that we've now retrieved the bind points for this resource
		_retrievedBindPoints = true;
	}

	// Clear the counter if required
	if (performReset && _resetCounter && hasCounter)
	{
		_dataArray->ResetCounter(commandBuffer);
	}

	// Bind the target resource to the descriptor set
	auto descriptorSet = program->GetDescriptorSet(setIndex);
	for (auto& descriptorWrite : _descriptorWrites)
	{
		descriptorWrite.dstSet = descriptorSet;
	}
	vkUpdateDescriptorSets(renderer->GetDevice(), (uint32_t)_descriptorWrites.size(), _descriptorWrites.data(), 0, nullptr);

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
bool ResourceArrayBindingInfo::HasCaptureTargets() const
{
	return (_isDataArrayBinding ? _dataArray->HasCaptureTargets() : _texelArray->HasCaptureTargets());
}

} // namespace cobalt::graphics
