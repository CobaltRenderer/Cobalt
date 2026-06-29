// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include "OpenGLStateContainer.h"
#include <Cobalt/LoggingInterface/LoggingInterface.pkg>
#include <Cobalt/RendererInterface/RendererInterface.pkg>
namespace cobalt::graphics {

class OpenGLDefaultState : public OpenGLStateContainer<IDefaultState>
{
public:
	// Constructors
	OpenGLDefaultState(cobalt::logging::ILogger* log, OpenGLRenderer* renderer);

	// Initialization methods
	void Delete() override;

	// Build state methods
	void MigrateBuildStateToDrawState();

private:
	cobalt::logging::ILogger* _log;
	OpenGLRenderer* _renderer;
};

} // namespace cobalt::graphics
