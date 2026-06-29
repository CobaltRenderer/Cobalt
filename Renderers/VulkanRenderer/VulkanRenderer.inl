// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include <Internal/RendererSupport/VariantHasType.h>
namespace cobalt::graphics {

//----------------------------------------------------------------------------------------
// Object modification/deletion methods
//----------------------------------------------------------------------------------------
template<class T>
void VulkanRenderer::FlagObjectModified(T* object)
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
void VulkanRenderer::DeleteObject(T* object)
{
	std::lock_guard<std::mutex> lock(_buildStateMutex);
	_state[_buildIndex].deletePendingObjects.push_back(object);
}

//----------------------------------------------------------------------------------------
// Queue methods
//----------------------------------------------------------------------------------------
inline bool VulkanRenderer::IsTransferQueueSharedWithGraphics() const
{
	return (_transferQueueFamilyIndex == _graphicsQueueFamilyIndex);
}

//----------------------------------------------------------------------------------------
template<class T>
void VulkanRenderer::ScheduleGraphicsQueueAcquireOperation(T* object)
{
	// Add the target buffer to the list of buffers which need to be transitioned back to the graphics queue before the
	// next frame begins. If the target buffer is already in the queue, we ensure it is only added once. Note that while
	// we could use an unordered_set here, we prefer a vector for performance reasons, so there's no need for hashing or
	// to reallocate memory, since the vector will naturally size itself to a suitable length.
	std::scoped_lock<std::mutex> lock(_pendingGraphicsQueueTransferMutex);
	for (const auto& entry : _pendingGraphicsQueueAcquireOperations)
	{
		if (std::visit([](auto* entryObject) { return static_cast<void*>(entryObject); }, entry) == static_cast<void*>(object))
		{
			return;
		}
	}
	_pendingGraphicsQueueAcquireOperations.push_back(object);
}

//----------------------------------------------------------------------------------------
template<class T>
void VulkanRenderer::ScheduleGraphicsQueueReleaseOperation(T* object)
{
	// Add the target buffer to the list of buffers which need to be released from the graphics queue after the current
	// frame ends. If the target buffer is already in the queue, we ensure it is only added once. Note that while we
	// could use an unordered_set here, we prefer a vector for performance reasons, so there's no need for hashing or
	// to reallocate memory, since the vector will naturally size itself to a suitable length.
	std::scoped_lock<std::mutex> lock(_pendingGraphicsQueueTransferMutex);
	for (const auto& entry : _pendingGraphicsQueueReleaseOperations)
	{
		if (std::visit([](auto* entryObject) { return static_cast<void*>(entryObject); }, entry) == static_cast<void*>(object))
		{
			return;
		}
	}
	_pendingGraphicsQueueReleaseOperations.push_back(object);
}

//----------------------------------------------------------------------------------------
template<class T>
bool VulkanRenderer::CancelPendingGraphicsQueueAcquireOperation(T* object)
{
	// In the unlikely event a buffer has been scheduled for a subsequent transfer after all previous transfers were
	// completed, but before the queue transition occurred, we search for the entry in the queue and remove it. Using
	// erase on the vector isn't ideal, but this is a rare case, and the vector isn't expected to be very large in real
	// world usage, so it's acceptable here, and preferrable to changing the container type to a list or set, which is
	// less performant on the expected path.
	std::scoped_lock<std::mutex> lock(_pendingGraphicsQueueTransferMutex);
	for (auto entryIterator = _pendingGraphicsQueueAcquireOperations.begin(); entryIterator != _pendingGraphicsQueueAcquireOperations.end(); ++entryIterator)
	{
		if (std::visit([](auto* entryObject) { return static_cast<void*>(entryObject); }, *entryIterator) == static_cast<void*>(object))
		{
			_pendingGraphicsQueueAcquireOperations.erase(entryIterator);
			return true;
		}
	}
	return false;
}

} // namespace cobalt::graphics
