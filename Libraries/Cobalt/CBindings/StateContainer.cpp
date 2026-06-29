// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "StateContainer.h"
#include <Cobalt/RendererInterface/RendererInterface.pkg>

using namespace cobalt::graphics;

//----------------------------------------------------------------------------------------
// Resource binding methods
//----------------------------------------------------------------------------------------
void Cobalt_StateContainer_BindTextureWithCombinedSampler1D(Cobalt_StateContainer container, uint32_t textureId, Cobalt_TextureBuffer1D texture, Cobalt_TextureSampler1D sampler)
{
	auto _this = reinterpret_cast<IStateContainer*>(container);

	_this->BindTextureWithCombinedSampler((TextureId)textureId, reinterpret_cast<ITextureBuffer1D*>(texture), reinterpret_cast<ITextureSampler1D*>(sampler));
}

//----------------------------------------------------------------------------------------
void Cobalt_StateContainer_BindTextureWithCombinedSampler2D(Cobalt_StateContainer container, uint32_t textureId, Cobalt_TextureBuffer2D texture, Cobalt_TextureSampler2D sampler)
{
	auto _this = reinterpret_cast<IStateContainer*>(container);

	_this->BindTextureWithCombinedSampler((TextureId)textureId, reinterpret_cast<ITextureBuffer2D*>(texture), reinterpret_cast<ITextureSampler2D*>(sampler));
}

//----------------------------------------------------------------------------------------
void Cobalt_StateContainer_BindTextureWithCombinedSampler3D(Cobalt_StateContainer container, uint32_t textureId, Cobalt_TextureBuffer3D texture, Cobalt_TextureSampler3D sampler)
{
	auto _this = reinterpret_cast<IStateContainer*>(container);

	_this->BindTextureWithCombinedSampler((TextureId)textureId, reinterpret_cast<ITextureBuffer3D*>(texture), reinterpret_cast<ITextureSampler3D*>(sampler));
}

//----------------------------------------------------------------------------------------
void Cobalt_StateContainer_BindTextureWithCombinedSamplerCube(Cobalt_StateContainer container, uint32_t textureId, Cobalt_TextureBufferCube texture, Cobalt_TextureSamplerCube sampler)
{
	auto _this = reinterpret_cast<IStateContainer*>(container);

	_this->BindTextureWithCombinedSampler((TextureId)textureId, reinterpret_cast<ITextureBufferCube*>(texture), reinterpret_cast<ITextureSamplerCube*>(sampler));
}

//----------------------------------------------------------------------------------------
void Cobalt_StateContainer_BindTextureWithCombinedSampler1DArray(Cobalt_StateContainer container, uint32_t textureId, Cobalt_TextureBuffer1DArray texture, Cobalt_TextureSampler1DArray sampler)
{
	auto _this = reinterpret_cast<IStateContainer*>(container);

	_this->BindTextureWithCombinedSampler((TextureId)textureId, reinterpret_cast<ITextureBuffer1DArray*>(texture), reinterpret_cast<ITextureSampler1DArray*>(sampler));
}

//----------------------------------------------------------------------------------------
void Cobalt_StateContainer_BindTextureWithCombinedSampler2DArray(Cobalt_StateContainer container, uint32_t textureId, Cobalt_TextureBuffer2DArray texture, Cobalt_TextureSampler2DArray sampler)
{
	auto _this = reinterpret_cast<IStateContainer*>(container);

	_this->BindTextureWithCombinedSampler((TextureId)textureId, reinterpret_cast<ITextureBuffer2DArray*>(texture), reinterpret_cast<ITextureSampler2DArray*>(sampler));
}

//----------------------------------------------------------------------------------------
void Cobalt_StateContainer_BindTextureWithCombinedSamplerCubeArray(Cobalt_StateContainer container, uint32_t textureId, Cobalt_TextureBufferCubeArray texture, Cobalt_TextureSamplerCubeArray sampler)
{
	auto _this = reinterpret_cast<IStateContainer*>(container);

	_this->BindTextureWithCombinedSampler((TextureId)textureId, reinterpret_cast<ITextureBufferCubeArray*>(texture), reinterpret_cast<ITextureSamplerCubeArray*>(sampler));
}

//----------------------------------------------------------------------------------------
void Cobalt_StateContainer_BindTexture1D(Cobalt_StateContainer container, uint32_t textureId, Cobalt_TextureBuffer1D texture)
{
	auto _this = reinterpret_cast<IStateContainer*>(container);

	_this->BindTexture((TextureId)textureId, reinterpret_cast<ITextureBuffer1D*>(texture));
}

//----------------------------------------------------------------------------------------
void Cobalt_StateContainer_BindTexture2D(Cobalt_StateContainer container, uint32_t textureId, Cobalt_TextureBuffer2D texture)
{
	auto _this = reinterpret_cast<IStateContainer*>(container);

	_this->BindTexture((TextureId)textureId, reinterpret_cast<ITextureBuffer2D*>(texture));
}

//----------------------------------------------------------------------------------------
void Cobalt_StateContainer_BindTexture3D(Cobalt_StateContainer container, uint32_t textureId, Cobalt_TextureBuffer3D texture)
{
	auto _this = reinterpret_cast<IStateContainer*>(container);

	_this->BindTexture((TextureId)textureId, reinterpret_cast<ITextureBuffer3D*>(texture));
}

//----------------------------------------------------------------------------------------
void Cobalt_StateContainer_BindTextureCube(Cobalt_StateContainer container, uint32_t textureId, Cobalt_TextureBufferCube texture)
{
	auto _this = reinterpret_cast<IStateContainer*>(container);

	_this->BindTexture((TextureId)textureId, reinterpret_cast<ITextureBufferCube*>(texture));
}

//----------------------------------------------------------------------------------------
void Cobalt_StateContainer_BindTexture1DArray(Cobalt_StateContainer container, uint32_t textureId, Cobalt_TextureBuffer1DArray texture)
{
	auto _this = reinterpret_cast<IStateContainer*>(container);

	_this->BindTexture((TextureId)textureId, reinterpret_cast<ITextureBuffer1DArray*>(texture));
}

//----------------------------------------------------------------------------------------
void Cobalt_StateContainer_BindTexture2DArray(Cobalt_StateContainer container, uint32_t textureId, Cobalt_TextureBuffer2DArray texture)
{
	auto _this = reinterpret_cast<IStateContainer*>(container);

	_this->BindTexture((TextureId)textureId, reinterpret_cast<ITextureBuffer2DArray*>(texture));
}

//----------------------------------------------------------------------------------------
void Cobalt_StateContainer_BindTextureCubeArray(Cobalt_StateContainer container, uint32_t textureId, Cobalt_TextureBufferCubeArray texture)
{
	auto _this = reinterpret_cast<IStateContainer*>(container);

	_this->BindTexture((TextureId)textureId, reinterpret_cast<ITextureBufferCubeArray*>(texture));
}

//----------------------------------------------------------------------------------------
void Cobalt_StateContainer_UnbindTexture(Cobalt_StateContainer container, uint32_t textureId)
{
	auto _this = reinterpret_cast<IStateContainer*>(container);

	_this->UnbindTexture((TextureId)textureId);
}

//----------------------------------------------------------------------------------------
void Cobalt_StateContainer_BindSampler1D(Cobalt_StateContainer container, uint32_t samplerId, Cobalt_TextureSampler1D sampler)
{
	auto _this = reinterpret_cast<IStateContainer*>(container);

	_this->BindSampler((SamplerId)samplerId, reinterpret_cast<ITextureSampler1D*>(sampler));
}

//----------------------------------------------------------------------------------------
void Cobalt_StateContainer_BindSampler2D(Cobalt_StateContainer container, uint32_t samplerId, Cobalt_TextureSampler2D sampler)
{
	auto _this = reinterpret_cast<IStateContainer*>(container);

	_this->BindSampler((SamplerId)samplerId, reinterpret_cast<ITextureSampler2D*>(sampler));
}

//----------------------------------------------------------------------------------------
void Cobalt_StateContainer_BindSampler3D(Cobalt_StateContainer container, uint32_t samplerId, Cobalt_TextureSampler3D sampler)
{
	auto _this = reinterpret_cast<IStateContainer*>(container);

	_this->BindSampler((SamplerId)samplerId, reinterpret_cast<ITextureSampler3D*>(sampler));
}

//----------------------------------------------------------------------------------------
void Cobalt_StateContainer_BindSamplerCube(Cobalt_StateContainer container, uint32_t samplerId, Cobalt_TextureSamplerCube sampler)
{
	auto _this = reinterpret_cast<IStateContainer*>(container);

	_this->BindSampler((SamplerId)samplerId, reinterpret_cast<ITextureSamplerCube*>(sampler));
}

//----------------------------------------------------------------------------------------
void Cobalt_StateContainer_BindSampler1DArray(Cobalt_StateContainer container, uint32_t samplerId, Cobalt_TextureSampler1DArray sampler)
{
	auto _this = reinterpret_cast<IStateContainer*>(container);

	_this->BindSampler((SamplerId)samplerId, reinterpret_cast<ITextureSampler1DArray*>(sampler));
}

//----------------------------------------------------------------------------------------
void Cobalt_StateContainer_BindSampler2DArray(Cobalt_StateContainer container, uint32_t samplerId, Cobalt_TextureSampler2DArray sampler)
{
	auto _this = reinterpret_cast<IStateContainer*>(container);

	_this->BindSampler((SamplerId)samplerId, reinterpret_cast<ITextureSampler2DArray*>(sampler));
}

//----------------------------------------------------------------------------------------
void Cobalt_StateContainer_BindSamplerCubeArray(Cobalt_StateContainer container, uint32_t samplerId, Cobalt_TextureSamplerCubeArray sampler)
{
	auto _this = reinterpret_cast<IStateContainer*>(container);

	_this->BindSampler((SamplerId)samplerId, reinterpret_cast<ITextureSamplerCubeArray*>(sampler));
}

//----------------------------------------------------------------------------------------
void Cobalt_StateContainer_UnbindSampler(Cobalt_StateContainer container, uint32_t samplerId)
{
	auto _this = reinterpret_cast<IStateContainer*>(container);

	_this->UnbindSampler((SamplerId)samplerId);
}

//----------------------------------------------------------------------------------------
void Cobalt_StateContainer_BindStateBuffer(Cobalt_StateContainer container, uint32_t stateBufferId, Cobalt_StateBuffer stateBuffer, uint32_t stateBufferPageNo)
{
	auto _this = reinterpret_cast<IStateContainer*>(container);

	_this->BindStateBuffer((StateBufferId)stateBufferId, reinterpret_cast<IStateBuffer*>(stateBuffer), stateBufferPageNo);
}

//----------------------------------------------------------------------------------------
void Cobalt_StateContainer_UnbindStateBuffer(Cobalt_StateContainer container, uint32_t stateBufferId)
{
	auto _this = reinterpret_cast<IStateContainer*>(container);

	_this->UnbindStateBuffer((StateBufferId)stateBufferId);
}

//----------------------------------------------------------------------------------------
void Cobalt_StateContainer_BindDataArray(Cobalt_StateContainer container, uint32_t resourceArrayId, Cobalt_DataArray dataArray, char resetCounter)
{
	auto _this = reinterpret_cast<IStateContainer*>(container);

	_this->BindResourceArray((ResourceArrayId)resourceArrayId, reinterpret_cast<IDataArray*>(dataArray), resetCounter != 0);
}

//----------------------------------------------------------------------------------------
void Cobalt_StateContainer_BindTexelArray(Cobalt_StateContainer container, uint32_t resourceArrayId, Cobalt_TexelArray texelArray)
{
	auto _this = reinterpret_cast<IStateContainer*>(container);

	_this->BindResourceArray((ResourceArrayId)resourceArrayId, reinterpret_cast<ITexelArray*>(texelArray));
}

//----------------------------------------------------------------------------------------
void Cobalt_StateContainer_UnbindResourceArray(Cobalt_StateContainer container, uint32_t resourceArrayId)
{
	auto _this = reinterpret_cast<IStateContainer*>(container);

	_this->UnbindResourceArray((ResourceArrayId)resourceArrayId);
}

//----------------------------------------------------------------------------------------
// State value methods
//----------------------------------------------------------------------------------------
void Cobalt_StateContainer_SetStateValueBool(Cobalt_StateContainer container, uint32_t stateId, char value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	auto _this = reinterpret_cast<IStateContainer*>(container);

	_this->SetStateValue((StateValueId)stateId, value != 0, arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
void Cobalt_StateContainer_SetStateValueV1Int8(Cobalt_StateContainer container, uint32_t stateId, int8_t value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	auto _this = reinterpret_cast<IStateContainer*>(container);

	_this->SetStateValue((StateValueId)stateId, V1Int8(value), arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
void Cobalt_StateContainer_SetStateValueV1Int16(Cobalt_StateContainer container, uint32_t stateId, int16_t value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	auto _this = reinterpret_cast<IStateContainer*>(container);

	_this->SetStateValue((StateValueId)stateId, V1Int16(value), arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
void Cobalt_StateContainer_SetStateValueV1Int32(Cobalt_StateContainer container, uint32_t stateId, int32_t value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	auto _this = reinterpret_cast<IStateContainer*>(container);

	_this->SetStateValue((StateValueId)stateId, V1Int32(value), arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
void Cobalt_StateContainer_SetStateValueV1UInt8(Cobalt_StateContainer container, uint32_t stateId, uint8_t value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	auto _this = reinterpret_cast<IStateContainer*>(container);

	_this->SetStateValue((StateValueId)stateId, V1UInt8(value), arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
void Cobalt_StateContainer_SetStateValueV1UInt16(Cobalt_StateContainer container, uint32_t stateId, uint16_t value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	auto _this = reinterpret_cast<IStateContainer*>(container);

	_this->SetStateValue((StateValueId)stateId, V1UInt16(value), arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
void Cobalt_StateContainer_SetStateValueV1UInt32(Cobalt_StateContainer container, uint32_t stateId, uint32_t value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	auto _this = reinterpret_cast<IStateContainer*>(container);

	_this->SetStateValue((StateValueId)stateId, V1UInt32(value), arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
void Cobalt_StateContainer_SetStateValueV1Float32(Cobalt_StateContainer container, uint32_t stateId, float value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	auto _this = reinterpret_cast<IStateContainer*>(container);

	_this->SetStateValue((StateValueId)stateId, V1Float32(value), arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
void Cobalt_StateContainer_SetStateValueV1Float64(Cobalt_StateContainer container, uint32_t stateId, double value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	auto _this = reinterpret_cast<IStateContainer*>(container);

	_this->SetStateValue((StateValueId)stateId, V1Float64(value), arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
void Cobalt_StateContainer_SetStateValueV2Int8(Cobalt_StateContainer container, uint32_t stateId, const int8_t value[2], const size_t* arrayIndices, size_t arrayIndexCount)
{
	auto _this = reinterpret_cast<IStateContainer*>(container);

	_this->SetStateValue((StateValueId)stateId, V2Int8(value[0], value[1]), arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
void Cobalt_StateContainer_SetStateValueV2Int16(Cobalt_StateContainer container, uint32_t stateId, const int16_t value[2], const size_t* arrayIndices, size_t arrayIndexCount)
{
	auto _this = reinterpret_cast<IStateContainer*>(container);

	_this->SetStateValue((StateValueId)stateId, V2Int16(value[0], value[1]), arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
void Cobalt_StateContainer_SetStateValueV2Int32(Cobalt_StateContainer container, uint32_t stateId, const int32_t value[2], const size_t* arrayIndices, size_t arrayIndexCount)
{
	auto _this = reinterpret_cast<IStateContainer*>(container);

	_this->SetStateValue((StateValueId)stateId, V2Int32(value[0], value[1]), arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
void Cobalt_StateContainer_SetStateValueV2UInt8(Cobalt_StateContainer container, uint32_t stateId, const uint8_t value[2], const size_t* arrayIndices, size_t arrayIndexCount)
{
	auto _this = reinterpret_cast<IStateContainer*>(container);

	_this->SetStateValue((StateValueId)stateId, V2UInt8(value[0], value[1]), arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
void Cobalt_StateContainer_SetStateValueV2UInt16(Cobalt_StateContainer container, uint32_t stateId, const uint16_t value[2], const size_t* arrayIndices, size_t arrayIndexCount)
{
	auto _this = reinterpret_cast<IStateContainer*>(container);

	_this->SetStateValue((StateValueId)stateId, V2UInt16(value[0], value[1]), arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
void Cobalt_StateContainer_SetStateValueV2UInt32(Cobalt_StateContainer container, uint32_t stateId, const uint32_t value[2], const size_t* arrayIndices, size_t arrayIndexCount)
{
	auto _this = reinterpret_cast<IStateContainer*>(container);

	_this->SetStateValue((StateValueId)stateId, V2UInt32(value[0], value[1]), arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
void Cobalt_StateContainer_SetStateValueV2Float32(Cobalt_StateContainer container, uint32_t stateId, const float value[2], const size_t* arrayIndices, size_t arrayIndexCount)
{
	auto _this = reinterpret_cast<IStateContainer*>(container);

	_this->SetStateValue((StateValueId)stateId, V2Float32(value[0], value[1]), arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
void Cobalt_StateContainer_SetStateValueV2Float64(Cobalt_StateContainer container, uint32_t stateId, const double value[2], const size_t* arrayIndices, size_t arrayIndexCount)
{
	auto _this = reinterpret_cast<IStateContainer*>(container);

	_this->SetStateValue((StateValueId)stateId, V2Float64(value[0], value[1]), arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
void Cobalt_StateContainer_SetStateValueV3Int8(Cobalt_StateContainer container, uint32_t stateId, const int8_t value[3], const size_t* arrayIndices, size_t arrayIndexCount)
{
	auto _this = reinterpret_cast<IStateContainer*>(container);

	_this->SetStateValue((StateValueId)stateId, V3Int8(value[0], value[1], value[2]), arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
void Cobalt_StateContainer_SetStateValueV3Int16(Cobalt_StateContainer container, uint32_t stateId, const int16_t value[3], const size_t* arrayIndices, size_t arrayIndexCount)
{
	auto _this = reinterpret_cast<IStateContainer*>(container);

	_this->SetStateValue((StateValueId)stateId, V3Int16(value[0], value[1], value[2]), arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
void Cobalt_StateContainer_SetStateValueV3Int32(Cobalt_StateContainer container, uint32_t stateId, const int32_t value[3], const size_t* arrayIndices, size_t arrayIndexCount)
{
	auto _this = reinterpret_cast<IStateContainer*>(container);

	_this->SetStateValue((StateValueId)stateId, V3Int32(value[0], value[1], value[2]), arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
void Cobalt_StateContainer_SetStateValueV3UInt8(Cobalt_StateContainer container, uint32_t stateId, const uint8_t value[3], const size_t* arrayIndices, size_t arrayIndexCount)
{
	auto _this = reinterpret_cast<IStateContainer*>(container);

	_this->SetStateValue((StateValueId)stateId, V3UInt8(value[0], value[1], value[2]), arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
void Cobalt_StateContainer_SetStateValueV3UInt16(Cobalt_StateContainer container, uint32_t stateId, const uint16_t value[3], const size_t* arrayIndices, size_t arrayIndexCount)
{
	auto _this = reinterpret_cast<IStateContainer*>(container);

	_this->SetStateValue((StateValueId)stateId, V3UInt16(value[0], value[1], value[2]), arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
void Cobalt_StateContainer_SetStateValueV3UInt32(Cobalt_StateContainer container, uint32_t stateId, const uint32_t value[3], const size_t* arrayIndices, size_t arrayIndexCount)
{
	auto _this = reinterpret_cast<IStateContainer*>(container);

	_this->SetStateValue((StateValueId)stateId, V3UInt32(value[0], value[1], value[2]), arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
void Cobalt_StateContainer_SetStateValueV3Float32(Cobalt_StateContainer container, uint32_t stateId, const float value[3], const size_t* arrayIndices, size_t arrayIndexCount)
{
	auto _this = reinterpret_cast<IStateContainer*>(container);

	_this->SetStateValue((StateValueId)stateId, V3Float32(value[0], value[1], value[2]), arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
void Cobalt_StateContainer_SetStateValueV3Float64(Cobalt_StateContainer container, uint32_t stateId, const double value[3], const size_t* arrayIndices, size_t arrayIndexCount)
{
	auto _this = reinterpret_cast<IStateContainer*>(container);

	_this->SetStateValue((StateValueId)stateId, V3Float64(value[0], value[1], value[2]), arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
void Cobalt_StateContainer_SetStateValueV4Int8(Cobalt_StateContainer container, uint32_t stateId, const int8_t value[4], const size_t* arrayIndices, size_t arrayIndexCount)
{
	auto _this = reinterpret_cast<IStateContainer*>(container);

	_this->SetStateValue((StateValueId)stateId, V4Int8(value[0], value[1], value[2], value[3]), arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
void Cobalt_StateContainer_SetStateValueV4Int16(Cobalt_StateContainer container, uint32_t stateId, const int16_t value[4], const size_t* arrayIndices, size_t arrayIndexCount)
{
	auto _this = reinterpret_cast<IStateContainer*>(container);

	_this->SetStateValue((StateValueId)stateId, V4Int16(value[0], value[1], value[2], value[3]), arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
void Cobalt_StateContainer_SetStateValueV4Int32(Cobalt_StateContainer container, uint32_t stateId, const int32_t value[4], const size_t* arrayIndices, size_t arrayIndexCount)
{
	auto _this = reinterpret_cast<IStateContainer*>(container);

	_this->SetStateValue((StateValueId)stateId, V4Int32(value[0], value[1], value[2], value[3]), arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
void Cobalt_StateContainer_SetStateValueV4UInt8(Cobalt_StateContainer container, uint32_t stateId, const uint8_t value[4], const size_t* arrayIndices, size_t arrayIndexCount)
{
	auto _this = reinterpret_cast<IStateContainer*>(container);

	_this->SetStateValue((StateValueId)stateId, V4UInt8(value[0], value[1], value[2], value[3]), arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
void Cobalt_StateContainer_SetStateValueV4UInt16(Cobalt_StateContainer container, uint32_t stateId, const uint16_t value[4], const size_t* arrayIndices, size_t arrayIndexCount)
{
	auto _this = reinterpret_cast<IStateContainer*>(container);

	_this->SetStateValue((StateValueId)stateId, V4UInt16(value[0], value[1], value[2], value[3]), arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
void Cobalt_StateContainer_SetStateValueV4UInt32(Cobalt_StateContainer container, uint32_t stateId, const uint32_t value[4], const size_t* arrayIndices, size_t arrayIndexCount)
{
	auto _this = reinterpret_cast<IStateContainer*>(container);

	_this->SetStateValue((StateValueId)stateId, V4UInt32(value[0], value[1], value[2], value[3]), arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
void Cobalt_StateContainer_SetStateValueV4Float32(Cobalt_StateContainer container, uint32_t stateId, const float value[4], const size_t* arrayIndices, size_t arrayIndexCount)
{
	auto _this = reinterpret_cast<IStateContainer*>(container);

	_this->SetStateValue((StateValueId)stateId, V4Float32(value[0], value[1], value[2], value[3]), arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
void Cobalt_StateContainer_SetStateValueV4Float64(Cobalt_StateContainer container, uint32_t stateId, const double value[4], const size_t* arrayIndices, size_t arrayIndexCount)
{
	auto _this = reinterpret_cast<IStateContainer*>(container);

	_this->SetStateValue((StateValueId)stateId, V4Float64(value[0], value[1], value[2], value[3]), arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
void Cobalt_StateContainer_SetStateValueM2Float32(Cobalt_StateContainer container, uint32_t stateId, const float value[4], const size_t* arrayIndices, size_t arrayIndexCount)
{
	auto _this = reinterpret_cast<IStateContainer*>(container);

	_this->SetStateValue((StateValueId)stateId, *reinterpret_cast<const M2Float32*>(value), arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
void Cobalt_StateContainer_SetStateValueM3Float32(Cobalt_StateContainer container, uint32_t stateId, const float value[9], const size_t* arrayIndices, size_t arrayIndexCount)
{
	auto _this = reinterpret_cast<IStateContainer*>(container);

	_this->SetStateValue((StateValueId)stateId, *reinterpret_cast<const M3Float32*>(value), arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
void Cobalt_StateContainer_SetStateValueM4Float32(Cobalt_StateContainer container, uint32_t stateId, const float value[16], const size_t* arrayIndices, size_t arrayIndexCount)
{
	auto _this = reinterpret_cast<IStateContainer*>(container);

	_this->SetStateValue((StateValueId)stateId, *reinterpret_cast<const M4Float32*>(value), arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
void Cobalt_StateContainer_ResetStateValue(Cobalt_StateContainer container, uint32_t stateId, const size_t* arrayIndices, size_t arrayIndexCount)
{
	auto _this = reinterpret_cast<IStateContainer*>(container);

	_this->ResetStateValue((StateValueId)stateId, arrayIndices, arrayIndexCount);
}
