// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include "IThreadInvocation.h"
#include <X11/Xlib.h>
namespace cobalt::graphics::testing {

class X11MessageThreadInvocation : public IThreadInvocation
{
public:
	// Constructors
	X11MessageThreadInvocation(std::thread::id threadId, Display* display, ::Window window, Atom invokeAtom);

protected:
	// Invocation methods
	std::thread::id GetTargetThreadId() const override;
	void PushPendingCallback(std::function<void()> callback) override;

private:
	std::thread::id _threadId;
	Display* _display;
	::Window _window;
	Atom _invokeAtom;
};

} // namespace cobalt::graphics::testing
