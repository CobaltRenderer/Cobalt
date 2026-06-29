// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include "IThreadInvocation.h"

namespace cobalt::graphics::testing {

class MacOSThreadInvocation : public IThreadInvocation
{
public:
	// Constructors
	explicit MacOSThreadInvocation(std::thread::id uiThreadId);

protected:
	// Invocation methods
	std::thread::id GetTargetThreadId() const override;
	void PushPendingCallback(std::function<void()> callback) override;

private:
	std::thread::id _threadId;
};

} // namespace cobalt::graphics::testing
