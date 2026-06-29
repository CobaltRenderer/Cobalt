// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "Direct3DTextureBuffer1D.h"
#include "Direct3DTextureBuffer1DArray.h"
#include "Direct3DTextureBuffer2D.h"
#include "Direct3DTextureBuffer2DArray.h"
#include "Direct3DTextureBuffer3D.h"
#include "Direct3DTextureBufferCube.h"
#include "Direct3DTextureBufferCubeArray.h"
#include "Direct3DTextureSampler1D.h"
#include "Direct3DTextureSampler1DArray.h"
#include "Direct3DTextureSampler2D.h"
#include "Direct3DTextureSampler2DArray.h"
#include "Direct3DTextureSampler3D.h"
#include "Direct3DTextureSamplerCube.h"
#include "Direct3DTextureSamplerCubeArray.h"
#include <Internal/RendererSupport/KnownDynamicCast.h>
#include <type_traits>
namespace cobalt::graphics {

//----------------------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------------------
template<class T>
Direct3DStateContainer<T>::Direct3DStateContainer(cobalt::logging::ILogger* log)
: _log(log)
{}

//----------------------------------------------------------------------------------------
template<class T>
Direct3DStateContainer<T>::~Direct3DStateContainer()
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
	for (size_t i = 0; i < _buildState.samplerEntries.size(); ++i)
	{
		delete _buildState.samplerEntries[i];
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
void Direct3DStateContainer<T>::BindTextureWithCombinedSampler(TextureId textureId, ITextureBuffer1D* texture, ITextureSampler1D* sampler)
{
	BindTextureWithCombinedSamplerInternal(textureId, KnownDynamicCast<Direct3DTextureBuffer1D*>(texture), KnownDynamicCast<Direct3DTextureSampler1D*>(sampler));
}

//----------------------------------------------------------------------------------------
template<class T>
void Direct3DStateContainer<T>::BindTextureWithCombinedSampler(TextureId textureId, ITextureBuffer2D* texture, ITextureSampler2D* sampler)
{
	BindTextureWithCombinedSamplerInternal(textureId, KnownDynamicCast<Direct3DTextureBuffer2D*>(texture), KnownDynamicCast<Direct3DTextureSampler2D*>(sampler));
}

//----------------------------------------------------------------------------------------
template<class T>
void Direct3DStateContainer<T>::BindTextureWithCombinedSampler(TextureId textureId, ITextureBuffer3D* texture, ITextureSampler3D* sampler)
{
	BindTextureWithCombinedSamplerInternal(textureId, KnownDynamicCast<Direct3DTextureBuffer3D*>(texture), KnownDynamicCast<Direct3DTextureSampler3D*>(sampler));
}

//----------------------------------------------------------------------------------------
template<class T>
void Direct3DStateContainer<T>::BindTextureWithCombinedSampler(TextureId textureId, ITextureBufferCube* texture, ITextureSamplerCube* sampler)
{
	BindTextureWithCombinedSamplerInternal(textureId, KnownDynamicCast<Direct3DTextureBufferCube*>(texture), KnownDynamicCast<Direct3DTextureSamplerCube*>(sampler));
}

//----------------------------------------------------------------------------------------
template<class T>
void Direct3DStateContainer<T>::BindTextureWithCombinedSampler(TextureId textureId, ITextureBuffer1DArray* texture, ITextureSampler1DArray* sampler)
{
	BindTextureWithCombinedSamplerInternal(textureId, KnownDynamicCast<Direct3DTextureBuffer1DArray*>(texture), KnownDynamicCast<Direct3DTextureSampler1DArray*>(sampler));
}

//----------------------------------------------------------------------------------------
template<class T>
void Direct3DStateContainer<T>::BindTextureWithCombinedSampler(TextureId textureId, ITextureBuffer2DArray* texture, ITextureSampler2DArray* sampler)
{
	BindTextureWithCombinedSamplerInternal(textureId, KnownDynamicCast<Direct3DTextureBuffer2DArray*>(texture), KnownDynamicCast<Direct3DTextureSampler2DArray*>(sampler));
}

//----------------------------------------------------------------------------------------
template<class T>
void Direct3DStateContainer<T>::BindTextureWithCombinedSampler(TextureId textureId, ITextureBufferCubeArray* texture, ITextureSamplerCubeArray* sampler)
{
	BindTextureWithCombinedSamplerInternal(textureId, KnownDynamicCast<Direct3DTextureBufferCubeArray*>(texture), KnownDynamicCast<Direct3DTextureSamplerCubeArray*>(sampler));
}

//----------------------------------------------------------------------------------------
template<class T>
template<class TextureType, class SamplerType>
void Direct3DStateContainer<T>::BindTextureWithCombinedSamplerInternal(TextureId textureId, TextureType* texture, SamplerType* sampler)
{
	// Validate the supplied texture ID
	if (textureId == TextureId::Null)
	{
		_log->Warning("Attempted to bind texture with invalid ID \"{0}\"", textureId);
		return;
	}

	// Since we're updating the bound state for this node, flag that the draw state is no longer current.
	Direct3DNode<T>::FlagDrawStateNotCurrent();

	// Add a binding entry for this texture, removing any existing binding that may be in place against the target
	// texture ID.
	auto* value = new TextureBindingWithCombinedSamplerInfo(textureId, texture, sampler);
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
void Direct3DStateContainer<T>::BindTexture(TextureId textureId, ITextureBuffer1D* texture)
{
	BindTextureInternal(textureId, KnownDynamicCast<Direct3DTextureBuffer1D*>(texture));
}

//----------------------------------------------------------------------------------------
template<class T>
void Direct3DStateContainer<T>::BindTexture(TextureId textureId, ITextureBuffer2D* texture)
{
	BindTextureInternal(textureId, KnownDynamicCast<Direct3DTextureBuffer2D*>(texture));
}

//----------------------------------------------------------------------------------------
template<class T>
void Direct3DStateContainer<T>::BindTexture(TextureId textureId, ITextureBuffer3D* texture)
{
	BindTextureInternal(textureId, KnownDynamicCast<Direct3DTextureBuffer3D*>(texture));
}

//----------------------------------------------------------------------------------------
template<class T>
void Direct3DStateContainer<T>::BindTexture(TextureId textureId, ITextureBufferCube* texture)
{
	BindTextureInternal(textureId, KnownDynamicCast<Direct3DTextureBufferCube*>(texture));
}

//----------------------------------------------------------------------------------------
template<class T>
void Direct3DStateContainer<T>::BindTexture(TextureId textureId, ITextureBuffer1DArray* texture)
{
	BindTextureInternal(textureId, KnownDynamicCast<Direct3DTextureBuffer1DArray*>(texture));
}

//----------------------------------------------------------------------------------------
template<class T>
void Direct3DStateContainer<T>::BindTexture(TextureId textureId, ITextureBuffer2DArray* texture)
{
	BindTextureInternal(textureId, KnownDynamicCast<Direct3DTextureBuffer2DArray*>(texture));
}

//----------------------------------------------------------------------------------------
template<class T>
void Direct3DStateContainer<T>::BindTexture(TextureId textureId, ITextureBufferCubeArray* texture)
{
	BindTextureInternal(textureId, KnownDynamicCast<Direct3DTextureBufferCubeArray*>(texture));
}

//----------------------------------------------------------------------------------------
template<class T>
template<class TextureType>
void Direct3DStateContainer<T>::BindTextureInternal(TextureId textureId, TextureType* texture)
{
	// Validate the supplied texture ID
	if (textureId == TextureId::Null)
	{
		_log->Warning("Attempted to bind texture with invalid ID \"{0}\"", textureId);
		return;
	}

	// Since we're updating the bound state for this node, flag that the draw state is no longer current.
	Direct3DNode<T>::FlagDrawStateNotCurrent();

	// Add a binding entry for this texture, removing any existing binding that may be in place against the target
	// texture ID.
	auto* value = new TextureBindingInfo(textureId, texture);
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
void Direct3DStateContainer<T>::UnbindTexture(TextureId textureId)
{
	// Validate the supplied texture ID
	if (textureId == TextureId::Null)
	{
		_log->Warning("Attempted to unbind texture with invalid ID \"{0}\"", textureId);
		return;
	}

	// Since we're updating the bound state for this node, flag that the draw state is no longer current.
	Direct3DNode<T>::FlagDrawStateNotCurrent();

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
void Direct3DStateContainer<T>::BindSampler(SamplerId samplerId, ITextureSampler1D* sampler)
{
	BindSamplerInternal(samplerId, KnownDynamicCast<Direct3DTextureSampler1D*>(sampler));
}

//----------------------------------------------------------------------------------------
template<class T>
void Direct3DStateContainer<T>::BindSampler(SamplerId samplerId, ITextureSampler2D* sampler)
{
	BindSamplerInternal(samplerId, KnownDynamicCast<Direct3DTextureSampler2D*>(sampler));
}

//----------------------------------------------------------------------------------------
template<class T>
void Direct3DStateContainer<T>::BindSampler(SamplerId samplerId, ITextureSampler3D* sampler)
{
	BindSamplerInternal(samplerId, KnownDynamicCast<Direct3DTextureSampler3D*>(sampler));
}

//----------------------------------------------------------------------------------------
template<class T>
void Direct3DStateContainer<T>::BindSampler(SamplerId samplerId, ITextureSamplerCube* sampler)
{
	BindSamplerInternal(samplerId, KnownDynamicCast<Direct3DTextureSamplerCube*>(sampler));
}

//----------------------------------------------------------------------------------------
template<class T>
void Direct3DStateContainer<T>::BindSampler(SamplerId samplerId, ITextureSampler1DArray* sampler)
{
	BindSamplerInternal(samplerId, KnownDynamicCast<Direct3DTextureSampler1DArray*>(sampler));
}

//----------------------------------------------------------------------------------------
template<class T>
void Direct3DStateContainer<T>::BindSampler(SamplerId samplerId, ITextureSampler2DArray* sampler)
{
	BindSamplerInternal(samplerId, KnownDynamicCast<Direct3DTextureSampler2DArray*>(sampler));
}

//----------------------------------------------------------------------------------------
template<class T>
void Direct3DStateContainer<T>::BindSampler(SamplerId samplerId, ITextureSamplerCubeArray* sampler)
{
	BindSamplerInternal(samplerId, KnownDynamicCast<Direct3DTextureSamplerCubeArray*>(sampler));
}

//----------------------------------------------------------------------------------------
template<class T>
template<class SamplerType>
void Direct3DStateContainer<T>::BindSamplerInternal(SamplerId samplerId, SamplerType* sampler)
{
	// Validate the supplied sampler ID
	if (samplerId == SamplerId::Null)
	{
		_log->Warning("Attempted to bind sampler with invalid ID \"{0}\"", samplerId);
		return;
	}

	// Since we're updating the bound state for this node, flag that the draw state is no longer current.
	Direct3DNode<T>::FlagDrawStateNotCurrent();

	// Add a binding entry for this sampler, removing any existing binding that may be in place against the target
	// sampler ID.
	auto* value = new SamplerBindingInfo(samplerId, sampler);
	for (size_t i = 0; i < _buildState.samplerEntries.size(); ++i)
	{
		ISamplerBindingInfo* bindingInfo = _buildState.samplerEntries[i];
		if (bindingInfo->GetSamplerId() == samplerId)
		{
			_buildState.samplerEntriesToDelete.push_back(bindingInfo);
			_buildState.samplerEntries[i] = value;
			return;
		}
	}
	_buildState.samplerEntries.push_back(value);
}

//----------------------------------------------------------------------------------------
template<class T>
void Direct3DStateContainer<T>::UnbindSampler(SamplerId samplerId)
{
	// Validate the supplied texture ID
	if (samplerId == SamplerId::Null)
	{
		_log->Warning("Attempted to unbind sampler with invalid ID \"{0}\"", samplerId);
		return;
	}

	// Since we're updating the bound state for this node, flag that the draw state is no longer current.
	Direct3DNode<T>::FlagDrawStateNotCurrent();

	// Remove any existing binding that may be in place against the target sampler ID
	for (size_t i = 0; i < _buildState.samplerEntries.size(); ++i)
	{
		ISamplerBindingInfo* bindingInfo = _buildState.samplerEntries[i];
		if (bindingInfo->GetSamplerId() == samplerId)
		{
			_buildState.samplerEntriesToDelete.push_back(bindingInfo);
			_buildState.samplerEntries.erase(_buildState.samplerEntries.begin() + i);
			return;
		}
	}
}

//----------------------------------------------------------------------------------------
template<class T>
void Direct3DStateContainer<T>::BindStateBuffer(StateBufferId stateBufferId, IStateBuffer* stateBuffer, uint32_t stateBufferPageNo)
{
	// Validate the supplied state buffer ID
	if (stateBufferId == graphics::StateBufferId::Null)
	{
		_log->Warning("Attempted to bind state buffer with ID \"{0}\"", stateBufferId);
		return;
	}

	// Since we're updating the bound state for this node, flag that the draw state is no longer current.
	Direct3DNode<T>::FlagDrawStateNotCurrent();

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
void Direct3DStateContainer<T>::UnbindStateBuffer(StateBufferId stateBufferId)
{
	// Validate the supplied state buffer ID
	if (stateBufferId == graphics::StateBufferId::Null)
	{
		_log->Warning("Attempted to unbind state buffer with ID \"{0}\"", stateBufferId);
		return;
	}

	// Since we're updating the bound state for this node, flag that the draw state is no longer current.
	Direct3DNode<T>::FlagDrawStateNotCurrent();

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
void Direct3DStateContainer<T>::BindResourceArray(ResourceArrayId resourceArrayId, IDataArray* resourceBuffer, bool resetCounter)
{
	// Validate the supplied data array ID
	if (resourceArrayId == ResourceArrayId::Null)
	{
		_log->Warning("Attempted to bind resource array with ID \"{0}\"", resourceArrayId);
		return;
	}

	// Since we're updating the bound state for this node, flag that the draw state is no longer current.
	Direct3DNode<T>::FlagDrawStateNotCurrent();

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
void Direct3DStateContainer<T>::BindResourceArray(ResourceArrayId resourceArrayId, ITexelArray* resourceBuffer)
{
	// Validate the supplied data array ID
	if (resourceArrayId == ResourceArrayId::Null)
	{
		_log->Warning("Attempted to bind resource array with ID \"{0}\"", resourceArrayId);
		return;
	}

	// Since we're updating the bound state for this node, flag that the draw state is no longer current.
	Direct3DNode<T>::FlagDrawStateNotCurrent();

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
void Direct3DStateContainer<T>::UnbindResourceArray(ResourceArrayId resourceArrayId)
{
	// Validate the supplied data array ID
	if (resourceArrayId == ResourceArrayId::Null)
	{
		_log->Warning("Attempted to unbind resource array with ID \"{0}\"", resourceArrayId);
		return;
	}

	// Since we're updating the bound state for this node, flag that the draw state is no longer current.
	Direct3DNode<T>::FlagDrawStateNotCurrent();

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
void Direct3DStateContainer<T>::SetStateValueInternal(StateValueId stateId, bool value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetStateValueInternal(stateId, arrayIndices, arrayIndexCount, new StateValueInfo<std::remove_const<std::remove_reference<decltype(value)>::type>::type>(stateId, value, arrayIndices, arrayIndexCount));
}

//----------------------------------------------------------------------------------------
template<class T>
void Direct3DStateContainer<T>::SetStateValueInternal(StateValueId stateId, const V1Int8& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetStateValueInternal(stateId, arrayIndices, arrayIndexCount, new StateValueInfo<std::remove_const<std::remove_reference<decltype(value)>::type>::type>(stateId, value, arrayIndices, arrayIndexCount));
}

//----------------------------------------------------------------------------------------
template<class T>
void Direct3DStateContainer<T>::SetStateValueInternal(StateValueId stateId, const V1Int16& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetStateValueInternal(stateId, arrayIndices, arrayIndexCount, new StateValueInfo<std::remove_const<std::remove_reference<decltype(value)>::type>::type>(stateId, value, arrayIndices, arrayIndexCount));
}

//----------------------------------------------------------------------------------------
template<class T>
void Direct3DStateContainer<T>::SetStateValueInternal(StateValueId stateId, const V1Int32& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetStateValueInternal(stateId, arrayIndices, arrayIndexCount, new StateValueInfo<std::remove_const<std::remove_reference<decltype(value)>::type>::type>(stateId, value, arrayIndices, arrayIndexCount));
}

//----------------------------------------------------------------------------------------
template<class T>
void Direct3DStateContainer<T>::SetStateValueInternal(StateValueId stateId, const V1UInt8& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetStateValueInternal(stateId, arrayIndices, arrayIndexCount, new StateValueInfo<std::remove_const<std::remove_reference<decltype(value)>::type>::type>(stateId, value, arrayIndices, arrayIndexCount));
}

//----------------------------------------------------------------------------------------
template<class T>
void Direct3DStateContainer<T>::SetStateValueInternal(StateValueId stateId, const V1UInt16& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetStateValueInternal(stateId, arrayIndices, arrayIndexCount, new StateValueInfo<std::remove_const<std::remove_reference<decltype(value)>::type>::type>(stateId, value, arrayIndices, arrayIndexCount));
}

//----------------------------------------------------------------------------------------
template<class T>
void Direct3DStateContainer<T>::SetStateValueInternal(StateValueId stateId, const V1UInt32& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetStateValueInternal(stateId, arrayIndices, arrayIndexCount, new StateValueInfo<std::remove_const<std::remove_reference<decltype(value)>::type>::type>(stateId, value, arrayIndices, arrayIndexCount));
}

//----------------------------------------------------------------------------------------
template<class T>
void Direct3DStateContainer<T>::SetStateValueInternal(StateValueId stateId, const V1Float32& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetStateValueInternal(stateId, arrayIndices, arrayIndexCount, new StateValueInfo<std::remove_const<std::remove_reference<decltype(value)>::type>::type>(stateId, value, arrayIndices, arrayIndexCount));
}

//----------------------------------------------------------------------------------------
template<class T>
void Direct3DStateContainer<T>::SetStateValueInternal(StateValueId stateId, const V1Float64& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetStateValueInternal(stateId, arrayIndices, arrayIndexCount, new StateValueInfo<std::remove_const<std::remove_reference<decltype(value)>::type>::type>(stateId, value, arrayIndices, arrayIndexCount));
}

//----------------------------------------------------------------------------------------
template<class T>
void Direct3DStateContainer<T>::SetStateValueInternal(StateValueId stateId, const V2Int8& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetStateValueInternal(stateId, arrayIndices, arrayIndexCount, new StateValueInfo<std::remove_const<std::remove_reference<decltype(value)>::type>::type>(stateId, value, arrayIndices, arrayIndexCount));
}

//----------------------------------------------------------------------------------------
template<class T>
void Direct3DStateContainer<T>::SetStateValueInternal(StateValueId stateId, const V2Int16& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetStateValueInternal(stateId, arrayIndices, arrayIndexCount, new StateValueInfo<std::remove_const<std::remove_reference<decltype(value)>::type>::type>(stateId, value, arrayIndices, arrayIndexCount));
}

//----------------------------------------------------------------------------------------
template<class T>
void Direct3DStateContainer<T>::SetStateValueInternal(StateValueId stateId, const V2Int32& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetStateValueInternal(stateId, arrayIndices, arrayIndexCount, new StateValueInfo<std::remove_const<std::remove_reference<decltype(value)>::type>::type>(stateId, value, arrayIndices, arrayIndexCount));
}

//----------------------------------------------------------------------------------------
template<class T>
void Direct3DStateContainer<T>::SetStateValueInternal(StateValueId stateId, const V2UInt8& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetStateValueInternal(stateId, arrayIndices, arrayIndexCount, new StateValueInfo<std::remove_const<std::remove_reference<decltype(value)>::type>::type>(stateId, value, arrayIndices, arrayIndexCount));
}

//----------------------------------------------------------------------------------------
template<class T>
void Direct3DStateContainer<T>::SetStateValueInternal(StateValueId stateId, const V2UInt16& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetStateValueInternal(stateId, arrayIndices, arrayIndexCount, new StateValueInfo<std::remove_const<std::remove_reference<decltype(value)>::type>::type>(stateId, value, arrayIndices, arrayIndexCount));
}

//----------------------------------------------------------------------------------------
template<class T>
void Direct3DStateContainer<T>::SetStateValueInternal(StateValueId stateId, const V2UInt32& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetStateValueInternal(stateId, arrayIndices, arrayIndexCount, new StateValueInfo<std::remove_const<std::remove_reference<decltype(value)>::type>::type>(stateId, value, arrayIndices, arrayIndexCount));
}

//----------------------------------------------------------------------------------------
template<class T>
void Direct3DStateContainer<T>::SetStateValueInternal(StateValueId stateId, const V2Float32& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetStateValueInternal(stateId, arrayIndices, arrayIndexCount, new StateValueInfo<std::remove_const<std::remove_reference<decltype(value)>::type>::type>(stateId, value, arrayIndices, arrayIndexCount));
}

//----------------------------------------------------------------------------------------
template<class T>
void Direct3DStateContainer<T>::SetStateValueInternal(StateValueId stateId, const V2Float64& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetStateValueInternal(stateId, arrayIndices, arrayIndexCount, new StateValueInfo<std::remove_const<std::remove_reference<decltype(value)>::type>::type>(stateId, value, arrayIndices, arrayIndexCount));
}

//----------------------------------------------------------------------------------------
template<class T>
void Direct3DStateContainer<T>::SetStateValueInternal(StateValueId stateId, const V3Int8& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetStateValueInternal(stateId, arrayIndices, arrayIndexCount, new StateValueInfo<std::remove_const<std::remove_reference<decltype(value)>::type>::type>(stateId, value, arrayIndices, arrayIndexCount));
}

//----------------------------------------------------------------------------------------
template<class T>
void Direct3DStateContainer<T>::SetStateValueInternal(StateValueId stateId, const V3Int16& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetStateValueInternal(stateId, arrayIndices, arrayIndexCount, new StateValueInfo<std::remove_const<std::remove_reference<decltype(value)>::type>::type>(stateId, value, arrayIndices, arrayIndexCount));
}

//----------------------------------------------------------------------------------------
template<class T>
void Direct3DStateContainer<T>::SetStateValueInternal(StateValueId stateId, const V3Int32& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetStateValueInternal(stateId, arrayIndices, arrayIndexCount, new StateValueInfo<std::remove_const<std::remove_reference<decltype(value)>::type>::type>(stateId, value, arrayIndices, arrayIndexCount));
}

//----------------------------------------------------------------------------------------
template<class T>
void Direct3DStateContainer<T>::SetStateValueInternal(StateValueId stateId, const V3UInt8& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetStateValueInternal(stateId, arrayIndices, arrayIndexCount, new StateValueInfo<std::remove_const<std::remove_reference<decltype(value)>::type>::type>(stateId, value, arrayIndices, arrayIndexCount));
}

//----------------------------------------------------------------------------------------
template<class T>
void Direct3DStateContainer<T>::SetStateValueInternal(StateValueId stateId, const V3UInt16& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetStateValueInternal(stateId, arrayIndices, arrayIndexCount, new StateValueInfo<std::remove_const<std::remove_reference<decltype(value)>::type>::type>(stateId, value, arrayIndices, arrayIndexCount));
}

//----------------------------------------------------------------------------------------
template<class T>
void Direct3DStateContainer<T>::SetStateValueInternal(StateValueId stateId, const V3UInt32& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetStateValueInternal(stateId, arrayIndices, arrayIndexCount, new StateValueInfo<std::remove_const<std::remove_reference<decltype(value)>::type>::type>(stateId, value, arrayIndices, arrayIndexCount));
}

//----------------------------------------------------------------------------------------
template<class T>
void Direct3DStateContainer<T>::SetStateValueInternal(StateValueId stateId, const V3Float32& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetStateValueInternal(stateId, arrayIndices, arrayIndexCount, new StateValueInfo<std::remove_const<std::remove_reference<decltype(value)>::type>::type>(stateId, value, arrayIndices, arrayIndexCount));
}

//----------------------------------------------------------------------------------------
template<class T>
void Direct3DStateContainer<T>::SetStateValueInternal(StateValueId stateId, const V3Float64& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetStateValueInternal(stateId, arrayIndices, arrayIndexCount, new StateValueInfo<std::remove_const<std::remove_reference<decltype(value)>::type>::type>(stateId, value, arrayIndices, arrayIndexCount));
}

//----------------------------------------------------------------------------------------
template<class T>
void Direct3DStateContainer<T>::SetStateValueInternal(StateValueId stateId, const V4Int8& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetStateValueInternal(stateId, arrayIndices, arrayIndexCount, new StateValueInfo<std::remove_const<std::remove_reference<decltype(value)>::type>::type>(stateId, value, arrayIndices, arrayIndexCount));
}

//----------------------------------------------------------------------------------------
template<class T>
void Direct3DStateContainer<T>::SetStateValueInternal(StateValueId stateId, const V4Int16& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetStateValueInternal(stateId, arrayIndices, arrayIndexCount, new StateValueInfo<std::remove_const<std::remove_reference<decltype(value)>::type>::type>(stateId, value, arrayIndices, arrayIndexCount));
}

//----------------------------------------------------------------------------------------
template<class T>
void Direct3DStateContainer<T>::SetStateValueInternal(StateValueId stateId, const V4Int32& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetStateValueInternal(stateId, arrayIndices, arrayIndexCount, new StateValueInfo<std::remove_const<std::remove_reference<decltype(value)>::type>::type>(stateId, value, arrayIndices, arrayIndexCount));
}

//----------------------------------------------------------------------------------------
template<class T>
void Direct3DStateContainer<T>::SetStateValueInternal(StateValueId stateId, const V4UInt8& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetStateValueInternal(stateId, arrayIndices, arrayIndexCount, new StateValueInfo<std::remove_const<std::remove_reference<decltype(value)>::type>::type>(stateId, value, arrayIndices, arrayIndexCount));
}

//----------------------------------------------------------------------------------------
template<class T>
void Direct3DStateContainer<T>::SetStateValueInternal(StateValueId stateId, const V4UInt16& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetStateValueInternal(stateId, arrayIndices, arrayIndexCount, new StateValueInfo<std::remove_const<std::remove_reference<decltype(value)>::type>::type>(stateId, value, arrayIndices, arrayIndexCount));
}

//----------------------------------------------------------------------------------------
template<class T>
void Direct3DStateContainer<T>::SetStateValueInternal(StateValueId stateId, const V4UInt32& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetStateValueInternal(stateId, arrayIndices, arrayIndexCount, new StateValueInfo<std::remove_const<std::remove_reference<decltype(value)>::type>::type>(stateId, value, arrayIndices, arrayIndexCount));
}

//----------------------------------------------------------------------------------------
template<class T>
void Direct3DStateContainer<T>::SetStateValueInternal(StateValueId stateId, const V4Float32& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetStateValueInternal(stateId, arrayIndices, arrayIndexCount, new StateValueInfo<std::remove_const<std::remove_reference<decltype(value)>::type>::type>(stateId, value, arrayIndices, arrayIndexCount));
}

//----------------------------------------------------------------------------------------
template<class T>
void Direct3DStateContainer<T>::SetStateValueInternal(StateValueId stateId, const V4Float64& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetStateValueInternal(stateId, arrayIndices, arrayIndexCount, new StateValueInfo<std::remove_const<std::remove_reference<decltype(value)>::type>::type>(stateId, value, arrayIndices, arrayIndexCount));
}

//----------------------------------------------------------------------------------------
template<class T>
void Direct3DStateContainer<T>::SetStateValueInternal(StateValueId stateId, const M2Float32& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetStateValueInternal(stateId, arrayIndices, arrayIndexCount, new StateValueInfo<std::remove_const<std::remove_reference<decltype(value)>::type>::type>(stateId, value, arrayIndices, arrayIndexCount));
}

//----------------------------------------------------------------------------------------
template<class T>
void Direct3DStateContainer<T>::SetStateValueInternal(StateValueId stateId, const M3Float32& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetStateValueInternal(stateId, arrayIndices, arrayIndexCount, new StateValueInfo<std::remove_const<std::remove_reference<decltype(value)>::type>::type>(stateId, value, arrayIndices, arrayIndexCount));
}

//----------------------------------------------------------------------------------------
template<class T>
void Direct3DStateContainer<T>::SetStateValueInternal(StateValueId stateId, const M4Float32& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetStateValueInternal(stateId, arrayIndices, arrayIndexCount, new StateValueInfo<std::remove_const<std::remove_reference<decltype(value)>::type>::type>(stateId, value, arrayIndices, arrayIndexCount));
}

//----------------------------------------------------------------------------------------
template<class T>
void Direct3DStateContainer<T>::SetStateValueInternal(StateValueId stateId, const size_t* arrayIndices, size_t arrayIndexCount, IStateValueInfo* value)
{
	// Validate the supplied state ID
	if (stateId == StateValueId::Null)
	{
		_log->Warning("Attempted to set state value with ID \"{0}\"", stateId);
		return;
	}

	// Since we're updating the bound state for this node, flag that the draw state is no longer current.
	Direct3DNode<T>::FlagDrawStateNotCurrent();

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
void Direct3DStateContainer<T>::ResetStateValue(StateValueId stateId, const size_t* arrayIndices, size_t arrayIndexCount)
{
	// Validate the supplied state ID
	if (stateId == StateValueId::Null)
	{
		_log->Warning("Attempted to clear state value with ID \"{0}\"", stateId);
		return;
	}

	// Since we're updating the bound state for this node, flag that the draw state is no longer current.
	Direct3DNode<T>::FlagDrawStateNotCurrent();

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
void Direct3DStateContainer<T>::MigrateBuildStateToDrawState()
{
	// Migrate our build state
	DeleteRemovedEntries(_drawState);
	_drawState = _buildState;
	_buildState.valueEntriesToDelete.clear();
	_buildState.textureEntriesToDelete.clear();
	_buildState.samplerEntriesToDelete.clear();
	_buildState.stateBufferEntriesToDelete.clear();
	_buildState.resourceBufferEntriesToDelete.clear();
}

//----------------------------------------------------------------------------------------
template<class T>
void Direct3DStateContainer<T>::DeleteRemovedEntries(MutableState& targetState)
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
	for (size_t i = 0; i < targetState.samplerEntriesToDelete.size(); ++i)
	{
		delete targetState.samplerEntriesToDelete[i];
	}
	targetState.samplerEntriesToDelete.clear();
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
const std::vector<IStateValueInfo*>& Direct3DStateContainer<T>::GetValueEntries() const
{
	return _drawState.valueEntries;
}

//----------------------------------------------------------------------------------------
template<class T>
const std::vector<ITextureBindingInfo*>& Direct3DStateContainer<T>::GetTextureEntries() const
{
	return _drawState.textureEntries;
}

//----------------------------------------------------------------------------------------
template<class T>
const std::vector<ISamplerBindingInfo*>& Direct3DStateContainer<T>::GetSamplerEntries() const
{
	return _drawState.samplerEntries;
}

//----------------------------------------------------------------------------------------
template<class T>
const std::vector<StateBufferBindingInfo*>& Direct3DStateContainer<T>::GetStateBufferEntries() const
{
	return _drawState.stateBufferEntries;
}

//----------------------------------------------------------------------------------------
template<class T>
const std::vector<ResourceArrayBindingInfo*>& Direct3DStateContainer<T>::GetResourceBufferEntries() const
{
	return _drawState.resourceBufferEntries;
}

} // namespace cobalt::graphics
