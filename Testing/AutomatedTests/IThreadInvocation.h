// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include <functional>
#include <thread>
namespace cobalt::graphics::testing {

class IThreadInvocation
{
public:
	// Invocation methods
	template<class T>
	void InvokeAsync(const T& command);
	template<class T>
	auto InvokeSync(const T& command) -> decltype(command());
	template<class T>
	void InvokeSyncVoidReturn(const T& command);

protected:
	// Invocation methods
	virtual std::thread::id GetTargetThreadId() const = 0;
	virtual void PushPendingCallback(std::function<void()> callback) = 0;
};

} // namespace cobalt::graphics::testing
#include "IThreadInvocation.inl"
