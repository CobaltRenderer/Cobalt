// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "UIThreadInvocation.Win32.h"
namespace cobalt::graphics::testing {

//----------------------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------------------
Win32MessageThreadInvocation::Win32MessageThreadInvocation(std::thread::id threadId, HWND targetWindow, UINT invokeMessageId)
: _threadId(threadId), _targetWindow(targetWindow), _invokeMessageId(invokeMessageId)
{}

//----------------------------------------------------------------------------------------
// Invocation methods
//----------------------------------------------------------------------------------------
std::thread::id Win32MessageThreadInvocation::GetTargetThreadId() const
{
	return _threadId;
}

//----------------------------------------------------------------------------------------
void Win32MessageThreadInvocation::PushPendingCallback(std::function<void()> callback)
{
	PostMessage(_targetWindow, _invokeMessageId, 0, LPARAM(new std::function<void()>(std::move(callback))));
}

} // namespace cobalt::graphics::testing
