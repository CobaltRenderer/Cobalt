// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "TransformHelper.h"
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>
#include <utility>
namespace cobalt::graphics::testing {

//----------------------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------------------
TransformHelper::TransformHelper(cobalt::logging::ILogger::unique_ptr log)
: _log(std::move(log))
{}

//----------------------------------------------------------------------------------------
// View transforms
//----------------------------------------------------------------------------------------
M4Float32 TransformHelper::LookAtCenterPerspective(const cobalt::graphics::V2Float32& screenSize, const cobalt::graphics::V3Float32& camPosition, float fov) const
{
	float magnitude = std::sqrt((camPosition.X() * camPosition.X()) + (camPosition.Y() * camPosition.Y()) + (camPosition.Z() * camPosition.Z()));
	return LookAtCenterPerspective(screenSize, camPosition, fov, 0.001f, magnitude * 2);
}

//----------------------------------------------------------------------------------------
M4Float32 TransformHelper::LookAtCenterPerspective(const cobalt::graphics::V2Float32& screenSize, const cobalt::graphics::V3Float32& camPosition, float fov, float nearPlane, float farPlane) const
{
	glm::vec3 cam(camPosition.X(), camPosition.Y(), camPosition.Z());

	glm::mat4 viewProj = glm::lookAt(cam, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	viewProj = glm::perspectiveFov(glm::radians(fov), screenSize.X(), screenSize.Y(), nearPlane, farPlane) * viewProj;

	return GlmToSrMatrix(viewProj);
}

//----------------------------------------------------------------------------------------
M4Float32 TransformHelper::GetRandomModelTransform(PseudoRandomGenerator& randomGenerator, bool performTranslate, bool performScale) const
{
	glm::mat4 modelTransform(1.0);

	if (performTranslate)
	{
		float randomOffsetX = (randomGenerator.GetNextNormalized() * 2.0f) - 1.0f;
		float randomOffsetY = (randomGenerator.GetNextNormalized() * 2.0f) - 1.0f;
		float randomOffsetZ = (randomGenerator.GetNextNormalized() * 2.0f) - 1.0f;
		glm::vec3 translation(randomOffsetX, randomOffsetY, randomOffsetZ);
		modelTransform = glm::translate(modelTransform, translation);
	}

	if (performScale)
	{
		float randomScaleX = (randomGenerator.GetNextNormalized() * 2.0f) - 1.0f;
		float randomScaleY = (randomGenerator.GetNextNormalized() * 2.0f) - 1.0f;
		float randomScaleZ = (randomGenerator.GetNextNormalized() * 2.0f) - 1.0f;
		glm::vec3 scale(randomScaleX, randomScaleY, randomScaleZ);
		modelTransform = glm::scale(modelTransform, scale);
	}

	return GlmToSrMatrix(modelTransform);
}

//----------------------------------------------------------------------------------------
// Conversion methods
//----------------------------------------------------------------------------------------
M4Float32 TransformHelper::GlmToSrMatrix(const glm::mat4& matrix) const
{
	M4Float32 out;
	for (int i = 0; i < 16; ++i)
	{
		out.data()[i] = matrix[i % 4][i / 4];
	}
	return out;
}

} // namespace cobalt::graphics::testing
