// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include "Direct3DStateContainer.h"
#include <Cobalt/LoggingInterface/LoggingInterface.pkg>
#include <Cobalt/RendererInterface/RendererInterface.pkg>
namespace cobalt::graphics {

class Direct3DDefaultState : public Direct3DStateContainer<IDefaultState>
{
public:
	// Constructors
	Direct3DDefaultState(cobalt::logging::ILogger* log, Direct3DRenderer* renderer);

	// Initialization methods
	void Delete() override;

	// Build state methods
	void MigrateBuildStateToDrawState();

private:
	cobalt::logging::ILogger* _log;
	Direct3DRenderer* _renderer;
};

} // namespace cobalt::graphics
