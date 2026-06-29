// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "OpenGLDefaultState.h"
#include "OpenGLRenderer.h"
namespace cobalt::graphics {

//----------------------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------------------
OpenGLDefaultState::OpenGLDefaultState(cobalt::logging::ILogger* log, OpenGLRenderer* renderer)
: OpenGLStateContainer(log), _renderer(renderer), _log(log)
{}

//----------------------------------------------------------------------------------------
// Initialization methods
//----------------------------------------------------------------------------------------
void OpenGLDefaultState::Delete()
{
	_renderer->DeleteObject(this);
}

//----------------------------------------------------------------------------------------
// Build state methods
//----------------------------------------------------------------------------------------
void OpenGLDefaultState::MigrateBuildStateToDrawState()
{
	// Migrate our build state
	if (!IsDrawStateCurrent())
	{
		// Transfer basic build state
		OpenGLStateContainer::MigrateBuildStateToDrawState();

		// Flag that our draw state is now current
		FlagDrawStateCurrent();
	}
}

} // namespace cobalt::graphics
