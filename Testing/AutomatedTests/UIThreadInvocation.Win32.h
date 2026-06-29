// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include "IThreadInvocation.h"
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
namespace cobalt::graphics::testing {

class Win32MessageThreadInvocation : public IThreadInvocation
{
public:
	// Constructors
	Win32MessageThreadInvocation(std::thread::id threadId, HWND targetWindow, UINT invokeMessageId);

protected:
	// Invocation methods
	std::thread::id GetTargetThreadId() const final;
	void PushPendingCallback(std::function<void()> callback) final;

private:
	std::thread::id _threadId;
	HWND _targetWindow;
	UINT _invokeMessageId;
};

} // namespace cobalt::graphics::testing
