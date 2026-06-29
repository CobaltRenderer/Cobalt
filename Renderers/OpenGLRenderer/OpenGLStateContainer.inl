// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "OpenGLTextureBuffer1D.h"
#include "OpenGLTextureBuffer1DArray.h"
#include "OpenGLTextureBuffer2D.h"
#include "OpenGLTextureBuffer2DArray.h"
#include "OpenGLTextureBuffer3D.h"
#include "OpenGLTextureBufferCube.h"
#include "OpenGLTextureBufferCubeArray.h"
#include "OpenGLTextureSampler1D.h"
#include "OpenGLTextureSampler1DArray.h"
#include "OpenGLTextureSampler2D.h"
#include "OpenGLTextureSampler2DArray.h"
#include "OpenGLTextureSampler3D.h"
#include "OpenGLTextureSamplerCube.h"
#include "OpenGLTextureSamplerCubeArray.h"
#include <Internal/RendererSupport/KnownDynamicCast.h>
#include <type_traits>
namespace cobalt::graphics {

//----------------------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------------------
template<class T>
OpenGLStateContainer<T>::OpenGLStateContainer(cobalt::logging::ILogger* log)
: _log(log)
{}

//----------------------------------------------------------------------------------------
template<class T>
OpenGLStateContainer<T>::~OpenGLStateContainer()
{
	DeleteRemovedEntries(_drawState);
	DeleteRemovedEntries(_buildState);
	for (size_t i = 0; i < _buildState.valueEntries.size(); ++i)
	{
		delete _buildState.valueEntries[i];
	}
	for (size_t i = 0; i < _buildState.textureEntries.size(); ++i)
	{
		delete _buildState.textureEntries[i];
	}
	for (size_t i = 0; i < _buildState.stateBufferEntries.size(); ++i)
	{
		delete _buildState.stateBufferEntries[i];
	}
	for (size_t i = 0; i < _buildState.resourceBufferEntries.size(); ++i)
	{
		delete _buildState.resourceBufferEntries[i];
	}
}

//----------------------------------------------------------------------------------------
// Resource binding methods
//----------------------------------------------------------------------------------------
template<class T>
void OpenGLStateContainer<T>::BindTextureWithCombinedSampler(TextureId textureId, ITextureBuffer1D* texture, ITextureSampler1D* sampler)
{
	BindTextureWithCombinedSamplerInternal(textureId, GL_TEXTURE_1D, KnownDynamicCast<OpenGLTextureBuffer1D*>(texture), KnownDynamicCast<OpenGLTextureSampler1D*>(sampler));
}

//----------------------------------------------------------------------------------------
template<class T>
void OpenGLStateContainer<T>::BindTextureWithCombinedSampler(TextureId textureId, ITextureBuffer2D* texture, ITextureSampler2D* sampler)
{
	BindTextureWithCombinedSamplerInternal(textureId, GL_TEXTURE_2D, KnownDynamicCast<OpenGLTextureBuffer2D*>(texture), KnownDynamicCast<OpenGLTextureSampler2D*>(sampler));
}

//----------------------------------------------------------------------------------------
template<class T>
void OpenGLStateContainer<T>::BindTextureWithCombinedSampler(TextureId textureId, ITextureBuffer3D* texture, ITextureSampler3D* sampler)
{
	BindTextureWithCombinedSamplerInternal(textureId, GL_TEXTURE_3D, KnownDynamicCast<OpenGLTextureBuffer3D*>(texture), KnownDynamicCast<OpenGLTextureSampler3D*>(sampler));
}

//----------------------------------------------------------------------------------------
template<class T>
void OpenGLStateContainer<T>::BindTextureWithCombinedSampler(TextureId textureId, ITextureBufferCube* texture, ITextureSamplerCube* sampler)
{
	BindTextureWithCombinedSamplerInternal(textureId, GL_TEXTURE_CUBE_MAP, KnownDynamicCast<OpenGLTextureBufferCube*>(texture), KnownDynamicCast<OpenGLTextureSamplerCube*>(sampler));
}

//----------------------------------------------------------------------------------------
template<class T>
void OpenGLStateContainer<T>::BindTextureWithCombinedSampler(TextureId textureId, ITextureBuffer1DArray* texture, ITextureSampler1DArray* sampler)
{
	BindTextureWithCombinedSamplerInternal(textureId, GL_TEXTURE_1D_ARRAY, KnownDynamicCast<OpenGLTextureBuffer1DArray*>(texture), KnownDynamicCast<OpenGLTextureSampler1DArray*>(sampler));
}

//----------------------------------------------------------------------------------------
template<class T>
void OpenGLStateContainer<T>::BindTextureWithCombinedSampler(TextureId textureId, ITextureBuffer2DArray* texture, ITextureSampler2DArray* sampler)
{
	BindTextureWithCombinedSamplerInternal(textureId, GL_TEXTURE_2D_ARRAY, KnownDynamicCast<OpenGLTextureBuffer2DArray*>(texture), KnownDynamicCast<OpenGLTextureSampler2DArray*>(sampler));
}

//----------------------------------------------------------------------------------------
template<class T>
void OpenGLStateContainer<T>::BindTextureWithCombinedSampler(TextureId textureId, ITextureBufferCubeArray* texture, ITextureSamplerCubeArray* sampler)
{
	BindTextureWithCombinedSamplerInternal(textureId,
#ifdef GL_VERSION_4_0
	                                       GL_TEXTURE_CUBE_MAP_ARRAY,
#else
	                                       0,
#endif
	                                       KnownDynamicCast<OpenGLTextureBufferCubeArray*>(texture),
	                                       KnownDynamicCast<OpenGLTextureSamplerCubeArray*>(sampler));
}

//----------------------------------------------------------------------------------------
template<class T>
template<class TextureType, class SamplerType>
void OpenGLStateContainer<T>::BindTextureWithCombinedSamplerInternal(TextureId textureId, GLenum textureTarget, TextureType* texture, SamplerType* sampler)
{
	// Validate the supplied texture ID
	if (textureId == TextureId::Null)
	{
		_log->Warning("Attempted to bind texture with invalid ID \"{0}\"", textureId);
		return;
	}

	// Since we're updating the bound state for this node, flag that the draw state is no longer current.
	OpenGLNode<T>::FlagDrawStateNotCurrent();

	// Add a binding entry for this texture, removing any existing binding that may be in place against the target
	// texture ID.
	auto* value = new TextureBindingWithCombinedSamplerInfo(textureId, textureTarget, texture, sampler);
	for (size_t i = 0; i < _buildState.textureEntries.size(); ++i)
	{
		ITextureBindingInfo* bindingInfo = _buildState.textureEntries[i];
		if (bindingInfo->GetTextureId() == textureId)
		{
			_buildState.textureEntriesToDelete.push_back(bindingInfo);
			_buildState.textureEntries[i] = value;
			return;
		}
	}
	_buildState.textureEntries.push_back(value);
}

//----------------------------------------------------------------------------------------
template<class T>
void OpenGLStateContainer<T>::BindTexture(TextureId textureId, ITextureBuffer1D* texture)
{
	BindTextureInternal(textureId, GL_TEXTURE_1D, KnownDynamicCast<OpenGLTextureBuffer1D*>(texture));
}

//----------------------------------------------------------------------------------------
template<class T>
void OpenGLStateContainer<T>::BindTexture(TextureId textureId, ITextureBuffer2D* texture)
{
	BindTextureInternal(textureId, GL_TEXTURE_2D, KnownDynamicCast<OpenGLTextureBuffer2D*>(texture));
}

//----------------------------------------------------------------------------------------
template<class T>
void OpenGLStateContainer<T>::BindTexture(TextureId textureId, ITextureBuffer3D* texture)
{
	BindTextureInternal(textureId, GL_TEXTURE_3D, KnownDynamicCast<OpenGLTextureBuffer3D*>(texture));
}

//----------------------------------------------------------------------------------------
template<class T>
void OpenGLStateContainer<T>::BindTexture(TextureId textureId, ITextureBufferCube* texture)
{
	BindTextureInternal(textureId, GL_TEXTURE_CUBE_MAP, KnownDynamicCast<OpenGLTextureBufferCube*>(texture));
}

//----------------------------------------------------------------------------------------
template<class T>
void OpenGLStateContainer<T>::BindTexture(TextureId textureId, ITextureBuffer1DArray* texture)
{
	BindTextureInternal(textureId, GL_TEXTURE_1D_ARRAY, KnownDynamicCast<OpenGLTextureBuffer1DArray*>(texture));
}

//----------------------------------------------------------------------------------------
template<class T>
void OpenGLStateContainer<T>::BindTexture(TextureId textureId, ITextureBuffer2DArray* texture)
{
	BindTextureInternal(textureId, GL_TEXTURE_2D_ARRAY, KnownDynamicCast<OpenGLTextureBuffer2DArray*>(texture));
}

//----------------------------------------------------------------------------------------
template<class T>
void OpenGLStateContainer<T>::BindTexture(TextureId textureId, ITextureBufferCubeArray* texture)
{
	BindTextureInternal(textureId,
#ifdef GL_VERSION_4_0
	                    GL_TEXTURE_CUBE_MAP_ARRAY,
#else
	                    0,
#endif
	                    KnownDynamicCast<OpenGLTextureBufferCubeArray*>(texture));
}

//----------------------------------------------------------------------------------------
template<class T>
template<class TextureType>
void OpenGLStateContainer<T>::BindTextureInternal(TextureId textureId, GLenum textureTarget, TextureType* texture)
{
	_log->Error("Attempted to bind a texture without a sampler object, but OpenGL only supports combined image samplers.");
}

//----------------------------------------------------------------------------------------
template<class T>
void OpenGLStateContainer<T>::UnbindTexture(TextureId textureId)
{
	// Validate the supplied texture ID
	if (textureId == TextureId::Null)
	{
		_log->Warning("Attempted to unbind texture with invalid ID \"{0}\"", textureId);
		return;
	}

	// Since we're updating the bound state for this node, flag that the draw state is no longer current.
	OpenGLNode<T>::FlagDrawStateNotCurrent();

	// Remove any existing binding that may be in place against the target texture ID
	for (size_t i = 0; i < _buildState.textureEntries.size(); ++i)
	{
		ITextureBindingInfo* bindingInfo = _buildState.textureEntries[i];
		if (bindingInfo->GetTextureId() == textureId)
		{
			_buildState.textureEntriesToDelete.push_back(bindingInfo);
			_buildState.textureEntries.erase(_buildState.textureEntries.begin() + i);
			return;
		}
	}
}

//----------------------------------------------------------------------------------------
template<class T>
void OpenGLStateContainer<T>::BindSampler(SamplerId samplerId, ITextureSampler1D* sampler)
{
	BindSamplerInternal(samplerId, GL_TEXTURE_1D, KnownDynamicCast<OpenGLTextureSampler1D*>(sampler));
}

//----------------------------------------------------------------------------------------
template<class T>
void OpenGLStateContainer<T>::BindSampler(SamplerId samplerId, ITextureSampler2D* sampler)
{
	BindSamplerInternal(samplerId, GL_TEXTURE_2D, KnownDynamicCast<OpenGLTextureSampler2D*>(sampler));
}

//----------------------------------------------------------------------------------------
template<class T>
void OpenGLStateContainer<T>::BindSampler(SamplerId samplerId, ITextureSampler3D* sampler)
{
	BindSamplerInternal(samplerId, GL_TEXTURE_3D, KnownDynamicCast<OpenGLTextureSampler3D*>(sampler));
}

//----------------------------------------------------------------------------------------
template<class T>
void OpenGLStateContainer<T>::BindSampler(SamplerId samplerId, ITextureSamplerCube* sampler)
{
	BindSamplerInternal(samplerId, GL_TEXTURE_CUBE_MAP, KnownDynamicCast<OpenGLTextureSamplerCube*>(sampler));
}

//----------------------------------------------------------------------------------------
template<class T>
void OpenGLStateContainer<T>::BindSampler(SamplerId samplerId, ITextureSampler1DArray* sampler)
{
	BindSamplerInternal(samplerId, GL_TEXTURE_1D_ARRAY, KnownDynamicCast<OpenGLTextureSampler1DArray*>(sampler));
}

//----------------------------------------------------------------------------------------
template<class T>
void OpenGLStateContainer<T>::BindSampler(SamplerId samplerId, ITextureSampler2DArray* sampler)
{
	BindSamplerInternal(samplerId, GL_TEXTURE_2D_ARRAY, KnownDynamicCast<OpenGLTextureSampler2DArray*>(sampler));
}

//----------------------------------------------------------------------------------------
template<class T>
void OpenGLStateContainer<T>::BindSampler(SamplerId samplerId, ITextureSamplerCubeArray* sampler)
{
	BindSamplerInternal(samplerId,
#ifdef GL_VERSION_4_0
	                    GL_TEXTURE_CUBE_MAP_ARRAY,
#else
	                    0,
#endif
	                    KnownDynamicCast<OpenGLTextureSamplerCubeArray*>(sampler));
}

//----------------------------------------------------------------------------------------
template<class T>
template<class SamplerType>
void OpenGLStateContainer<T>::BindSamplerInternal(SamplerId samplerId, GLenum textureTarget, SamplerType* sampler)
{
	_log->Error("Attempted to bind a sampler object, but OpenGL only supports combined image samplers.");
}

//----------------------------------------------------------------------------------------
template<class T>
void OpenGLStateContainer<T>::UnbindSampler(SamplerId samplerId)
{
	_log->Error("Attempted to unbind a sampler object, but OpenGL only supports combined image samplers.");
}

//----------------------------------------------------------------------------------------
template<class T>
void OpenGLStateContainer<T>::BindStateBuffer(StateBufferId stateBufferId, IStateBuffer* stateBuffer, uint32_t stateBufferPageNo)
{
	// Validate the supplied state buffer ID
	if (stateBufferId == StateBufferId::Null)
	{
		_log->Warning("Attempted to bind state buffer with ID \"{0}\"", stateBufferId);
		return;
	}

	// Since we're updating the bound state for this node, flag that the draw state is no longer current.
	OpenGLNode<T>::FlagDrawStateNotCurrent();

	// Add a binding entry for this state buffer, removing any existing binding that may be in place against the target
	// state buffer ID.
	auto* value = new StateBufferBindingInfo(stateBufferId, stateBufferPageNo, stateBuffer);
	for (size_t i = 0; i < _buildState.stateBufferEntries.size(); ++i)
	{
		StateBufferBindingInfo* bindingInfo = _buildState.stateBufferEntries[i];
		if (bindingInfo->GetStateBufferId() == stateBufferId)
		{
			_buildState.stateBufferEntriesToDelete.push_back(bindingInfo);
			_buildState.stateBufferEntries[i] = value;
			return;
		}
	}
	_buildState.stateBufferEntries.push_back(value);
}

//----------------------------------------------------------------------------------------
template<class T>
void OpenGLStateContainer<T>::UnbindStateBuffer(StateBufferId stateBufferId)
{
	// Validate the supplied state buffer ID
	if (stateBufferId == StateBufferId::Null)
	{
		_log->Warning("Attempted to unbind state buffer with ID \"{0}\"", stateBufferId);
		return;
	}

	// Since we're updating the bound state for this node, flag that the draw state is no longer current.
	OpenGLNode<T>::FlagDrawStateNotCurrent();

	// Remove any existing binding that may be in place against the target state buffer ID
	for (size_t i = 0; i < _buildState.stateBufferEntries.size(); ++i)
	{
		StateBufferBindingInfo* bindingInfo = _buildState.stateBufferEntries[i];
		if (bindingInfo->GetStateBufferId() == stateBufferId)
		{
			_buildState.stateBufferEntriesToDelete.push_back(bindingInfo);
			_buildState.stateBufferEntries.erase(_buildState.stateBufferEntries.begin() + i);
		}
	}
}

//----------------------------------------------------------------------------------------
template<class T>
void OpenGLStateContainer<T>::BindResourceArray(ResourceArrayId resourceArrayId, IDataArray* resourceBuffer, bool resetCounter)
{
	// Validate the supplied data array ID
	if (resourceArrayId == ResourceArrayId::Null)
	{
		_log->Warning("Attempted to bind resource array with ID \"{0}\"", resourceArrayId);
		return;
	}

	// Since we're updating the bound state for this node, flag that the draw state is no longer current.
	OpenGLNode<T>::FlagDrawStateNotCurrent();

	// Add a binding entry for this data array, removing any existing binding that may be in place against the
	// target data array ID.
	auto* value = new ResourceArrayBindingInfo(resourceArrayId, resourceBuffer, resetCounter);
	for (size_t i = 0; i < _buildState.resourceBufferEntries.size(); ++i)
	{
		ResourceArrayBindingInfo* bindingInfo = _buildState.resourceBufferEntries[i];
		if (bindingInfo->GetResourceArrayId() == resourceArrayId)
		{
			_buildState.resourceBufferEntriesToDelete.push_back(bindingInfo);
			_buildState.resourceBufferEntries[i] = value;
			return;
		}
	}
	_buildState.resourceBufferEntries.push_back(value);
}

//----------------------------------------------------------------------------------------
template<class T>
void OpenGLStateContainer<T>::BindResourceArray(ResourceArrayId resourceArrayId, ITexelArray* resourceBuffer)
{
	// Validate the supplied data array ID
	if (resourceArrayId == ResourceArrayId::Null)
	{
		_log->Warning("Attempted to bind resource array with ID \"{0}\"", resourceArrayId);
		return;
	}

	// Since we're updating the bound state for this node, flag that the draw state is no longer current.
	OpenGLNode<T>::FlagDrawStateNotCurrent();

	// Add a binding entry for this data array, removing any existing binding that may be in place against the
	// target data array ID.
	auto* value = new ResourceArrayBindingInfo(resourceArrayId, resourceBuffer);
	for (size_t i = 0; i < _buildState.resourceBufferEntries.size(); ++i)
	{
		ResourceArrayBindingInfo* bindingInfo = _buildState.resourceBufferEntries[i];
		if (bindingInfo->GetResourceArrayId() == resourceArrayId)
		{
			_buildState.resourceBufferEntriesToDelete.push_back(bindingInfo);
			_buildState.resourceBufferEntries[i] = value;
			return;
		}
	}
	_buildState.resourceBufferEntries.push_back(value);
}

//----------------------------------------------------------------------------------------
template<class T>
void OpenGLStateContainer<T>::UnbindResourceArray(ResourceArrayId resourceArrayId)
{
	// Validate the supplied data array ID
	if (resourceArrayId == ResourceArrayId::Null)
	{
		_log->Warning("Attempted to unbind resource array with ID \"{0}\"", resourceArrayId);
		return;
	}

	// Since we're updating the bound state for this node, flag that the draw state is no longer current.
	OpenGLNode<T>::FlagDrawStateNotCurrent();

	// Remove any existing binding that may be in place against the target data array ID
	for (size_t i = 0; i < _buildState.resourceBufferEntries.size(); ++i)
	{
		ResourceArrayBindingInfo* bindingInfo = _buildState.resourceBufferEntries[i];
		if (bindingInfo->GetResourceArrayId() == resourceArrayId)
		{
			_buildState.resourceBufferEntriesToDelete.push_back(bindingInfo);
			_buildState.resourceBufferEntries.erase(_buildState.resourceBufferEntries.begin() + i);
		}
	}
}

//----------------------------------------------------------------------------------------
// State value methods
//----------------------------------------------------------------------------------------
template<class T>
void OpenGLStateContainer<T>::SetStateValueInternal(StateValueId stateId, bool value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetStateValueInternal(stateId, arrayIndices, arrayIndexCount, new StateValueInfo<std::remove_const<std::remove_reference<decltype(value)>::type>::type>(stateId, value, arrayIndices, arrayIndexCount));
}

//----------------------------------------------------------------------------------------
template<class T>
void OpenGLStateContainer<T>::SetStateValueInternal(StateValueId stateId, const V1Int8& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetStateValueInternal(stateId, arrayIndices, arrayIndexCount, new StateValueInfo<std::remove_const<std::remove_reference<decltype(value)>::type>::type>(stateId, value, arrayIndices, arrayIndexCount));
}

//----------------------------------------------------------------------------------------
template<class T>
void OpenGLStateContainer<T>::SetStateValueInternal(StateValueId stateId, const V1Int16& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetStateValueInternal(stateId, arrayIndices, arrayIndexCount, new StateValueInfo<std::remove_const<std::remove_reference<decltype(value)>::type>::type>(stateId, value, arrayIndices, arrayIndexCount));
}

//----------------------------------------------------------------------------------------
template<class T>
void OpenGLStateContainer<T>::SetStateValueInternal(StateValueId stateId, const V1Int32& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetStateValueInternal(stateId, arrayIndices, arrayIndexCount, new StateValueInfo<std::remove_const<std::remove_reference<decltype(value)>::type>::type>(stateId, value, arrayIndices, arrayIndexCount));
}

//----------------------------------------------------------------------------------------
template<class T>
void OpenGLStateContainer<T>::SetStateValueInternal(StateValueId stateId, const V1UInt8& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetStateValueInternal(stateId, arrayIndices, arrayIndexCount, new StateValueInfo<std::remove_const<std::remove_reference<decltype(value)>::type>::type>(stateId, value, arrayIndices, arrayIndexCount));
}

//----------------------------------------------------------------------------------------
template<class T>
void OpenGLStateContainer<T>::SetStateValueInternal(StateValueId stateId, const V1UInt16& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetStateValueInternal(stateId, arrayIndices, arrayIndexCount, new StateValueInfo<std::remove_const<std::remove_reference<decltype(value)>::type>::type>(stateId, value, arrayIndices, arrayIndexCount));
}

//----------------------------------------------------------------------------------------
template<class T>
void OpenGLStateContainer<T>::SetStateValueInternal(StateValueId stateId, const V1UInt32& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetStateValueInternal(stateId, arrayIndices, arrayIndexCount, new StateValueInfo<std::remove_const<std::remove_reference<decltype(value)>::type>::type>(stateId, value, arrayIndices, arrayIndexCount));
}

//----------------------------------------------------------------------------------------
template<class T>
void OpenGLStateContainer<T>::SetStateValueInternal(StateValueId stateId, const V1Float32& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetStateValueInternal(stateId, arrayIndices, arrayIndexCount, new StateValueInfo<std::remove_const<std::remove_reference<decltype(value)>::type>::type>(stateId, value, arrayIndices, arrayIndexCount));
}

//----------------------------------------------------------------------------------------
template<class T>
void OpenGLStateContainer<T>::SetStateValueInternal(StateValueId stateId, const V1Float64& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetStateValueInternal(stateId, arrayIndices, arrayIndexCount, new StateValueInfo<std::remove_const<std::remove_reference<decltype(value)>::type>::type>(stateId, value, arrayIndices, arrayIndexCount));
}

//----------------------------------------------------------------------------------------
template<class T>
void OpenGLStateContainer<T>::SetStateValueInternal(StateValueId stateId, const V2Int8& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetStateValueInternal(stateId, arrayIndices, arrayIndexCount, new StateValueInfo<std::remove_const<std::remove_reference<decltype(value)>::type>::type>(stateId, value, arrayIndices, arrayIndexCount));
}

//----------------------------------------------------------------------------------------
template<class T>
void OpenGLStateContainer<T>::SetStateValueInternal(StateValueId stateId, const V2Int16& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetStateValueInternal(stateId, arrayIndices, arrayIndexCount, new StateValueInfo<std::remove_const<std::remove_reference<decltype(value)>::type>::type>(stateId, value, arrayIndices, arrayIndexCount));
}

//----------------------------------------------------------------------------------------
template<class T>
void OpenGLStateContainer<T>::SetStateValueInternal(StateValueId stateId, const V2Int32& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetStateValueInternal(stateId, arrayIndices, arrayIndexCount, new StateValueInfo<std::remove_const<std::remove_reference<decltype(value)>::type>::type>(stateId, value, arrayIndices, arrayIndexCount));
}

//----------------------------------------------------------------------------------------
template<class T>
void OpenGLStateContainer<T>::SetStateValueInternal(StateValueId stateId, const V2UInt8& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetStateValueInternal(stateId, arrayIndices, arrayIndexCount, new StateValueInfo<std::remove_const<std::remove_reference<decltype(value)>::type>::type>(stateId, value, arrayIndices, arrayIndexCount));
}

//----------------------------------------------------------------------------------------
template<class T>
void OpenGLStateContainer<T>::SetStateValueInternal(StateValueId stateId, const V2UInt16& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetStateValueInternal(stateId, arrayIndices, arrayIndexCount, new StateValueInfo<std::remove_const<std::remove_reference<decltype(value)>::type>::type>(stateId, value, arrayIndices, arrayIndexCount));
}

//----------------------------------------------------------------------------------------
template<class T>
void OpenGLStateContainer<T>::SetStateValueInternal(StateValueId stateId, const V2UInt32& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetStateValueInternal(stateId, arrayIndices, arrayIndexCount, new StateValueInfo<std::remove_const<std::remove_reference<decltype(value)>::type>::type>(stateId, value, arrayIndices, arrayIndexCount));
}

//----------------------------------------------------------------------------------------
template<class T>
void OpenGLStateContainer<T>::SetStateValueInternal(StateValueId stateId, const V2Float32& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetStateValueInternal(stateId, arrayIndices, arrayIndexCount, new StateValueInfo<std::remove_const<std::remove_reference<decltype(value)>::type>::type>(stateId, value, arrayIndices, arrayIndexCount));
}

//----------------------------------------------------------------------------------------
template<class T>
void OpenGLStateContainer<T>::SetStateValueInternal(StateValueId stateId, const V2Float64& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetStateValueInternal(stateId, arrayIndices, arrayIndexCount, new StateValueInfo<std::remove_const<std::remove_reference<decltype(value)>::type>::type>(stateId, value, arrayIndices, arrayIndexCount));
}

//----------------------------------------------------------------------------------------
template<class T>
void OpenGLStateContainer<T>::SetStateValueInternal(StateValueId stateId, const V3Int8& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetStateValueInternal(stateId, arrayIndices, arrayIndexCount, new StateValueInfo<std::remove_const<std::remove_reference<decltype(value)>::type>::type>(stateId, value, arrayIndices, arrayIndexCount));
}

//----------------------------------------------------------------------------------------
template<class T>
void OpenGLStateContainer<T>::SetStateValueInternal(StateValueId stateId, const V3Int16& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetStateValueInternal(stateId, arrayIndices, arrayIndexCount, new StateValueInfo<std::remove_const<std::remove_reference<decltype(value)>::type>::type>(stateId, value, arrayIndices, arrayIndexCount));
}

//----------------------------------------------------------------------------------------
template<class T>
void OpenGLStateContainer<T>::SetStateValueInternal(StateValueId stateId, const V3Int32& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetStateValueInternal(stateId, arrayIndices, arrayIndexCount, new StateValueInfo<std::remove_const<std::remove_reference<decltype(value)>::type>::type>(stateId, value, arrayIndices, arrayIndexCount));
}

//----------------------------------------------------------------------------------------
template<class T>
void OpenGLStateContainer<T>::SetStateValueInternal(StateValueId stateId, const V3UInt8& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetStateValueInternal(stateId, arrayIndices, arrayIndexCount, new StateValueInfo<std::remove_const<std::remove_reference<decltype(value)>::type>::type>(stateId, value, arrayIndices, arrayIndexCount));
}

//----------------------------------------------------------------------------------------
template<class T>
void OpenGLStateContainer<T>::SetStateValueInternal(StateValueId stateId, const V3UInt16& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetStateValueInternal(stateId, arrayIndices, arrayIndexCount, new StateValueInfo<std::remove_const<std::remove_reference<decltype(value)>::type>::type>(stateId, value, arrayIndices, arrayIndexCount));
}

//----------------------------------------------------------------------------------------
template<class T>
void OpenGLStateContainer<T>::SetStateValueInternal(StateValueId stateId, const V3UInt32& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetStateValueInternal(stateId, arrayIndices, arrayIndexCount, new StateValueInfo<std::remove_const<std::remove_reference<decltype(value)>::type>::type>(stateId, value, arrayIndices, arrayIndexCount));
}

//----------------------------------------------------------------------------------------
template<class T>
void OpenGLStateContainer<T>::SetStateValueInternal(StateValueId stateId, const V3Float32& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetStateValueInternal(stateId, arrayIndices, arrayIndexCount, new StateValueInfo<std::remove_const<std::remove_reference<decltype(value)>::type>::type>(stateId, value, arrayIndices, arrayIndexCount));
}

//----------------------------------------------------------------------------------------
template<class T>
void OpenGLStateContainer<T>::SetStateValueInternal(StateValueId stateId, const V3Float64& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetStateValueInternal(stateId, arrayIndices, arrayIndexCount, new StateValueInfo<std::remove_const<std::remove_reference<decltype(value)>::type>::type>(stateId, value, arrayIndices, arrayIndexCount));
}

//----------------------------------------------------------------------------------------
template<class T>
void OpenGLStateContainer<T>::SetStateValueInternal(StateValueId stateId, const V4Int8& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetStateValueInternal(stateId, arrayIndices, arrayIndexCount, new StateValueInfo<std::remove_const<std::remove_reference<decltype(value)>::type>::type>(stateId, value, arrayIndices, arrayIndexCount));
}

//----------------------------------------------------------------------------------------
template<class T>
void OpenGLStateContainer<T>::SetStateValueInternal(StateValueId stateId, const V4Int16& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetStateValueInternal(stateId, arrayIndices, arrayIndexCount, new StateValueInfo<std::remove_const<std::remove_reference<decltype(value)>::type>::type>(stateId, value, arrayIndices, arrayIndexCount));
}

//----------------------------------------------------------------------------------------
template<class T>
void OpenGLStateContainer<T>::SetStateValueInternal(StateValueId stateId, const V4Int32& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetStateValueInternal(stateId, arrayIndices, arrayIndexCount, new StateValueInfo<std::remove_const<std::remove_reference<decltype(value)>::type>::type>(stateId, value, arrayIndices, arrayIndexCount));
}

//----------------------------------------------------------------------------------------
template<class T>
void OpenGLStateContainer<T>::SetStateValueInternal(StateValueId stateId, const V4UInt8& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetStateValueInternal(stateId, arrayIndices, arrayIndexCount, new StateValueInfo<std::remove_const<std::remove_reference<decltype(value)>::type>::type>(stateId, value, arrayIndices, arrayIndexCount));
}

//----------------------------------------------------------------------------------------
template<class T>
void OpenGLStateContainer<T>::SetStateValueInternal(StateValueId stateId, const V4UInt16& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetStateValueInternal(stateId, arrayIndices, arrayIndexCount, new StateValueInfo<std::remove_const<std::remove_reference<decltype(value)>::type>::type>(stateId, value, arrayIndices, arrayIndexCount));
}

//----------------------------------------------------------------------------------------
template<class T>
void OpenGLStateContainer<T>::SetStateValueInternal(StateValueId stateId, const V4UInt32& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetStateValueInternal(stateId, arrayIndices, arrayIndexCount, new StateValueInfo<std::remove_const<std::remove_reference<decltype(value)>::type>::type>(stateId, value, arrayIndices, arrayIndexCount));
}

//----------------------------------------------------------------------------------------
template<class T>
void OpenGLStateContainer<T>::SetStateValueInternal(StateValueId stateId, const V4Float32& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetStateValueInternal(stateId, arrayIndices, arrayIndexCount, new StateValueInfo<std::remove_const<std::remove_reference<decltype(value)>::type>::type>(stateId, value, arrayIndices, arrayIndexCount));
}

//----------------------------------------------------------------------------------------
template<class T>
void OpenGLStateContainer<T>::SetStateValueInternal(StateValueId stateId, const V4Float64& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetStateValueInternal(stateId, arrayIndices, arrayIndexCount, new StateValueInfo<std::remove_const<std::remove_reference<decltype(value)>::type>::type>(stateId, value, arrayIndices, arrayIndexCount));
}

//----------------------------------------------------------------------------------------
template<class T>
void OpenGLStateContainer<T>::SetStateValueInternal(StateValueId stateId, const M2Float32& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetStateValueInternal(stateId, arrayIndices, arrayIndexCount, new StateValueInfo<std::remove_const<std::remove_reference<decltype(value)>::type>::type>(stateId, value, arrayIndices, arrayIndexCount));
}

//----------------------------------------------------------------------------------------
template<class T>
void OpenGLStateContainer<T>::SetStateValueInternal(StateValueId stateId, const M3Float32& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetStateValueInternal(stateId, arrayIndices, arrayIndexCount, new StateValueInfo<std::remove_const<std::remove_reference<decltype(value)>::type>::type>(stateId, value, arrayIndices, arrayIndexCount));
}

//----------------------------------------------------------------------------------------
template<class T>
void OpenGLStateContainer<T>::SetStateValueInternal(StateValueId stateId, const M4Float32& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetStateValueInternal(stateId, arrayIndices, arrayIndexCount, new StateValueInfo<std::remove_const<std::remove_reference<decltype(value)>::type>::type>(stateId, value, arrayIndices, arrayIndexCount));
}

//----------------------------------------------------------------------------------------
template<class T>
void OpenGLStateContainer<T>::SetStateValueInternal(StateValueId stateId, const size_t* arrayIndices, size_t arrayIndexCount, IStateValueInfo* value)
{
	// Validate the supplied state ID
	if (stateId == StateValueId::Null)
	{
		_log->Warning("Attempted to set state value with ID \"{0}\"", stateId);
		return;
	}

	// Since we're updating the bound state for this node, flag that the draw state is no longer current.
	OpenGLNode<T>::FlagDrawStateNotCurrent();

	// Add a value entry for this state, removing any existing value that may already have been set for the target state
	// ID.
	for (size_t i = 0; i < _buildState.valueEntries.size(); ++i)
	{
		IStateValueInfo* entry = _buildState.valueEntries[i];
		if ((entry->GetAttributeId() == stateId) && (entry->GetArrayIndexCount() == arrayIndexCount))
		{
			bool arrayIndicesMatch = true;
			auto entryArrayIndices = entry->GetArrayIndices();
			size_t arrayIndex = 0;
			while (arrayIndex < entry->GetArrayIndexCount())
			{
				if (entryArrayIndices[arrayIndex] != arrayIndices[arrayIndex])
				{
					arrayIndicesMatch = false;
					break;
				}
				++arrayIndex;
			}
			if (arrayIndicesMatch)
			{
				_buildState.valueEntriesToDelete.push_back(entry);
				_buildState.valueEntries[i] = value;
				return;
			}
		}
	}
	_buildState.valueEntries.push_back(value);
}

//----------------------------------------------------------------------------------------
template<class T>
void OpenGLStateContainer<T>::ResetStateValue(StateValueId stateId, const size_t* arrayIndices, size_t arrayIndexCount)
{
	// Validate the supplied state ID
	if (stateId == StateValueId::Null)
	{
		_log->Warning("Attempted to reset state value with ID \"{0}\"", stateId);
		return;
	}

	// Since we're updating the bound state for this node, flag that the draw state is no longer current.
	OpenGLNode<T>::FlagDrawStateNotCurrent();

	// Remove any existing value that may have been set for the target state ID
	for (size_t i = 0; i < _buildState.valueEntries.size(); ++i)
	{
		IStateValueInfo* entry = _buildState.valueEntries[i];
		if ((entry->GetAttributeId() == stateId) && (entry->GetArrayIndexCount() == arrayIndexCount))
		{
			bool arrayIndicesMatch = true;
			auto entryArrayIndices = entry->GetArrayIndices();
			size_t arrayIndex = 0;
			while (arrayIndex < entry->GetArrayIndexCount())
			{
				if (entryArrayIndices[arrayIndex] != arrayIndices[arrayIndex])
				{
					arrayIndicesMatch = false;
					break;
				}
				++arrayIndex;
			}
			if (arrayIndicesMatch)
			{
				_buildState.valueEntriesToDelete.push_back(entry);
				_buildState.valueEntries.erase(_buildState.valueEntries.begin() + i);
				return;
			}
		}
	}
}

//----------------------------------------------------------------------------------------
// Build state methods
//----------------------------------------------------------------------------------------
template<class T>
void OpenGLStateContainer<T>::MigrateBuildStateToDrawState()
{
	// Migrate our build state
	DeleteRemovedEntries(_drawState);
	_drawState = _buildState;
	_buildState.valueEntriesToDelete.clear();
	_buildState.textureEntriesToDelete.clear();
	_buildState.stateBufferEntriesToDelete.clear();
	_buildState.resourceBufferEntriesToDelete.clear();
}

//----------------------------------------------------------------------------------------
template<class T>
void OpenGLStateContainer<T>::DeleteRemovedEntries(MutableState& targetState)
{
	for (size_t i = 0; i < targetState.valueEntriesToDelete.size(); ++i)
	{
		delete targetState.valueEntriesToDelete[i];
	}
	targetState.valueEntriesToDelete.clear();
	for (size_t i = 0; i < targetState.textureEntriesToDelete.size(); ++i)
	{
		delete targetState.textureEntriesToDelete[i];
	}
	targetState.textureEntriesToDelete.clear();
	for (size_t i = 0; i < targetState.stateBufferEntriesToDelete.size(); ++i)
	{
		delete targetState.stateBufferEntriesToDelete[i];
	}
	targetState.stateBufferEntriesToDelete.clear();
	for (size_t i = 0; i < targetState.resourceBufferEntriesToDelete.size(); ++i)
	{
		delete targetState.resourceBufferEntriesToDelete[i];
	}
	targetState.resourceBufferEntriesToDelete.clear();
}

//----------------------------------------------------------------------------------------
template<class T>
const std::vector<IStateValueInfo*>& OpenGLStateContainer<T>::GetValueEntries() const
{
	return _drawState.valueEntries;
}

//----------------------------------------------------------------------------------------
template<class T>
const std::vector<ITextureBindingInfo*>& OpenGLStateContainer<T>::GetTextureEntries() const
{
	return _drawState.textureEntries;
}

//----------------------------------------------------------------------------------------
template<class T>
const std::vector<StateBufferBindingInfo*>& OpenGLStateContainer<T>::GetStateBufferEntries() const
{
	return _drawState.stateBufferEntries;
}

//----------------------------------------------------------------------------------------
template<class T>
const std::vector<ResourceArrayBindingInfo*>& OpenGLStateContainer<T>::GetResourceBufferEntries() const
{
	return _drawState.resourceBufferEntries;
}

} // namespace cobalt::graphics
