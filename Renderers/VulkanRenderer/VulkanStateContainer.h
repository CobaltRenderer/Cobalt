// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include "BindingHelpers.h"
#include "VulkanNode.h"
#include <Cobalt/LoggingInterface/LoggingInterface.pkg>
#include <Cobalt/RendererInterface/RendererInterface.pkg>
#include <vector>
namespace cobalt::graphics {

template<class T>
class VulkanStateContainer : public VulkanNode<T>
{
public:
	// Constructors
	explicit VulkanStateContainer(cobalt::logging::ILogger* log);
	~VulkanStateContainer();

	// Resource binding methods
	void BindTextureWithCombinedSampler(TextureId textureId, ITextureBuffer1D* texture, ITextureSampler1D* sampler) override;
	void BindTextureWithCombinedSampler(TextureId textureId, ITextureBuffer2D* texture, ITextureSampler2D* sampler) override;
	void BindTextureWithCombinedSampler(TextureId textureId, ITextureBuffer3D* texture, ITextureSampler3D* sampler) override;
	void BindTextureWithCombinedSampler(TextureId textureId, ITextureBufferCube* texture, ITextureSamplerCube* sampler) override;
	void BindTextureWithCombinedSampler(TextureId textureId, ITextureBuffer1DArray* texture, ITextureSampler1DArray* sampler) override;
	void BindTextureWithCombinedSampler(TextureId textureId, ITextureBuffer2DArray* texture, ITextureSampler2DArray* sampler) override;
	void BindTextureWithCombinedSampler(TextureId textureId, ITextureBufferCubeArray* texture, ITextureSamplerCubeArray* sampler) override;
	void BindTexture(TextureId textureId, ITextureBuffer1D* texture) override;
	void BindTexture(TextureId textureId, ITextureBuffer2D* texture) override;
	void BindTexture(TextureId textureId, ITextureBuffer3D* texture) override;
	void BindTexture(TextureId textureId, ITextureBufferCube* texture) override;
	void BindTexture(TextureId textureId, ITextureBuffer1DArray* texture) override;
	void BindTexture(TextureId textureId, ITextureBuffer2DArray* texture) override;
	void BindTexture(TextureId textureId, ITextureBufferCubeArray* texture) override;
	void UnbindTexture(TextureId textureId) override;
	void BindSampler(SamplerId samplerId, ITextureSampler1D* sampler) override;
	void BindSampler(SamplerId samplerId, ITextureSampler2D* sampler) override;
	void BindSampler(SamplerId samplerId, ITextureSampler3D* sampler) override;
	void BindSampler(SamplerId samplerId, ITextureSamplerCube* sampler) override;
	void BindSampler(SamplerId samplerId, ITextureSampler1DArray* sampler) override;
	void BindSampler(SamplerId samplerId, ITextureSampler2DArray* sampler) override;
	void BindSampler(SamplerId samplerId, ITextureSamplerCubeArray* sampler) override;
	void UnbindSampler(SamplerId samplerId) override;
	void BindStateBuffer(StateBufferId stateBufferId, IStateBuffer* stateBuffer, uint32_t stateBufferPageNo) override;
	void UnbindStateBuffer(StateBufferId stateBufferId) override;
	void BindResourceArray(ResourceArrayId resourceArrayId, IDataArray* resourceBuffer, bool resetCounter) override;
	void BindResourceArray(ResourceArrayId resourceArrayId, ITexelArray* resourceBuffer) override;
	void UnbindResourceArray(ResourceArrayId resourceArrayId) override;

	// State value methods
	void ResetStateValue(StateValueId stateId, const size_t* arrayIndices, size_t arrayIndexCount) override;

	// Build state methods
	void MigrateBuildStateToDrawState();
	const std::vector<IStateValueInfo*>& GetValueEntries() const;
	const std::vector<ITextureBindingInfo*>& GetTextureEntries() const;
	const std::vector<ISamplerBindingInfo*>& GetSamplerEntries() const;
	const std::vector<StateBufferBindingInfo*>& GetStateBufferEntries() const;
	const std::vector<ResourceArrayBindingInfo*>& GetResourceBufferEntries() const;
	void SetDescriptorSetIndex(size_t index, int stateIndex);
	size_t GetDescriptorSetIndex(int stateIndex);

protected:
	// State value methods
	void SetStateValueInternal(StateValueId stateId, bool value, const size_t* arrayIndices, size_t arrayIndexCount) override;
	void SetStateValueInternal(StateValueId stateId, const V1Int8& value, const size_t* arrayIndices, size_t arrayIndexCount) override;
	void SetStateValueInternal(StateValueId stateId, const V1Int16& value, const size_t* arrayIndices, size_t arrayIndexCount) override;
	void SetStateValueInternal(StateValueId stateId, const V1Int32& value, const size_t* arrayIndices, size_t arrayIndexCount) override;
	void SetStateValueInternal(StateValueId stateId, const V1UInt8& value, const size_t* arrayIndices, size_t arrayIndexCount) override;
	void SetStateValueInternal(StateValueId stateId, const V1UInt16& value, const size_t* arrayIndices, size_t arrayIndexCount) override;
	void SetStateValueInternal(StateValueId stateId, const V1UInt32& value, const size_t* arrayIndices, size_t arrayIndexCount) override;
	void SetStateValueInternal(StateValueId stateId, const V1Float32& value, const size_t* arrayIndices, size_t arrayIndexCount) override;
	void SetStateValueInternal(StateValueId stateId, const V1Float64& value, const size_t* arrayIndices, size_t arrayIndexCount) override;
	void SetStateValueInternal(StateValueId stateId, const V2Int8& value, const size_t* arrayIndices, size_t arrayIndexCount) override;
	void SetStateValueInternal(StateValueId stateId, const V2Int16& value, const size_t* arrayIndices, size_t arrayIndexCount) override;
	void SetStateValueInternal(StateValueId stateId, const V2Int32& value, const size_t* arrayIndices, size_t arrayIndexCount) override;
	void SetStateValueInternal(StateValueId stateId, const V2UInt8& value, const size_t* arrayIndices, size_t arrayIndexCount) override;
	void SetStateValueInternal(StateValueId stateId, const V2UInt16& value, const size_t* arrayIndices, size_t arrayIndexCount) override;
	void SetStateValueInternal(StateValueId stateId, const V2UInt32& value, const size_t* arrayIndices, size_t arrayIndexCount) override;
	void SetStateValueInternal(StateValueId stateId, const V2Float32& value, const size_t* arrayIndices, size_t arrayIndexCount) override;
	void SetStateValueInternal(StateValueId stateId, const V2Float64& value, const size_t* arrayIndices, size_t arrayIndexCount) override;
	void SetStateValueInternal(StateValueId stateId, const V3Int8& value, const size_t* arrayIndices, size_t arrayIndexCount) override;
	void SetStateValueInternal(StateValueId stateId, const V3Int16& value, const size_t* arrayIndices, size_t arrayIndexCount) override;
	void SetStateValueInternal(StateValueId stateId, const V3Int32& value, const size_t* arrayIndices, size_t arrayIndexCount) override;
	void SetStateValueInternal(StateValueId stateId, const V3UInt8& value, const size_t* arrayIndices, size_t arrayIndexCount) override;
	void SetStateValueInternal(StateValueId stateId, const V3UInt16& value, const size_t* arrayIndices, size_t arrayIndexCount) override;
	void SetStateValueInternal(StateValueId stateId, const V3UInt32& value, const size_t* arrayIndices, size_t arrayIndexCount) override;
	void SetStateValueInternal(StateValueId stateId, const V3Float32& value, const size_t* arrayIndices, size_t arrayIndexCount) override;
	void SetStateValueInternal(StateValueId stateId, const V3Float64& value, const size_t* arrayIndices, size_t arrayIndexCount) override;
	void SetStateValueInternal(StateValueId stateId, const V4Int8& value, const size_t* arrayIndices, size_t arrayIndexCount) override;
	void SetStateValueInternal(StateValueId stateId, const V4Int16& value, const size_t* arrayIndices, size_t arrayIndexCount) override;
	void SetStateValueInternal(StateValueId stateId, const V4Int32& value, const size_t* arrayIndices, size_t arrayIndexCount) override;
	void SetStateValueInternal(StateValueId stateId, const V4UInt8& value, const size_t* arrayIndices, size_t arrayIndexCount) override;
	void SetStateValueInternal(StateValueId stateId, const V4UInt16& value, const size_t* arrayIndices, size_t arrayIndexCount) override;
	void SetStateValueInternal(StateValueId stateId, const V4UInt32& value, const size_t* arrayIndices, size_t arrayIndexCount) override;
	void SetStateValueInternal(StateValueId stateId, const V4Float32& value, const size_t* arrayIndices, size_t arrayIndexCount) override;
	void SetStateValueInternal(StateValueId stateId, const V4Float64& value, const size_t* arrayIndices, size_t arrayIndexCount) override;
	void SetStateValueInternal(StateValueId stateId, const M2Float32& value, const size_t* arrayIndices, size_t arrayIndexCount) override;
	void SetStateValueInternal(StateValueId stateId, const M3Float32& value, const size_t* arrayIndices, size_t arrayIndexCount) override;
	void SetStateValueInternal(StateValueId stateId, const M4Float32& value, const size_t* arrayIndices, size_t arrayIndexCount) override;
	inline void SetStateValueInternal(StateValueId stateId, const size_t* arrayIndices, size_t arrayIndexCount, IStateValueInfo* value);

private:
	// Structures
	struct MutableState
	{
		std::vector<IStateValueInfo*> valueEntries;
		std::vector<ITextureBindingInfo*> textureEntries;
		std::vector<ISamplerBindingInfo*> samplerEntries;
		std::vector<StateBufferBindingInfo*> stateBufferEntries;
		std::vector<ResourceArrayBindingInfo*> resourceBufferEntries;
		std::vector<IStateValueInfo*> valueEntriesToDelete;
		std::vector<ITextureBindingInfo*> textureEntriesToDelete;
		std::vector<ISamplerBindingInfo*> samplerEntriesToDelete;
		std::vector<StateBufferBindingInfo*> stateBufferEntriesToDelete;
		std::vector<ResourceArrayBindingInfo*> resourceBufferEntriesToDelete;
	};

private:
	// Resource binding methods
	template<class TextureType, class SamplerType>
	void BindTextureWithCombinedSamplerInternal(TextureId textureId, TextureType* texture, SamplerType* sampler);
	template<class TextureType>
	void BindTextureInternal(TextureId textureId, TextureType* texture);
	template<class SamplerType>
	void BindSamplerInternal(SamplerId samplerId, SamplerType* sampler);

	// Build state methods
	void DeleteRemovedEntries(MutableState& targetState);

private:
	MutableState _drawState;
	MutableState _buildState;
	cobalt::logging::ILogger* _log;
	std::vector<size_t> _descriptorSetIndex;
	size_t _descriptorSetIndexSize = 0;
	size_t _standardDescriptorSetIndex = std::numeric_limits<size_t>::max();
};

} // namespace cobalt::graphics
#include "VulkanStateContainer.inl"
