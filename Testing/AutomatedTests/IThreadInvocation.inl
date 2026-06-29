// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
// Disable compiler warnings generated from Microsoft headers under VS2017
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4355)
#endif
#include <condition_variable>
#include <future>
#include <mutex>
#ifdef _MSC_VER
#pragma warning(pop)
#endif
namespace cobalt::graphics::testing {

//----------------------------------------------------------------------------------------
// Invocation methods
//----------------------------------------------------------------------------------------
template<class T>
void IThreadInvocation::InvokeAsync(const T& command)
{
	PushPendingCallback(std::move(command));
}

//----------------------------------------------------------------------------------------
template<class T>
auto IThreadInvocation::InvokeSync(const T& command) -> decltype(command())
{
	if (GetTargetThreadId() != std::this_thread::get_id())
	{
		using RetType = decltype(InvokeSync(command));
		std::promise<RetType> promise;
		std::future<RetType> future = promise.get_future();
		InvokeAsync([&] { promise.set_value(std::move(command())); });
		return future.get();
	}

	return command();
}

//----------------------------------------------------------------------------------------
template<class T>
void IThreadInvocation::InvokeSyncVoidReturn(const T& command)
{
	if (GetTargetThreadId() != std::this_thread::get_id())
	{
		std::mutex eventProcessedMutex;
		std::unique_lock<std::mutex> lock(eventProcessedMutex);
		std::condition_variable_any notifyEventProcessed;
		bool eventProcessed = false;
		InvokeAsync([&] {
			command();
			std::unique_lock<std::mutex> lock2(eventProcessedMutex);
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

} // namespace cobalt::graphics::testing
