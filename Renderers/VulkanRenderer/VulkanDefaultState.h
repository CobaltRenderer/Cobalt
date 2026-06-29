// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include "VulkanStateContainer.h"
#include <Cobalt/LoggingInterface/LoggingInterface.pkg>
#include <Cobalt/RendererInterface/RendererInterface.pkg>
namespace cobalt::graphics {

class VulkanDefaultState : public VulkanStateContainer<IDefaultState>
{
public:
	// Constructors
	VulkanDefaultState(cobalt::logging::ILogger* log, VulkanRenderer* renderer);

	// Initialization methods
	void Delete() override;

	// Build state methods
	void MigrateBuildStateToDrawState();

private:
	cobalt::logging::ILogger* _log;
	VulkanRenderer* _renderer;
};

} // namespace cobalt::graphics
