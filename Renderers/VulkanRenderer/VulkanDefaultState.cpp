// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "VulkanDefaultState.h"
#include "VulkanRenderer.h"
namespace cobalt::graphics {

//----------------------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------------------
VulkanDefaultState::VulkanDefaultState(cobalt::logging::ILogger* log, VulkanRenderer* renderer)
: VulkanStateContainer(log), _renderer(renderer), _log(log)
{}

//----------------------------------------------------------------------------------------
// Initialization methods
//----------------------------------------------------------------------------------------
void VulkanDefaultState::Delete()
{
	_renderer->DeleteObject(this);
}

//----------------------------------------------------------------------------------------
// Build state methods
//----------------------------------------------------------------------------------------
void VulkanDefaultState::MigrateBuildStateToDrawState()
{
	// Migrate our build state
	if (!IsDrawStateCurrent())
	{
		// Transfer basic build state
		VulkanStateContainer::MigrateBuildStateToDrawState();

		// Flag that our draw state is now current
		FlagDrawStateCurrent();
	}
}

} // namespace cobalt::graphics
