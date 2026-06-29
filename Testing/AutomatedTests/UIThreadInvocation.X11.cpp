// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "UIThreadInvocation.X11.h"
#include <cstdint>
namespace cobalt::graphics::testing {

//----------------------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------------------
X11MessageThreadInvocation::X11MessageThreadInvocation(std::thread::id threadId, Display* display, ::Window window, Atom invokeAtom)
: _threadId(threadId), _display(display), _window(window), _invokeAtom(invokeAtom)
{}

//----------------------------------------------------------------------------------------
// Invocation methods
//----------------------------------------------------------------------------------------
std::thread::id X11MessageThreadInvocation::GetTargetThreadId() const
{
	return _threadId;
}

//----------------------------------------------------------------------------------------
void X11MessageThreadInvocation::PushPendingCallback(std::function<void()> callback)
{
	auto* heapCallback = new std::function<void()>(std::move(callback));

	XEvent ev = {};
	ev.xclient.type = ClientMessage;
	ev.xclient.display = _display;
	ev.xclient.window = _window;
	ev.xclient.message_type = _invokeAtom;
	ev.xclient.format = 32;
#if INTPTR_MAX == INT64_MAX
	ev.xclient.data.l[0] = (long)reinterpret_cast<uintptr_t>(heapCallback);
	ev.xclient.data.l[1] = (long)(reinterpret_cast<uintptr_t>(heapCallback) >> 32);
#else
	ev.xclient.data.l[0] = (long)reinterpret_cast<uintptr_t>(heapCallback);
#endif

	XSendEvent(_display, _window, False, 0, &ev);
	XFlush(_display);
}

} // namespace cobalt::graphics::testing
