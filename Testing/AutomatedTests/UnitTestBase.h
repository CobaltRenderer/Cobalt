// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include "GeometryHelper.h"
#include "ITestSession.h"
#include "IUnitTest.h"
#include "MipmappingHelper.h"
#include "TextureHelper.h"
#include "TexturedQuadSceneHelper.h"
#include "TransformHelper.h"
#include <Cobalt/LoggingInterface/LoggingInterface.pkg>
#include <Cobalt/RendererInterface/RendererInterface.pkg>
#include <memory>
namespace cobalt::graphics::testing {

class UnitTestBase : public IUnitTest
{
public:
	// Constructors
	explicit UnitTestBase(const std::string& testFullName);

	// Test properties
	std::string TestFullName() const override;
	Type GetType() const override;

	// Test methods
	bool ExecuteTest(ITestSession& session) override;

	// Helpers
	GeometryHelper& Geometry() const;
	MipmappingHelper& Mipmapping() const;
	TextureHelper& Texture() const;
	TexturedQuadSceneHelper& TexturedQuad() const;
	TransformHelper& Transform() const;

protected:
	// Test methods
	void RequireInternal(const std::string& fileName, size_t lineNumber, const std::string& expressionAsString, bool expression) override;
	void DrawOneFrame();
	void DrawFrames(size_t frameCount);
	ITestSession::ProfileResults DrawFramesAndProfile(std::chrono::duration<float> timespan);

private:
	cobalt::graphics::IRenderer* _renderer = nullptr;
	cobalt::logging::ILogger* _log = nullptr;
	ITestSession* _session = nullptr;
	mutable std::unique_ptr<GeometryHelper> _geometryHelper;
	mutable std::unique_ptr<MipmappingHelper> _mipmappingHelper;
	mutable std::unique_ptr<TextureHelper> _textureHelper;
	mutable std::unique_ptr<TexturedQuadSceneHelper> _texturedQuadSceneHelper;
	mutable std::unique_ptr<TransformHelper> _transformHelper;
	std::string _testFullName;
};

} // namespace cobalt::graphics::testing
