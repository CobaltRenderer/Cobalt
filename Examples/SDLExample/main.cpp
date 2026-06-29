// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include <Cobalt/Logging/Logging.pkg>
#include <Cobalt/RendererInterface/PlatformBindings.pkg>
#include <Cobalt/RendererInterface/RendererInterface.pkg>
#define SDL_MAIN_HANDLED
#include <SDL3/SDL.h>
#include <atomic>
#include <chrono>
#include <cmath>
#include <set>
#include <string>
#include <thread>
using namespace cobalt::graphics;

//----------------------------------------------------------------------------------------
static void UpdateThread(cobalt::logging::ILogger* log, std::atomic_flag& updateThreadShutdownRequested, IRenderer& renderer, ReadWriteMutex& renderThreadMutex, IRenderPassNode& renderPassNode);
static void RenderThread(cobalt::logging::ILogger* log, std::atomic_flag& renderThreadStartupComplete, std::atomic_flag& renderThreadShutdownRequested, IRenderer& renderer, ReadWriteMutex& renderThreadMutex, IFrameBuffer& mainWindowFrameBuffer, IRenderPassNode*& renderPassNodePointer);

//----------------------------------------------------------------------------------------
static const std::string VertexShader = R"(
struct VSInput
{
	float4 position : position;
	float3 color : color;
};

struct VSOutput
{
	float4 position : SV_POSITION;
	float3 color : COLOR;
};

uniform float rotation;
static const float PI = 3.14159265359;

VSOutput main(VSInput IN)
{
	VSOutput OUT;

	float angle = PI * rotation;
	float s, c;
	sincos(angle, s, c);
	float2x2 rotMatrix = float2x2(c, -s, s, c);
	float2 rotatedPos = mul(rotMatrix, IN.position.xy);

	OUT.color = IN.color;
	OUT.position = float4(rotatedPos, IN.position.z, IN.position.w);

	return OUT;
}
)";

//----------------------------------------------------------------------------------------
static const std::string FragmentShader = R"(
struct VSOutput
{
	float4 position : SV_POSITION;
	float3 color : COLOR;
};

float4 main(VSOutput IN) : SV_TARGET0
{
	return float4(IN.color, 1.0f);
}
)";

//----------------------------------------------------------------------------------------
int main(int argc, char** argv)
{
	// Create the log manager
	cobalt::logging::LogManager logManager;
	logManager.SetIncludeSeverity(cobalt::logging::ILogger::SeverityFilter::All);
	auto log = logManager.GetLogger("");

	// Create a console window log target
	auto consoleLogTarget = cobalt::logging::LogTargetStandardOut::Create(true);
	logManager.AddLogTarget(std::move(consoleLogTarget));

	// Select an explicit renderer plugin type if requested
	bool rendererApiSelectEnabled = false;
	RendererPlugin::ApiFamily targetApiFamily = {};
	if (argc > 1)
	{
		rendererApiSelectEnabled = true;
		std::string apiFamilyAsString = argv[1];
		if (apiFamilyAsString == "OpenGL")
		{
			targetApiFamily = RendererPlugin::ApiFamily::OpenGL;
		}
		else if (apiFamilyAsString == "Direct3D")
		{
			targetApiFamily = RendererPlugin::ApiFamily::Direct3D;
		}
		else if (apiFamilyAsString == "Vulkan")
		{
			targetApiFamily = RendererPlugin::ApiFamily::Vulkan;
		}
		else
		{
			log->Info("Requested unknown renderer API family of type \"{0}\"", apiFamilyAsString);
			rendererApiSelectEnabled = false;
		}
		if (rendererApiSelectEnabled)
		{
			log->Info("Setting target renderer API family to type \"{0}\"", apiFamilyAsString);
		}
	}

	// Set the SDL video driver explicitly if requested on the command line
	if (argc > 2)
	{
		// Windows: "windows"
		// macOS:   "cocoa"
		// Linux:   "x11", "wayland"
		log->Info("Setting SDL video driver to \"{0}\"", argv[2]);
		SDL_SetHint(SDL_HINT_VIDEO_DRIVER, argv[2]);
	}

	// Select the renderer plugin. We use the plugin enumerator to dynamically locate and select plugins here without
	// any link time dependencies, but we could also statically link to a renderer and retrieve its info structure
	// directly, like this:
	//   GetOpenGL4RendererPlugin(rendererPlugin)
	// Also note that once we've retrieved the desired plugin, we no longer need to keep the plugin enumerator around.
	// Destroying it allows any other plugin assemblies to be unloaded from memory.
	RenderPluginEnumerator pluginEnumerator(log->GetLoggerChildScope("PluginEnumerator"));
	if (!pluginEnumerator.EnumeratePluginsInDirectory(pluginEnumerator.GetProcessDirectory()))
	{
		log->Critical("EnumeratePluginsInDirectory failed");
		return 1;
	}
	if (rendererApiSelectEnabled)
	{
		pluginEnumerator.FilterPluginsNotOfFamily(targetApiFamily);
	}
	std::optional<RendererPlugin> preferredRenderPlugin = pluginEnumerator.GetPreferredPlugin();
	if (!preferredRenderPlugin.has_value())
	{
		log->Critical("Failed to locate a renderer plugin");
		return 1;
	}
	log->Info("Selected renderer: {0} [{1}]", preferredRenderPlugin->GetDisplayName().Get(), preferredRenderPlugin->GetName().Get());

	// Select the graphics device
	auto deviceEnumerator = preferredRenderPlugin->CreateGraphicsDeviceEnumerator(log->GetLoggerChildScope("Renderer"));
	if (!deviceEnumerator->EnumerateDevices())
	{
		log->Critical("EnumerateDevices failed");
		return 1;
	}
	IGraphicsDevice* device = deviceEnumerator->GetPreferredDevice();
	if (device == nullptr)
	{
		log->Critical("Failed to locate supported graphics device");
		return 1;
	}
	log->Info("Selected device {0} from vendor {1}", device->GetDeviceName().Get(), device->GetVendorName().Get());

	// Initialize SDL
	if (!SDL_Init(SDL_INIT_VIDEO))
	{
		log->Error("SDL_Init failed: {0}", SDL_GetError());
		return 1;
	}

	// Create the renderer. After this is complete, we could dispose of the device enumerator if we wanted to, which
	// will in tern destroy the device object, but the created renderer can continue to be used. We need to keep the
	// rendererModuleHandle we were returned alive however, in order to keep the renderer plugin loaded.
	std::set<IGraphicsDevice::Feature> enabledFeatures = device->GetAllSupportedFeatures();
	std::set<IRenderer::Options> enabledOptions;
#ifdef _DEBUG
	enabledOptions.insert(IRenderer::Options::EnableDebugLogging);
#endif
	auto renderer = device->CreateRenderer(enabledFeatures, enabledOptions);
	WindowSystemInfoSDL3 windowSystemInfo;
	windowSystemInfo.Initialize(log.get());
	if (!renderer->Initialize(*windowSystemInfo.Get(), IRenderer::InitializationFlags::None))
	{
		log->Critical("Failed to initialize renderer");
		return 1;
	}

	// Create our main window
	int mainWindowWidthInPixels = 1024;
	int mainWindowHeightInPixels = 1024;
	SDL_Window* mainWindow = CobaltSDLCreateWindow("Cobalt Renderer + SDL", mainWindowWidthInPixels, mainWindowHeightInPixels, SDL_WINDOW_RESIZABLE);
	if (mainWindow == nullptr)
	{
		log->Error("SDL_CreateWindow failed: {0}", SDL_GetError());
		SDL_Quit();
		return 1;
	}
	auto mainWindowId = SDL_GetWindowID(mainWindow);

	// Define the framebuffer for the main window, and bind it to the SDL window. Note that BindWindow MUST be called on
	// the UI thread. Also note that we don't limit FPS here by default for demonstration purposes. In a real
	// application, you'd probably want to set enableVSync to true here.
	bool enableVSync = false;
	auto mainWindowFrameBuffer = renderer->CreateFrameBuffer();
	WindowInfoSDL3 mainWindowInfo;
	mainWindowInfo.Initialize(log.get(), mainWindow, V2UInt32(mainWindowWidthInPixels, mainWindowHeightInPixels));
	IFrameBuffer::WindowBindingFlags windowBindingFlags = (enableVSync ? IFrameBuffer::WindowBindingFlags::LimitSwapToVSync : IFrameBuffer::WindowBindingFlags::None);
	if (!mainWindowFrameBuffer->BindWindow(*mainWindowInfo.Get(), IFrameBuffer::WindowDepthStencilMode::DepthUNorm24, IFrameBuffer::WindowColorSpaceMode::Default, windowBindingFlags))
	{
		log->Error("Failed to bind main window");
		SDL_Quit();
		return 1;
	}
	mainWindowFrameBuffer->DefineViewportRegion(V2UInt32(0, 0), V2UInt32(mainWindowWidthInPixels, mainWindowHeightInPixels));

	// Spawn our render thread. In our example here, this will also define an initial render pass. In a real program,
	// you may also want to setup some of your main render passes at this stage, IE, a "scaffold" of initially empty
	// render passes and group nodes, such as opaque+transparent+composite. The renderer will optimize out empty
	// branches of the tree during rendering.
	ReadWriteMutex renderThreadMutex;
	std::atomic_flag renderThreadStartupComplete;
	std::atomic_flag renderThreadShutdownRequested;
	std::atomic_flag renderThreadShutdownComplete;
	IRenderPassNode* renderPassNode = nullptr;
	std::thread([&]() {
		RenderThread(log.get(), renderThreadStartupComplete, renderThreadShutdownRequested, *renderer, renderThreadMutex, *mainWindowFrameBuffer, renderPassNode);
		renderThreadShutdownComplete.test_and_set();
		renderThreadShutdownComplete.notify_all();
		SDL_Event event = {};
		event.type = SDL_EVENT_USER;
		SDL_PushEvent(&event);
	}).detach();

	// Spawn our "update" thread. This demonstrates the ideal model, where the render thread can be focused on just
	// advancing frames, and a separate thread is responsible for preparing the content for the next frame. One or more
	// threads can safely create and modify scene content while the frame is drawing in parallel, as long as
	// StartNewFrame() is only ever called in a globally exclusive manner.
	std::atomic_flag updateThreadShutdownRequested;
	std::thread([&]() {
		renderThreadStartupComplete.wait(false);
		UpdateThread(log.get(), updateThreadShutdownRequested, *renderer, renderThreadMutex, *renderPassNode);
		renderThreadShutdownRequested.test_and_set();
	}).detach();

	// Run our window message loop
	SDL_Event event;
	while (!renderThreadShutdownComplete.test() && SDL_WaitEvent(&event))
	{
		switch (event.type)
		{
		case SDL_EVENT_KEY_DOWN:
			if (event.key.key == SDLK_ESCAPE)
			{
				updateThreadShutdownRequested.test_and_set();
			}
			break;
		case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
			if (mainWindowId == event.window.windowID)
			{
				updateThreadShutdownRequested.test_and_set();
			}
			break;
		case SDL_EVENT_QUIT:
			updateThreadShutdownRequested.test_and_set();
			break;
		case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
			if (mainWindowId == event.window.windowID)
			{
				// Notify the renderer of the resize event. Note that this MUST be called from the UI thread, as
				// some platforms (IE, AppKit, Wayland) require this. The call will not block, but we need to
				// synchronize with the renderer so it doesn't collide with frame advance.
				auto lock = renderThreadMutex.ObtainReadLock();
				mainWindowWidthInPixels = event.window.data1;
				mainWindowHeightInPixels = event.window.data2;
				if (!mainWindowFrameBuffer->NotifyWindowResized(V2UInt32(mainWindowWidthInPixels, mainWindowHeightInPixels)))
				{
					log->Warning("Failed to update main window size");
				}
				mainWindowFrameBuffer->DefineViewportRegion(V2UInt32(0, 0), V2UInt32(mainWindowWidthInPixels, mainWindowHeightInPixels));
			}
			break;
		}
	}

	// Delete the main window framebuffer. We do this to ensure it's unbound from the window, so the window can be
	// cleanly destroyed. This could also be done on the render thread during cleanup.
	mainWindowFrameBuffer.reset();

	// Advance the renderer to the point where any resources pending deletion have been removed. This makes it safe to
	// destroy windows which were bound to the renderer immediately after this call.
	renderer->WaitForDeferredDeletionComplete();

	// Destroy the main window
	SDL_DestroyWindow(mainWindow);

	// Release the renderer. We need to do this here, since we've created the renderer directly in main, and as on some
	// systems (IE, x11) resources need to be freed by the renderer during destruction before the window system is
	// destroyed.
	renderer.reset();

	// Clean up the SDL framework. All other resources will be destroyed automatically on close.
	SDL_Quit();
	return 0;
}

//----------------------------------------------------------------------------------------
static void RenderThread(cobalt::logging::ILogger* log, std::atomic_flag& renderThreadStartupComplete, std::atomic_flag& renderThreadShutdownRequested, IRenderer& renderer, ReadWriteMutex& renderThreadMutex, IFrameBuffer& mainWindowFrameBuffer, IRenderPassNode*& renderPassNodePointer)
{
	// Create our render pass node
	auto renderPassNode = renderer.CreateRenderPassNode();
	renderPassNode->BindFrameBuffer(&mainWindowFrameBuffer);
	renderPassNode->SetAttachmentClearData(IFrameBuffer::AttachmentType::Color, 0, V4Float32(0.0f, 0.0f, 0.0f, 1.0f));
	renderPassNode->SetAttachmentClearData(IFrameBuffer::AttachmentType::Depth, 0, V4Float32(1.0f, 0.0f, 0.0f, 0.0f));
	renderPassNodePointer = renderPassNode.get();

	// Bind our render tree to the renderer
	renderer.SetRenderPasses(&renderPassNode, 1);

	// Run our render/logic loop
	renderThreadStartupComplete.test_and_set();
	renderThreadStartupComplete.notify_all();
	auto lastFpsPrintTime = std::chrono::steady_clock::now();
	auto fpsPrintInterval = std::chrono::seconds(2);
	size_t frameCountSinceLastUpdate = 0;
	while (!renderThreadShutdownRequested.test())
	{
		// Update FPS, printing it to the console if enough time has elapsed.
		auto currentTime = std::chrono::steady_clock::now();
		auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - lastFpsPrintTime);
		if (duration > fpsPrintInterval)
		{
			auto elapsedSeconds = std::chrono::duration<float>(duration).count();
			auto fps = (float)frameCountSinceLastUpdate / elapsedSeconds;
			log->Info("Current FPS: {0}", std::to_string(fps));
			lastFpsPrintTime = currentTime;
			frameCountSinceLastUpdate = 0;
		}

		// We do a wait here because we don't want to leave the mutex locked almost all the time
		renderer.WaitForDrawComplete();
		++frameCountSinceLastUpdate;
		auto lock = renderThreadMutex.ObtainWriteLock();
		renderer.StartNewFrame();
	}

	// Remove all our defined render passes. This will unlink all contained passes and make it safe for any child nodes
	// or referenced resources to be disposed of when this function returns.
	renderer.RemoveAllRenderPasses();
}

//----------------------------------------------------------------------------------------
static void UpdateThread(cobalt::logging::ILogger* log, std::atomic_flag& updateThreadShutdownRequested, IRenderer& renderer, ReadWriteMutex& renderThreadMutex, IRenderPassNode& renderPassNode)
{
	// Simple error handling macro for code brevity
#define REQUIRE(...)                                                                                 \
	do                                                                                               \
	{                                                                                                \
		if (!(__VA_ARGS__))                                                                          \
		{                                                                                            \
			log->Error("REQUIRE failed at {0}:{1} - {2}", __FILE__, (size_t)__LINE__, #__VA_ARGS__); \
			return;                                                                                  \
		}                                                                                            \
	} while (0)

	// Obtain a lock to synchronize with the StartNewFrame() function in the render thread. Multiple threads can take a
	// "read" lock at once, so concurrent updates from more than one background thread are possible. Ideally all
	// updating logic is done without a lock taken without interacting the renderer itself, and a lock is only obtained
	// at the end when pushing the accumulated changes to the renderer. For the case of large updates to existing
	// buffers that need to start as early as possible, it is best to use multiple buffers and "rotate" them when all
	// required changes are complete, possibly using batch transfers to allow them to span multiple frames. This will
	// allow you to keep scene updates "transactional", while still allowing transfers to begin as early as possible.
	auto lock = renderThreadMutex.ObtainReadLock();

	// Create and compile our shader program
	auto shaderProgram = renderer.CreateShaderProgram();
	REQUIRE(shaderProgram->LoadShaderStage(IShaderProgram::ShaderStage::Vertex, ShaderSourceInfoHLSL(VertexShader)));
	REQUIRE(shaderProgram->LoadShaderStage(IShaderProgram::ShaderStage::Fragment, ShaderSourceInfoHLSL(FragmentShader)));
	REQUIRE(shaderProgram->CompileProgram());

	// Retrieve our shader attribute IDs
	auto positionAttributeId = shaderProgram->GetVertexAttributeId("position");
	auto colorAttributeId = shaderProgram->GetVertexAttributeId("color");

	// Create our vertex buffer and populate it with data. We make an equilateral triangle here centered at 0,0.
	const float triangleRadius = 0.75f;
	const float pi = 3.14159265f;
	std::vector<V4Float32> positionVertexData = {
	  {0.0f, triangleRadius, 0.0f, 1.0f},
	  {triangleRadius * std::cos(-pi / 6.0f), triangleRadius * std::sin(-pi / 6.0f), 0.0f, 1.0f},
	  {triangleRadius * std::cos(7.0f * pi / 6.0f), triangleRadius * std::sin(7.0f * pi / 6.0f), 0.0f, 1.0f}};
	std::vector<V3Float32> colorVertexData = {{1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}};
	VertexAttribute<V4Float32> vertexAttributePosition(positionVertexData.size(), IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadNever, IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadOften);
	VertexAttribute<V3Float32> vertexAttributeColor(colorVertexData.size(), IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadNever, IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadOften);
	auto vertexBuffer = renderer.CreateVertexBuffer();
	REQUIRE(vertexBuffer->BindVertexAttribute(vertexAttributePosition));
	REQUIRE(vertexAttributePosition.SetInitialData(positionVertexData));
	REQUIRE(vertexBuffer->BindVertexAttribute(vertexAttributeColor));
	REQUIRE(vertexAttributeColor.SetInitialData(colorVertexData));
	REQUIRE(vertexBuffer->AllocateMemory());

	// Create our renderable node
	auto renderableNode = renderer.CreateRenderableNode();
	REQUIRE(renderableNode->BindVertexAttribute(vertexAttributePosition, positionAttributeId));
	REQUIRE(renderableNode->BindVertexAttribute(vertexAttributeColor, colorAttributeId));
	REQUIRE(renderableNode->SetPrimitiveMode(IRenderableNode::PrimitiveMode::Triangles));

	// Create our state group node
	auto groupNode = renderer.CreateStateGroupNode();
	groupNode->AddChildNode(renderableNode.get());

	// Create our program node
	auto programNode = renderer.CreateProgramNode();
	REQUIRE(programNode->BindShaderProgram(shaderProgram.get()));
	programNode->AddChildNode(groupNode.get());

	// Get the state value ID for the rotation uniform, and set the group node as the state container to apply it on.
	// Note that although we're setting the rotation on the entire group node, we could set it on an individual
	// renderable node instead, or at the program node level via default state. Bindings and uniforms can therefore be
	// shared at a high level, or separated at a more granular level. The renderer will generate optimal code to make
	// state and binding updates as efficient as possible in either case.
	float rotation = 0.0f;
	auto rotationStateValueId = shaderProgram->GetStateValueId("rotation");
	groupNode->SetStateValue(rotationStateValueId, V1Float32(rotation));

	// Add the program node to the scene
	renderPassNode.AddChildNode(programNode.get());

	// Run our update loop
	auto startTime = std::chrono::steady_clock::now();
	while (!updateThreadShutdownRequested.test())
	{
		// Sleep for 20ms. We release our lock here and obtain it again afterwards, so we don't continuously block the
		// render thread.
		lock.reset();
		std::this_thread::sleep_for(std::chrono::milliseconds(20));
		lock = renderThreadMutex.ObtainReadLock();

		// Update the rotation value. Note that we could do anything we want here, such as adding, modifying, or
		// removing nodes or resources for the next frame draw.
		const double fullRotationIntervalInMilliseconds = 5000.0;
		auto currentTime = std::chrono::steady_clock::now();
		auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - startTime);
		rotation = -(float)((double)duration.count() / fullRotationIntervalInMilliseconds);
		groupNode->SetStateValue(rotationStateValueId, V1Float32(rotation));
	}

	// Remove the program node from the scene
	renderPassNode.RemoveChildNode(programNode.get());

	// Remove error handing macro
#undef REQUIRE
}
