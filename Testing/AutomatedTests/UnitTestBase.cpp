// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "UnitTestBase.h"
#include "ITestSession.h"
namespace cobalt::graphics::testing {

//----------------------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------------------
UnitTestBase::UnitTestBase(const std::string& testFullName)
: _testFullName(testFullName)
{}

//----------------------------------------------------------------------------------------
// Test properties
//----------------------------------------------------------------------------------------
std::string UnitTestBase::TestFullName() const
{
	return _testFullName;
}

//----------------------------------------------------------------------------------------
UnitTestBase::Type UnitTestBase::GetType() const
{
	return IUnitTest::Type::UnitTest;
}

//----------------------------------------------------------------------------------------
// Test methods
//----------------------------------------------------------------------------------------
bool UnitTestBase::ExecuteTest(ITestSession& session)
{
	_renderer = &session.Renderer();
	_log = &session.Log();
	_session = &session;

	// Attempt to execute this test
	bool result = false;
	std::string failureMessage;
	try
	{
		result = ExecuteTestInternal(session);
	}
	catch (const std::exception& ex)
	{
		failureMessage = "An exception has been thrown: " + std::string(ex.what());
		session.AddTestFailure("Exception", "", ex.what());
	}
	catch (...)
	{
		failureMessage = "An exception has been thrown.";
		session.AddTestFailure("Exception", "", failureMessage);
	}

	// Return the result of test execution to the caller
	return result;
}

//----------------------------------------------------------------------------------------
void UnitTestBase::RequireInternal(const std::string& fileName, size_t lineNumber, const std::string& expressionAsString, bool expression)
{
	if (!expression)
	{
		std::string errorMessage = "Require call failed at " + fileName + ":" + std::to_string(lineNumber) + ". \"" + expressionAsString + "\"";
		_log->Error(errorMessage);
		throw std::logic_error(errorMessage.c_str());
	}
}

//----------------------------------------------------------------------------------------
void UnitTestBase::DrawOneFrame()
{
	_session->NotifyBeforeFrameDraw();
	_renderer->StartNewFrame();
	_renderer->WaitForOutputCaptureComplete();
}

//----------------------------------------------------------------------------------------
void UnitTestBase::DrawFrames(size_t frameCount)
{
	_session->NotifyBeforeFrameDraw();
	while (frameCount > 0)
	{
		_renderer->StartNewFrame();
		--frameCount;
	}
	_renderer->WaitForOutputCaptureComplete();
}

//----------------------------------------------------------------------------------------
ITestSession::ProfileResults UnitTestBase::DrawFramesAndProfile(std::chrono::duration<float> timespan)
{
	// Ensure no previous frame is still drawing, and ensure any previous resources have been deallocated.
	_renderer->WaitForOutputCaptureComplete();
	_renderer->WaitForDeferredDeletionComplete();

	// Draw one frame first, to ensure transfer/setup tasks are completed.
	_session->NotifyBeforeFrameDraw();
	auto setupStartTime = std::chrono::steady_clock::now();
	_renderer->StartNewFrame();
	_renderer->WaitForOutputCaptureComplete();
	auto setupEndTime = std::chrono::steady_clock::now();

	// Draw the current frame continuously, until the desired timespan elapses.
	_session->NotifyBeforeFrameDraw();
	size_t drawnFrames = 0;
	auto startTime = std::chrono::steady_clock::now();
	auto currentTime = startTime;
	do
	{
		_renderer->StartNewFrame();
		++drawnFrames;
		currentTime = std::chrono::steady_clock::now();
	} while ((currentTime - startTime) < timespan);

	// Wait for the last submitted frame to complete drawing. We do this to ensure that we have the most accurate
	// calculation for total draw time, otherwise it's likely one submitted frame would still be drawing, which we would
	// end up evaluating as though it was complete below.
	_renderer->WaitForOutputCaptureComplete();
	auto endTime = std::chrono::steady_clock::now();

	// Calculate performance metrics on the render process, and return the results to the caller.
	auto totalSetupTime = std::chrono::duration<double, std::chrono::milliseconds::period>(setupEndTime - setupStartTime);
	auto totalRenderTime = std::chrono::duration<double, std::chrono::milliseconds::period>(endTime - startTime);
	ITestSession::ProfileResults results{};
	results.framesDrawn = drawnFrames;
	results.totalRenderTime = totalRenderTime;
	results.totalSetupTime = totalSetupTime;
	results.averageFrameDrawTime = totalRenderTime / (double)drawnFrames;
	results.averageFPS = (float)((double)drawnFrames / std::chrono::duration<double, std::chrono::seconds::period>(totalRenderTime).count());
	return results;
}

//----------------------------------------------------------------------------------------
// Helpers
//----------------------------------------------------------------------------------------
GeometryHelper& UnitTestBase::Geometry() const
{
	if (_geometryHelper == nullptr)
	{
		_geometryHelper = std::make_unique<GeometryHelper>(_log->CloneLogger());
	}
	return *_geometryHelper;
}

//----------------------------------------------------------------------------------------
MipmappingHelper& UnitTestBase::Mipmapping() const
{
	if (_mipmappingHelper == nullptr)
	{
		_mipmappingHelper = std::make_unique<MipmappingHelper>(_log->CloneLogger());
	}
	return *_mipmappingHelper;
}

//----------------------------------------------------------------------------------------
TextureHelper& UnitTestBase::Texture() const
{
	if (_textureHelper == nullptr)
	{
		_textureHelper = std::make_unique<TextureHelper>(_log->CloneLogger());
	}
	return *_textureHelper;
}

//----------------------------------------------------------------------------------------
TexturedQuadSceneHelper& UnitTestBase::TexturedQuad() const
{
	if (_texturedQuadSceneHelper == nullptr)
	{
		_texturedQuadSceneHelper = std::make_unique<TexturedQuadSceneHelper>(_log->CloneLogger());
	}
	return *_texturedQuadSceneHelper;
}

//----------------------------------------------------------------------------------------
TransformHelper& UnitTestBase::Transform() const
{
	if (_transformHelper == nullptr)
	{
		_transformHelper = std::make_unique<TransformHelper>(_log->CloneLogger());
	}
	return *_transformHelper;
}

} // namespace cobalt::graphics::testing
