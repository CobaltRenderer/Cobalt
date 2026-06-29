// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "Direct3DDefaultState.h"
#include "Direct3DRenderer.h"
namespace cobalt::graphics {

//----------------------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------------------
Direct3DDefaultState::Direct3DDefaultState(cobalt::logging::ILogger* log, Direct3DRenderer* renderer)
: Direct3DStateContainer(log), _renderer(renderer), _log(log)
{}

//----------------------------------------------------------------------------------------
// Initialization methods
//----------------------------------------------------------------------------------------
void Direct3DDefaultState::Delete()
{
	_renderer->DeleteObject(this);
}

//----------------------------------------------------------------------------------------
// Build state methods
//----------------------------------------------------------------------------------------
void Direct3DDefaultState::MigrateBuildStateToDrawState()
{
	// Migrate our build state
	if (!IsDrawStateCurrent())
	{
		// Transfer basic build state
		Direct3DStateContainer::MigrateBuildStateToDrawState();

		// Flag that our draw state is now current
		FlagDrawStateCurrent();
	}
}

} // namespace cobalt::graphics
