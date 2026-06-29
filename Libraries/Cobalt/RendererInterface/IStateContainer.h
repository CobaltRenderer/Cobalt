// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include "MatrixTypes.h"
#include "Tokens.h"
#include "VectorTypes.h"
namespace cobalt { namespace graphics {
class ITextureBuffer1D;
class ITextureBuffer2D;
class ITextureBuffer3D;
class ITextureBufferCube;
class ITextureBuffer1DArray;
class ITextureBuffer2DArray;
class ITextureBufferCubeArray;
class ITextureSampler1D;
class ITextureSampler2D;
class ITextureSampler3D;
class ITextureSamplerCube;
class ITextureSampler1DArray;
class ITextureSampler2DArray;
class ITextureSamplerCubeArray;
class IDataArray;
class ITexelArray;
class IStateBuffer;

class IStateContainer
{
public:
	// Resource binding methods
	virtual void BindTextureWithCombinedSampler(TextureId textureId, ITextureBuffer1D* texture, ITextureSampler1D* sampler) = 0;
	virtual void BindTextureWithCombinedSampler(TextureId textureId, ITextureBuffer2D* texture, ITextureSampler2D* sampler) = 0;
	virtual void BindTextureWithCombinedSampler(TextureId textureId, ITextureBuffer3D* texture, ITextureSampler3D* sampler) = 0;
	virtual void BindTextureWithCombinedSampler(TextureId textureId, ITextureBufferCube* texture, ITextureSamplerCube* sampler) = 0;
	virtual void BindTextureWithCombinedSampler(TextureId textureId, ITextureBuffer1DArray* texture, ITextureSampler1DArray* sampler) = 0;
	virtual void BindTextureWithCombinedSampler(TextureId textureId, ITextureBuffer2DArray* texture, ITextureSampler2DArray* sampler) = 0;
	virtual void BindTextureWithCombinedSampler(TextureId textureId, ITextureBufferCubeArray* texture, ITextureSamplerCubeArray* sampler) = 0;
	virtual void BindTexture(TextureId textureId, ITextureBuffer1D* texture) = 0;
	virtual void BindTexture(TextureId textureId, ITextureBuffer2D* texture) = 0;
	virtual void BindTexture(TextureId textureId, ITextureBuffer3D* texture) = 0;
	virtual void BindTexture(TextureId textureId, ITextureBufferCube* texture) = 0;
	virtual void BindTexture(TextureId textureId, ITextureBuffer1DArray* texture) = 0;
	virtual void BindTexture(TextureId textureId, ITextureBuffer2DArray* texture) = 0;
	virtual void BindTexture(TextureId textureId, ITextureBufferCubeArray* texture) = 0;
	virtual void UnbindTexture(TextureId textureId) = 0;
	virtual void BindSampler(SamplerId samplerId, ITextureSampler1D* sampler) = 0;
	virtual void BindSampler(SamplerId samplerId, ITextureSampler2D* sampler) = 0;
	virtual void BindSampler(SamplerId samplerId, ITextureSampler3D* sampler) = 0;
	virtual void BindSampler(SamplerId samplerId, ITextureSamplerCube* sampler) = 0;
	virtual void BindSampler(SamplerId samplerId, ITextureSampler1DArray* sampler) = 0;
	virtual void BindSampler(SamplerId samplerId, ITextureSampler2DArray* sampler) = 0;
	virtual void BindSampler(SamplerId samplerId, ITextureSamplerCubeArray* sampler) = 0;
	virtual void UnbindSampler(SamplerId samplerId) = 0;
	virtual void BindStateBuffer(StateBufferId stateBufferId, IStateBuffer* stateBuffer, uint32_t stateBufferPageNo = 0) = 0;
	virtual void UnbindStateBuffer(StateBufferId stateBufferId) = 0;
	virtual void BindResourceArray(ResourceArrayId resourceArrayId, IDataArray* resourceBuffer, bool resetCounter = true) = 0;
	virtual void BindResourceArray(ResourceArrayId resourceArrayId, ITexelArray* resourceBuffer) = 0;
	virtual void UnbindResourceArray(ResourceArrayId resourceArrayId) = 0;

	// State value methods
	template<class T>
	inline void SetStateValue(StateValueId stateId, const T& value);
	template<class T, class... IndexTs>
	inline void SetStateValue(StateValueId stateId, const T& value, size_t firstArrayIndex, IndexTs... additionalArrayIndices);
	template<class T>
	inline void SetStateValue(StateValueId stateId, const T& value, const size_t* arrayIndices, size_t arrayIndexCount);
	inline void ResetStateValue(StateValueId stateId);
	template<class... IndexTs>
	inline void ResetStateValue(StateValueId stateId, size_t firstArrayIndex, IndexTs... additionalArrayIndices);
	virtual void ResetStateValue(StateValueId stateId, const size_t* arrayIndices, size_t arrayIndexCount) = 0;

protected:
	// State value methods
	virtual void SetStateValueInternal(StateValueId stateId, bool value, const size_t* arrayIndices, size_t arrayIndexCount) = 0;
	virtual void SetStateValueInternal(StateValueId stateId, const V1Int8& value, const size_t* arrayIndices, size_t arrayIndexCount) = 0;
	virtual void SetStateValueInternal(StateValueId stateId, const V1Int16& value, const size_t* arrayIndices, size_t arrayIndexCount) = 0;
	virtual void SetStateValueInternal(StateValueId stateId, const V1Int32& value, const size_t* arrayIndices, size_t arrayIndexCount) = 0;
	virtual void SetStateValueInternal(StateValueId stateId, const V1UInt8& value, const size_t* arrayIndices, size_t arrayIndexCount) = 0;
	virtual void SetStateValueInternal(StateValueId stateId, const V1UInt16& value, const size_t* arrayIndices, size_t arrayIndexCount) = 0;
	virtual void SetStateValueInternal(StateValueId stateId, const V1UInt32& value, const size_t* arrayIndices, size_t arrayIndexCount) = 0;
	virtual void SetStateValueInternal(StateValueId stateId, const V1Float32& value, const size_t* arrayIndices, size_t arrayIndexCount) = 0;
	virtual void SetStateValueInternal(StateValueId stateId, const V1Float64& value, const size_t* arrayIndices, size_t arrayIndexCount) = 0;
	virtual void SetStateValueInternal(StateValueId stateId, const V2Int8& value, const size_t* arrayIndices, size_t arrayIndexCount) = 0;
	virtual void SetStateValueInternal(StateValueId stateId, const V2Int16& value, const size_t* arrayIndices, size_t arrayIndexCount) = 0;
	virtual void SetStateValueInternal(StateValueId stateId, const V2Int32& value, const size_t* arrayIndices, size_t arrayIndexCount) = 0;
	virtual void SetStateValueInternal(StateValueId stateId, const V2UInt8& value, const size_t* arrayIndices, size_t arrayIndexCount) = 0;
	virtual void SetStateValueInternal(StateValueId stateId, const V2UInt16& value, const size_t* arrayIndices, size_t arrayIndexCount) = 0;
	virtual void SetStateValueInternal(StateValueId stateId, const V2UInt32& value, const size_t* arrayIndices, size_t arrayIndexCount) = 0;
	virtual void SetStateValueInternal(StateValueId stateId, const V2Float32& value, const size_t* arrayIndices, size_t arrayIndexCount) = 0;
	virtual void SetStateValueInternal(StateValueId stateId, const V2Float64& value, const size_t* arrayIndices, size_t arrayIndexCount) = 0;
	virtual void SetStateValueInternal(StateValueId stateId, const V3Int8& value, const size_t* arrayIndices, size_t arrayIndexCount) = 0;
	virtual void SetStateValueInternal(StateValueId stateId, const V3Int16& value, const size_t* arrayIndices, size_t arrayIndexCount) = 0;
	virtual void SetStateValueInternal(StateValueId stateId, const V3Int32& value, const size_t* arrayIndices, size_t arrayIndexCount) = 0;
	virtual void SetStateValueInternal(StateValueId stateId, const V3UInt8& value, const size_t* arrayIndices, size_t arrayIndexCount) = 0;
	virtual void SetStateValueInternal(StateValueId stateId, const V3UInt16& value, const size_t* arrayIndices, size_t arrayIndexCount) = 0;
	virtual void SetStateValueInternal(StateValueId stateId, const V3UInt32& value, const size_t* arrayIndices, size_t arrayIndexCount) = 0;
	virtual void SetStateValueInternal(StateValueId stateId, const V3Float32& value, const size_t* arrayIndices, size_t arrayIndexCount) = 0;
	virtual void SetStateValueInternal(StateValueId stateId, const V3Float64& value, const size_t* arrayIndices, size_t arrayIndexCount) = 0;
	virtual void SetStateValueInternal(StateValueId stateId, const V4Int8& value, const size_t* arrayIndices, size_t arrayIndexCount) = 0;
	virtual void SetStateValueInternal(StateValueId stateId, const V4Int16& value, const size_t* arrayIndices, size_t arrayIndexCount) = 0;
	virtual void SetStateValueInternal(StateValueId stateId, const V4Int32& value, const size_t* arrayIndices, size_t arrayIndexCount) = 0;
	virtual void SetStateValueInternal(StateValueId stateId, const V4UInt8& value, const size_t* arrayIndices, size_t arrayIndexCount) = 0;
	virtual void SetStateValueInternal(StateValueId stateId, const V4UInt16& value, const size_t* arrayIndices, size_t arrayIndexCount) = 0;
	virtual void SetStateValueInternal(StateValueId stateId, const V4UInt32& value, const size_t* arrayIndices, size_t arrayIndexCount) = 0;
	virtual void SetStateValueInternal(StateValueId stateId, const V4Float32& value, const size_t* arrayIndices, size_t arrayIndexCount) = 0;
	virtual void SetStateValueInternal(StateValueId stateId, const V4Float64& value, const size_t* arrayIndices, size_t arrayIndexCount) = 0;
	virtual void SetStateValueInternal(StateValueId stateId, const M2Float32& value, const size_t* arrayIndices, size_t arrayIndexCount) = 0;
	virtual void SetStateValueInternal(StateValueId stateId, const M3Float32& value, const size_t* arrayIndices, size_t arrayIndexCount) = 0;
	virtual void SetStateValueInternal(StateValueId stateId, const M4Float32& value, const size_t* arrayIndices, size_t arrayIndexCount) = 0;

	// Constructors
	~IStateContainer() = default;
};

}} // namespace cobalt::graphics
#include "IStateContainer.inl"
