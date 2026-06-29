// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
// Disable compiler warnings generated from Microsoft headers under VS2017
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4355)
#endif
#include <future>
#ifdef _MSC_VER
#pragma warning(pop)
#endif
#include <Internal/RendererSupport/VariantHasType.h>
namespace cobalt::graphics {

//----------------------------------------------------------------------------------------
// Object modification/deletion methods
//----------------------------------------------------------------------------------------
template<class T>
void OpenGLRenderer::FlagObjectModified(T* object)
{
	std::lock_guard<std::mutex> lock(_buildStateMutex);
	_state[_buildIndex].migrateStatePendingObjects.push_back(object);
	// RJS - As of 2019-10-01, clang-tidy gets confused by this if statement, even with C++17 support enabled.
#ifndef __clang_analyzer__
	if constexpr (variant_has_type<T*, MutableState::BufferUpdateObjectTypes>::value)
	{
		_state[_buildIndex].bufferUpdatePendingObjects.push_back(object);
	}
#ifdef GL_VERSION_4_3
	if constexpr (variant_has_type<T*, MutableState::BufferTransferObjectTypes>::value)
	{
		_state[_buildIndex].bufferTransferPendingObjects.push_back(object);
	}
#endif
#endif
}

//----------------------------------------------------------------------------------------
template<class T>
void OpenGLRenderer::DeleteObject(T* object)
{
	std::lock_guard<std::mutex> lock(_buildStateMutex);
	_state[_buildIndex].deletePendingObjects.push_back(object);
}

//----------------------------------------------------------------------------------------
// Render thread invocation methods
//----------------------------------------------------------------------------------------
template<class T>
void OpenGLRenderer::RenderThreadInvokeAsync(const T& command)
{
	std::unique_lock<std::mutex> lock(_renderThreadMutex);
	_pendingInvocations.push(std::move(command));
	_notifyRenderThreadTaskPending.notify_all();
}

//----------------------------------------------------------------------------------------
template<class T>
auto OpenGLRenderer::RenderThreadInvokeSync(const T& command) -> decltype(command())
{
	if (_renderThreadID != std::this_thread::get_id())
	{
		using RetType = decltype(RenderThreadInvokeSync(command));
		std::promise<RetType> promise;
		std::future<RetType> future = promise.get_future();
		RenderThreadInvokeAsync([&] { promise.set_value(std::move(command())); });
		return future.get();
	}

	return command();
}

//----------------------------------------------------------------------------------------
template<class T>
void OpenGLRenderer::RenderThreadInvokeSyncVoidReturn(const T& command)
{
	if (_renderThreadID != std::this_thread::get_id())
	{
		std::mutex eventProcessedMutex;
		std::unique_lock<std::mutex> lock(eventProcessedMutex);
		std::condition_variable_any notifyEventProcessed;
		bool eventProcessed = false;
		RenderThreadInvokeAsync([&] {
			command();
			std::unique_lock<std::mutex> lock(eventProcessedMutex);
			eventProcessed = true;
			notifyEventProcessed.notify_all();
		});
		while (!eventProcessed)
		{
			notifyEventProcessed.wait(lock);
		}
		return;
	}

	command();
}

} // namespace cobalt::graphics
