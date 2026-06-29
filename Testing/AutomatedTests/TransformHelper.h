// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include <Cobalt/LoggingInterface/LoggingInterface.pkg>
#include <Cobalt/RendererInterface/RendererInterface.pkg>
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4201)
#endif
#include <glm/glm.hpp>
#ifdef _MSC_VER
#pragma warning(pop)
#endif
#include "PseudoRandomGenerator.h"
#include <vector>
namespace cobalt::graphics::testing {

class TransformHelper
{
public:
	// Constructors
	explicit TransformHelper(cobalt::logging::ILogger::unique_ptr log);

	// View methods
	M4Float32 LookAtCenterPerspective(const cobalt::graphics::V2Float32& screenSize, const cobalt::graphics::V3Float32& camPosition, float fov = 80.0f) const;
	M4Float32 LookAtCenterPerspective(const cobalt::graphics::V2Float32& screenSize, const cobalt::graphics::V3Float32& camPosition, float fov, float nearPlane, float farPlane) const;

	// Model methods
	M4Float32 GetRandomModelTransform(PseudoRandomGenerator& randomGenerator, bool performTranslate = true, bool performScale = true) const;

private:
	// Conversion methods
	cobalt::graphics::M4Float32 GlmToSrMatrix(const glm::mat4& matrix) const;

private:
	cobalt::logging::ILogger::unique_ptr _log;
};

} // namespace cobalt::graphics::testing
