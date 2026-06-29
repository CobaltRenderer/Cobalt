// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "../UnitTestBase.h"
namespace cobalt::graphics::testing {
using namespace cobalt::graphics;

DEFINE_UNIT_TEST_WITH_BASE("Framebuffer/FramebufferAttachmentLifecycle", UnitTestBase)
{
	// Retrieve our important objects
	auto& renderer = session.Renderer();
	auto& uiThread = session.UIThread();
	auto& testWindowInfo = *session.TestWindowPlatformInfo();
	const V2UInt32 frameBufferSize = session.TestWindowSize();

	// Verify that a window-bound framebuffer accepts resize notifications and remains usable afterwards.
	{
		auto frameBuffer = renderer.CreateFrameBuffer();
		frameBuffer->DefineViewportRegion(V2UInt32(0, 0), frameBufferSize);
		REQUIRE(uiThread.InvokeSync([&] { return frameBuffer->BindWindow(testWindowInfo, IFrameBuffer::WindowDepthStencilMode::None); }));

		auto renderPassNode = renderer.CreateRenderPassNode();
		renderPassNode->BindFrameBuffer(frameBuffer.get());
		renderPassNode->SetAttachmentClearData(IFrameBuffer::AttachmentType::Color, 0, V4Float32(1.0f, 0.0f, 0.0f, 1.0f));
		renderer.SetRenderPasses(&renderPassNode, 1);

		auto frameBufferCapture = renderer.CreateFrameBufferOutput();
		frameBufferCapture->SetDetachAfterCapture(true);
		frameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
		DrawOneFrame();
		session.AddTestImageResult("WindowBeforeResizeNotification", "A fully red window-bound framebuffer before sending a resize notification.", std::move(frameBufferCapture), IImageDiff::Algorithm::AllOfThem, 0.95);

		REQUIRE(uiThread.InvokeSync([&] { return frameBuffer->NotifyWindowResized(frameBufferSize); }));
		renderPassNode->SetAttachmentClearData(IFrameBuffer::AttachmentType::Color, 0, V4Float32(0.0f, 1.0f, 0.0f, 1.0f));
		frameBufferCapture = renderer.CreateFrameBufferOutput();
		frameBufferCapture->SetDetachAfterCapture(true);
		frameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
		DrawOneFrame();
		session.AddTestImageResult("WindowAfterResizeNotification", "A fully green window-bound framebuffer after sending a resize notification and rendering again.", std::move(frameBufferCapture), IImageDiff::Algorithm::AllOfThem, 0.95);
		renderer.RemoveAllRenderPasses();
	}

	renderer.WaitForDeferredDeletionComplete();

	// Verify that a color attachment can be unbound and replaced.
	{
		auto firstColorTexture = renderer.CreateTextureBuffer2D();
		firstColorTexture->SetUsageFlags(ITextureBuffer::UsageFlags::FrameBufferOutput);
		firstColorTexture->SetTextureFormat(ITextureBuffer::ImageFormat::RGBA, ITextureBuffer::DataFormat::UNorm8);
		firstColorTexture->SetTextureDimensions(frameBufferSize);
		REQUIRE(firstColorTexture->AllocateMemory());

		auto secondColorTexture = renderer.CreateTextureBuffer2D();
		secondColorTexture->SetUsageFlags(ITextureBuffer::UsageFlags::FrameBufferOutput);
		secondColorTexture->SetTextureFormat(ITextureBuffer::ImageFormat::RGBA, ITextureBuffer::DataFormat::UNorm8);
		secondColorTexture->SetTextureDimensions(frameBufferSize);
		REQUIRE(secondColorTexture->AllocateMemory());

		auto frameBuffer = renderer.CreateFrameBuffer();
		frameBuffer->DefineViewportRegion(V2UInt32(0, 0), frameBufferSize);
		REQUIRE(frameBuffer->BindTexture(firstColorTexture.get(), IFrameBuffer::AttachmentType::Color));
		frameBuffer->UnbindTexture(IFrameBuffer::AttachmentType::Color);
		REQUIRE(frameBuffer->BindTexture(secondColorTexture.get(), IFrameBuffer::AttachmentType::Color));

		auto renderPassNode = renderer.CreateRenderPassNode();
		renderPassNode->BindFrameBuffer(frameBuffer.get());
		renderPassNode->SetAttachmentClearData(IFrameBuffer::AttachmentType::Color, 0, V4Float32(0.0f, 1.0f, 0.0f, 1.0f));
		renderer.SetRenderPasses(&renderPassNode, 1);

		auto frameBufferCapture = renderer.CreateFrameBufferOutput();
		frameBufferCapture->SetDetachAfterCapture(true);
		frameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
		DrawOneFrame();
		session.AddTestImageResult("ColorAttachmentRebind", "A fully green image after rebinding the framebuffer color attachment before rendering.", std::move(frameBufferCapture), IImageDiff::Algorithm::AllOfThem, 0.95);
		renderer.RemoveAllRenderPasses();
	}

	// Verify that multisample resolve can be disabled, unbound, rebound, and enabled again.
	auto multiSampleColorTexture = renderer.CreateTextureBuffer2D();
	multiSampleColorTexture->SetUsageFlags(ITextureBuffer::UsageFlags::FrameBufferOutput);
	multiSampleColorTexture->SetTextureFormat(ITextureBuffer::ImageFormat::RGBA, ITextureBuffer::DataFormat::UNorm8);
	multiSampleColorTexture->SetTextureDimensions(frameBufferSize);
	multiSampleColorTexture->SetSampleCount(ITextureBuffer::SampleCount::SampleCount2);
	if (!multiSampleColorTexture->AllocateMemory())
	{
		session.AddTestSkipped("MultiSampleResolveLifecycle", "This part of the test was skipped, as the current device could not allocate a 2x multi-sampled color attachment.");
		return true;
	}

	auto resolveTexture = renderer.CreateTextureBuffer2D();
	resolveTexture->SetUsageFlags(ITextureBuffer::UsageFlags::FrameBufferOutput | ITextureBuffer::UsageFlags::MultiSampleResolve);
	resolveTexture->SetTextureFormat(ITextureBuffer::ImageFormat::RGBA, ITextureBuffer::DataFormat::UNorm8);
	resolveTexture->SetTextureDimensions(frameBufferSize);
	REQUIRE(resolveTexture->AllocateMemory());

	auto multiSampleFrameBuffer = renderer.CreateFrameBuffer();
	multiSampleFrameBuffer->DefineViewportRegion(V2UInt32(0, 0), frameBufferSize);
	REQUIRE(multiSampleFrameBuffer->BindTexture(multiSampleColorTexture.get(), IFrameBuffer::AttachmentType::Color));
	REQUIRE(multiSampleFrameBuffer->BindMultiSamplingResolveTexture(resolveTexture.get(), IFrameBuffer::AttachmentType::Color));

	auto resolvedFrameBuffer = renderer.CreateFrameBuffer();
	resolvedFrameBuffer->DefineViewportRegion(V2UInt32(0, 0), frameBufferSize);
	REQUIRE(resolvedFrameBuffer->BindTexture(resolveTexture.get(), IFrameBuffer::AttachmentType::Color));

	auto multiSamplePass = renderer.CreateRenderPassNode();
	multiSamplePass->BindFrameBuffer(multiSampleFrameBuffer.get());
	multiSamplePass->SetAttachmentClearData(IFrameBuffer::AttachmentType::Color, 0, V4Float32(1.0f, 0.0f, 0.0f, 1.0f));
	multiSamplePass->EnableAttachmentMultiSamplingResolution(IFrameBuffer::AttachmentType::Color, 0);

	auto resolvedPass = renderer.CreateRenderPassNode();
	resolvedPass->BindFrameBuffer(resolvedFrameBuffer.get());
	resolvedPass->SetAttachmentLoadStoreBehavior(IFrameBuffer::AttachmentType::Color, 0, IRenderPassNode::AttachmentLoadBehavior::LoadExistingData, IRenderPassNode::AttachmentStoreBehavior::StoreFinalData);

	IRenderPassNode* renderPasses[] = {multiSamplePass.get(), resolvedPass.get()};
	renderer.SetRenderPasses(&renderPasses[0], 2);

	auto frameBufferCapture = renderer.CreateFrameBufferOutput();
	frameBufferCapture->SetDetachAfterCapture(true);
	resolvedFrameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
	DrawOneFrame();
	session.AddTestImageResult("MultiSampleResolveEnabled", "A fully red image resolved from the multisampled color attachment.", std::move(frameBufferCapture), IImageDiff::Algorithm::AllOfThem, 0.95);

	multiSamplePass->DisableAttachmentMultiSamplingResolution(IFrameBuffer::AttachmentType::Color, 0);
	multiSamplePass->SetAttachmentClearData(IFrameBuffer::AttachmentType::Color, 0, V4Float32(0.0f, 1.0f, 0.0f, 1.0f));
	frameBufferCapture = renderer.CreateFrameBufferOutput();
	frameBufferCapture->SetDetachAfterCapture(true);
	resolvedFrameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
	DrawOneFrame();
	session.AddTestImageResult("MultiSampleResolveDisabled", "A fully red image preserved while multisample resolve is disabled.", std::move(frameBufferCapture), IImageDiff::Algorithm::AllOfThem, 0.95);

	multiSampleFrameBuffer->UnbindMultiSamplingResolveTexture(IFrameBuffer::AttachmentType::Color);
	REQUIRE(multiSampleFrameBuffer->BindMultiSamplingResolveTexture(resolveTexture.get(), IFrameBuffer::AttachmentType::Color));
	multiSamplePass->EnableAttachmentMultiSamplingResolution(IFrameBuffer::AttachmentType::Color, 0);
	multiSamplePass->SetAttachmentClearData(IFrameBuffer::AttachmentType::Color, 0, V4Float32(0.0f, 0.0f, 1.0f, 1.0f));
	frameBufferCapture = renderer.CreateFrameBufferOutput();
	frameBufferCapture->SetDetachAfterCapture(true);
	resolvedFrameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
	DrawOneFrame();
	session.AddTestImageResult("MultiSampleResolveReEnabled", "A fully blue image after rebinding the resolve target and re-enabling multisample resolve.", std::move(frameBufferCapture), IImageDiff::Algorithm::AllOfThem, 0.95);

	// Remove all our defined render passes
	renderer.RemoveAllRenderPasses();
	return true;
}

} // namespace cobalt::graphics::testing
