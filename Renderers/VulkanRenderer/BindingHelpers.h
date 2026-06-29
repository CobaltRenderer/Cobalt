// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include "VulkanHeaders.h"
#include "VulkanShaderProgram.h"
#include <Cobalt/RendererInterface/RendererInterface.pkg>
#include <vector>
namespace cobalt::graphics {
class VulkanRenderer;
class VulkanDataArray;
class VulkanTexelArray;
class VulkanStateBuffer;

//----------------------------------------------------------------------------------------
class IConstantStateValueInfo
{
public:
	inline virtual ~IConstantStateValueInfo() = 0;
	virtual StateValueId GetAttributeId() const = 0;
	virtual size_t GetArrayIndexCount() const = 0;
	virtual const size_t* GetArrayIndices() const = 0;
	virtual void ApplyValue(VulkanShaderProgram* program) const = 0;
};

//----------------------------------------------------------------------------------------
class IStateValueInfo
{
public:
	inline virtual ~IStateValueInfo() = 0;
	virtual StateValueId GetAttributeId() const = 0;
	virtual size_t GetArrayIndexCount() const = 0;
	virtual const size_t* GetArrayIndices() const = 0;
	virtual void ApplyValue(VulkanShaderProgram* program) const = 0;
};

//----------------------------------------------------------------------------------------
class ITextureBindingInfo
{
public:
	inline virtual ~ITextureBindingInfo() = 0;
	virtual TextureId GetTextureId() const = 0;
	virtual void BindTexture(const VulkanRenderer* renderer, VulkanShaderProgram* program, size_t setIndex) = 0;
};

//----------------------------------------------------------------------------------------
class ISamplerBindingInfo
{
public:
	inline virtual ~ISamplerBindingInfo() = 0;
	virtual SamplerId GetSamplerId() const = 0;
	virtual void BindSampler(const VulkanRenderer* renderer, VulkanShaderProgram* program, size_t setIndex) = 0;
};

//----------------------------------------------------------------------------------------
class StateBufferBindingInfo
{
public:
	StateBufferBindingInfo(StateBufferId stateBufferId, uint32_t stateBufferPageNo, IStateBuffer* stateBuffer);
	StateBufferId GetStateBufferId() const;
	void BindStateBuffer(const VulkanRenderer* renderer, VulkanShaderProgram* program, size_t setIndex);

private:
	std::vector<VkWriteDescriptorSet> _descriptorWrites;
	VkDescriptorBufferInfo _bufferInfo = {};
	VulkanStateBuffer* _stateBuffer;
	const VulkanShaderProgram::StateBufferBindPointEntry* _bindPoints = nullptr;
	size_t _bindPointCount = 0;
	StateBufferId _stateBufferId;
	uint32_t _stateBufferPageNo;
	bool _retrievedBindPoints = false;
};

//----------------------------------------------------------------------------------------
class ResourceArrayBindingInfo
{
public:
	ResourceArrayBindingInfo(ResourceArrayId resourceArrayId, IDataArray* resourceBuffer, bool resetCounter);
	ResourceArrayBindingInfo(ResourceArrayId resourceArrayId, ITexelArray* resourceBuffer);
	ResourceArrayId GetResourceArrayId() const;
	void BindResourceArray(const VulkanRenderer* renderer, VulkanShaderProgram* program, size_t setIndex, VkCommandBuffer commandBuffer, bool performReset);
	bool HasCaptureTargets() const;

private:
	std::vector<VkWriteDescriptorSet> _descriptorWrites;
	VkDescriptorBufferInfo _bufferInfo = {};
	VkDescriptorBufferInfo _counterBufferInfo = {};
	VkBufferView _bufferView = VK_NULL_HANDLE;
	VulkanDataArray* _dataArray = nullptr;
	VulkanTexelArray* _texelArray = nullptr;
	const VulkanShaderProgram::ResourceArrayBindPointEntry* _bindPoints = nullptr;
	size_t _bindPointCount = 0;
	ResourceArrayId _resourceArrayId;
	bool _isDataArrayBinding;
	bool _resetCounter = false;
	bool _retrievedBindPoints = false;
};

//----------------------------------------------------------------------------------------
template<class ValueType>
class ConstantStateValueInfo : public IConstantStateValueInfo
{
public:
	inline ConstantStateValueInfo(StateValueId stateId, const ValueType& value, const size_t* arrayIndices, size_t arrayIndexCount);
	inline StateValueId GetAttributeId() const override;
	inline size_t GetArrayIndexCount() const override;
	inline const size_t* GetArrayIndices() const override;
	inline void ApplyValue(VulkanShaderProgram* program) const override;

private:
	StateValueId _stateId;
	std::vector<size_t> _arrayIndices;
	ValueType _value;
};

//----------------------------------------------------------------------------------------
template<class ValueType>
class StateValueInfo : public IStateValueInfo
{
public:
	inline StateValueInfo(StateValueId stateId, const ValueType& value, const size_t* arrayIndices, size_t arrayIndexCount);
	inline StateValueId GetAttributeId() const override;
	inline size_t GetArrayIndexCount() const override;
	inline const size_t* GetArrayIndices() const override;
	inline void ApplyValue(VulkanShaderProgram* program) const override;

private:
	StateValueId _stateId;
	std::vector<size_t> _arrayIndices;
	ValueType _value;
};

//----------------------------------------------------------------------------------------
template<class T>
class TextureBindingInfo : public ITextureBindingInfo
{
public:
	inline TextureBindingInfo(TextureId textureId, T* texture);
	inline TextureId GetTextureId() const override;
	inline void BindTexture(const VulkanRenderer* renderer, VulkanShaderProgram* program, size_t setIndex) override;

private:
	std::vector<VkWriteDescriptorSet> _descriptorWrites;
	VkDescriptorImageInfo _imageInfo = {};
	T* _texture;
	const VulkanShaderProgram::TextureBindPointEntry* _bindPoints = nullptr;
	size_t _bindPointCount = 0;
	TextureId _textureId;
	bool _retrievedBindPoints = false;
};

//----------------------------------------------------------------------------------------
template<class TextureType, class SamplerType>
class TextureBindingWithCombinedSamplerInfo : public ITextureBindingInfo
{
public:
	inline TextureBindingWithCombinedSamplerInfo(TextureId textureId, TextureType* texture, SamplerType* sampler);
	inline TextureId GetTextureId() const override;
	inline void BindTexture(const VulkanRenderer* renderer, VulkanShaderProgram* program, size_t setIndex) override;

private:
	std::vector<VkWriteDescriptorSet> _descriptorWrites;
	VkDescriptorImageInfo _imageInfo = {};
	TextureType* _texture;
	SamplerType* _sampler;
	const VulkanShaderProgram::TextureBindPointEntry* _bindPoints = nullptr;
	size_t _bindPointCount = 0;
	TextureId _textureId;
	bool _retrievedBindPoints = false;
};

//----------------------------------------------------------------------------------------
template<class T>
class SamplerBindingInfo : public ISamplerBindingInfo
{
public:
	inline SamplerBindingInfo(SamplerId samplerId, T* sampler);
	inline SamplerId GetSamplerId() const override;
	inline void BindSampler(const VulkanRenderer* renderer, VulkanShaderProgram* program, size_t setIndex) override;

private:
	std::vector<VkWriteDescriptorSet> _descriptorWrites;
	VkDescriptorImageInfo _imageInfo = {};
	T* _sampler;
	const VulkanShaderProgram::SamplerBindPointEntry* _bindPoints = nullptr;
	size_t _bindPointCount = 0;
	SamplerId _samplerId;
	bool _retrievedBindPoints = false;
};

} // namespace cobalt::graphics
#include "BindingHelpers.inl"
