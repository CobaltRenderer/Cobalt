// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include <Internal/RendererSupport/VariantHasType.h>
namespace cobalt::graphics {

//----------------------------------------------------------------------------------------
// Object modification/deletion methods
//----------------------------------------------------------------------------------------
template<class T>
void Direct3DRenderer::FlagObjectModified(T* object)
{
	std::lock_guard<std::mutex> lock(_buildStateMutex);
	_state[_buildIndex].migrateStatePendingObjects.push_back(object);
	// RJS - As of 2019-10-01, clang-tidy gets confused by this if statement, even with C++17 support enabled.
#ifndef __clang_analyzer__
	if constexpr (variant_has_type<T*, MutableState::BufferUpdateObjectTypes>::value)
	{
		_state[_buildIndex].bufferUpdatePendingObjects.push_back(object);
	}
	if constexpr (variant_has_type<T*, MutableState::BufferTransferObjectTypes>::value)
	{
		_state[_buildIndex].bufferTransferPendingObjects.push_back(object);
	}
#endif
}

//----------------------------------------------------------------------------------------
template<class T>
void Direct3DRenderer::DeleteObject(T* object)
{
	std::lock_guard<std::mutex> lock(_buildStateMutex);
	_state[_buildIndex].deletePendingObjects.push_back(object);
}

} // namespace cobalt::graphics
