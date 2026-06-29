// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "UIThreadInvocation.Cocoa.h"
#include <dispatch/dispatch.h>
namespace cobalt::graphics::testing {

//----------------------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------------------
MacOSThreadInvocation::MacOSThreadInvocation(std::thread::id uiThreadId)
: _threadId(uiThreadId)
{}

//----------------------------------------------------------------------------------------
// Invocation methods
//----------------------------------------------------------------------------------------
std::thread::id MacOSThreadInvocation::GetTargetThreadId() const
{
    return _threadId;
}

//----------------------------------------------------------------------------------------
void MacOSThreadInvocation::PushPendingCallback(std::function<void()> callback)
{
    auto* heapCallback = new std::function<void()>(std::move(callback));
    dispatch_async_f(dispatch_get_main_queue(), heapCallback, [](void* ctx) { auto* fn = static_cast<std::function<void()>*>(ctx); (*fn)(); delete fn; });
}

} // namespace cobalt::graphics::testing
