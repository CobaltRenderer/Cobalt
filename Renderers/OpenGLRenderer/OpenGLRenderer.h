// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include "BindingHelpers.h"
#include "OpenGLHeaders.h"
#include "OpenGLShaderProgram.h"
#include <Cobalt/LoggingInterface/LoggingInterface.pkg>
#include <Cobalt/RendererInterface/RendererInterface.pkg>
#include <functional>
#include <map>
#include <queue>
#include <string>
#include <unordered_set>
#include <variant>
#include <vector>
#ifdef OPENGL_USE_EGL
#include <gbm.h>
#endif
namespace cobalt::graphics {
class OpenGLDefaultState;
class OpenGLFrameBuffer;
class OpenGLFrameBufferOutput;
class OpenGLIndexBuffer;
class OpenGLProgramNode;
class OpenGLRenderPassNode;
class OpenGLRenderableNode;
class OpenGLShaderProgram;
class OpenGLDataArray;
class OpenGLDataArrayOutput;
class OpenGLTexelArray;
class OpenGLTexelArrayOutput;
class OpenGLStateBuffer;
class OpenGLStateBufferLayout;
class OpenGLStateGroupNode;
class OpenGLTextureBuffer1D;
class OpenGLTextureBuffer1DArray;
class OpenGLTextureBuffer2D;
class OpenGLTextureBuffer2DArray;
class OpenGLTextureBuffer3D;
class OpenGLTextureBufferCube;
class OpenGLTextureBufferCubeArray;
class OpenGLTextureSampler1D;
class OpenGLTextureSampler1DArray;
class OpenGLTextureSampler2D;
class OpenGLTextureSampler2DArray;
class OpenGLTextureSampler3D;
class OpenGLTextureSamplerCube;
class OpenGLTextureSamplerCubeArray;
class OpenGLVertexBuffer;

class OpenGLRenderer : public IRenderer
{
public:
	// Structures
	struct PixelFormatInfo
	{
#ifdef OPENGL_USE_PLATFORM_WIN32
		PIXELFORMATDESCRIPTOR pixelFormatDescriptor;
		int pixelFormatIndex;
#endif
#ifdef OPENGL_USE_EGL
		EGLConfig fbConfig = nullptr;
		int redBits;
		int greenBits;
		int blueBits;
		int alphaBits;
		int depthBits;
		int stencilBits;
#elif defined(OPENGL_USE_PLATFORM_XLIB)
		GLXFBConfig fbConfig = nullptr;
		XVisualInfo visualInfo;
		int redBits;
		int greenBits;
		int blueBits;
		int alphaBits;
		int depthBits;
		int stencilBits;
#endif
#ifdef OPENGL_USE_PLATFORM_APPKIT
		CGLPixelFormatObj pixelFormatObj = nullptr;
		int virtualScreenNo;
		int colorBits;
		int alphaBits;
		int depthBits;
		int stencilBits;
#endif
	};
	struct ContainerObjectsPendingRelease
	{
		bool slotAllocated = false;
		size_t generationIndex = 0;
		std::vector<GLuint>* vertexArrayObjects = nullptr;
	};

public:
	// Constructors
#ifdef __linux__
	OpenGLRenderer(cobalt::logging::ILogger::unique_ptr log, const std::set<IGraphicsDevice::Feature>& enabledFeatures, const std::set<Options>& enabledOptions, bool finishWhenSwitchingContexts, bool flushFramebufferTextureWritesBeforeWindowSampling, bool usingSoftwareRenderer, const std::string& renderNodePath);
#else
	OpenGLRenderer(cobalt::logging::ILogger::unique_ptr log, const std::set<IGraphicsDevice::Feature>& enabledFeatures, const std::set<Options>& enabledOptions, bool finishWhenSwitchingContexts, bool flushFramebufferTextureWritesBeforeWindowSampling, bool usingSoftwareRenderer);
#endif

	// Initialization methods
	SuccessToken Initialize(const WindowSystemInfoBase& windowSystemInfo, InitializationFlags flags) override;
	void Delete() override;

	// Geometry buffer methods
	IVertexBuffer* CreateVertexBufferInternal() override;
	IIndexBuffer* CreateIndexBufferInternal() override;

	// Image buffer methods
	ITextureBuffer1D* CreateTextureBuffer1DInternal() override;
	ITextureBuffer2D* CreateTextureBuffer2DInternal() override;
	ITextureBuffer3D* CreateTextureBuffer3DInternal() override;
	ITextureBufferCube* CreateTextureBufferCubeInternal() override;
	ITextureBuffer1DArray* CreateTextureBuffer1DArrayInternal() override;
	ITextureBuffer2DArray* CreateTextureBuffer2DArrayInternal() override;
	ITextureBufferCubeArray* CreateTextureBufferCubeArrayInternal() override;

	// Image sampler methods
	ITextureSampler1D* CreateTextureSampler1DInternal() override;
	ITextureSampler2D* CreateTextureSampler2DInternal() override;
	ITextureSampler3D* CreateTextureSampler3DInternal() override;
	ITextureSamplerCube* CreateTextureSamplerCubeInternal() override;
	ITextureSampler1DArray* CreateTextureSampler1DArrayInternal() override;
	ITextureSampler2DArray* CreateTextureSampler2DArrayInternal() override;
	ITextureSamplerCubeArray* CreateTextureSamplerCubeArrayInternal() override;

	// Data array methods
	IDataArray* CreateDataArrayInternal() override;
	IDataArrayOutput* CreateDataArrayOutputInternal() override;
	ITexelArray* CreateTexelArrayInternal() override;
	ITexelArrayOutput* CreateTexelArrayOutputInternal() override;

	// Batch methods
	ITransferBatch* CreateTransferBatchInternal(ITransferBatch::StartTiming startTiming, ITransferBatch::EndTiming endTiming) override;

	// Frame buffer methods
	IFrameBuffer* CreateFrameBufferInternal() override;
	IFrameBufferOutput* CreateFrameBufferOutputInternal() override;

	// State buffer methods
	IStateBuffer* CreateStateBufferInternal() override;
	IStateBufferLayout* CreateStateBufferLayoutInternal() override;

	// Render tree node methods
	IRenderPassNode* CreateRenderPassNodeInternal() override;
	IProgramNode* CreateProgramNodeInternal() override;
	IStateGroupNode* CreateStateGroupNodeInternal() override;
	IRenderableNode* CreateRenderableNodeInternal() override;
	IDefaultState* CreateDefaultStateInternal() override;

	// Program methods
	IShaderProgram* CreateShaderProgramInternal() override;

	// Object modification/deletion methods
	template<class T>
	void FlagObjectModified(T* object);
	template<class T>
	void DeleteObject(T* object);

	// Scene content methods
	void SetRenderPasses(IRenderPassNode* const* childNodes, size_t childNodeCount, const int32_t* childNodeSortOrder = nullptr) override;
	void SetRenderPasses(IRenderPassNode::unique_ptr const* childNodes, size_t childNodeCount, const int32_t* childNodeSortOrder = nullptr) override;
	void RemoveAllRenderPasses() override;

	// Render methods
	void StartNewFrame() override;
	void WaitForDrawComplete() const override;
	void WaitForOutputCaptureComplete() const override;
	void WaitForDeferredDeletionComplete() const override;
	void AddCurrentFrameBufferOutput(OpenGLFrameBufferOutput* frameBufferOutput);
	void AddCurrentDataArrayOutput(OpenGLDataArrayOutput* resourceBufferOutput);
	void AddCurrentDataArray(OpenGLDataArray* resourceBuffer);
	void AddCurrentTexelArrayOutput(OpenGLTexelArrayOutput* resourceBufferOutput);
	void AddCurrentTexelArray(OpenGLTexelArray* resourceBuffer);

	// Render thread invocation methods
	template<class T>
	void RenderThreadInvokeAsync(const T& command);
	template<class T>
	auto RenderThreadInvokeSync(const T& command) -> decltype(command());
	template<class T>
	void RenderThreadInvokeSyncVoidReturn(const T& command);

	// State buffer methods
	int GetMaxUniformBlockSize() const;
	int GetMaxShaderUniformBlocks(IShaderProgram::ShaderStage stage) const;
	int GetUniformBufferOffsetAlignment() const;

	// Texture and sampler methods
	float GetMaxTextureSamplerAnisotropy() const;

	// Rendering context methods
#ifdef OPENGL_USE_PLATFORM_WIN32
	bool SelectWindowPixelFormat(HWND hwnd, IFrameBuffer::WindowColorSpaceMode windowColorSpaceMode, IFrameBuffer::WindowDepthStencilMode windowDepthStencilMode, HDC& deviceContext, PixelFormatInfo& pixelFormatInfo) const;
	static bool SelectWindowPixelFormat(HWND hwnd, IFrameBuffer::WindowColorSpaceMode windowColorSpaceMode, IFrameBuffer::WindowDepthStencilMode windowDepthStencilMode, HDC& deviceContext, PixelFormatInfo& pixelFormatInfo, const cobalt::logging::ILogger& log, bool debugLoggingEnabled);
	bool RetrieveOrCreateRenderingContextForWindow(HWND hwnd, HDC deviceContext, const PixelFormatInfo& pixelFormatInfo, HGLRC& renderingContext, size_t& renderingContextIndex, bool makeCurrent);
	bool ActivateRenderingContext(HWND hwnd, HDC deviceContext, HGLRC renderingContext) const;
#elif defined(OPENGL_USE_EGL)
	bool SelectWindowPixelFormat(EGLDisplay display, IFrameBuffer::WindowColorSpaceMode windowColorSpaceMode, IFrameBuffer::WindowDepthStencilMode windowDepthStencilMode, PixelFormatInfo& pixelFormatInfo, bool offscreenDevice) const;
	static bool SelectWindowPixelFormat(EGLDisplay display, IFrameBuffer::WindowColorSpaceMode windowColorSpaceMode, IFrameBuffer::WindowDepthStencilMode windowDepthStencilMode, PixelFormatInfo& pixelFormatInfo, bool offscreenDevice, const cobalt::logging::ILogger& log, bool debugLoggingEnabled);
	bool RetrieveOrCreateRenderingContextForWindow(EGLDisplay display, EGLSurface window, const PixelFormatInfo& pixelFormatInfo, EGLContext& renderingContext, size_t& renderingContextIndex, bool makeCurrent);
	bool ActivateRenderingContext(EGLDisplay display, EGLSurface surface, EGLContext renderingContext) const;
#elif defined(OPENGL_USE_PLATFORM_XLIB)
	bool SelectWindowPixelFormat(::Display* display, IFrameBuffer::WindowColorSpaceMode windowColorSpaceMode, IFrameBuffer::WindowDepthStencilMode windowDepthStencilMode, PixelFormatInfo& pixelFormatInfo) const;
	static bool SelectWindowPixelFormat(::Display* display, IFrameBuffer::WindowColorSpaceMode windowColorSpaceMode, IFrameBuffer::WindowDepthStencilMode windowDepthStencilMode, PixelFormatInfo& pixelFormatInfo, const cobalt::logging::ILogger& log, bool debugLoggingEnabled);
	bool RetrieveOrCreateRenderingContextForWindow(::Display* display, ::Window window, const PixelFormatInfo& pixelFormatInfo, GLXContext& renderingContext, size_t& renderingContextIndex, bool makeCurrent);
	bool ActivateRenderingContext(::Display* display, ::Window window, GLXContext renderingContext) const;
#elif defined(OPENGL_USE_PLATFORM_APPKIT)
	bool SelectPixelFormat(IFrameBuffer::WindowColorSpaceMode windowColorSpaceMode, IFrameBuffer::WindowDepthStencilMode windowDepthStencilMode, PixelFormatInfo& pixelFormatInfo) const;
	static bool SelectPixelFormat(IFrameBuffer::WindowColorSpaceMode windowColorSpaceMode, IFrameBuffer::WindowDepthStencilMode windowDepthStencilMode, PixelFormatInfo& pixelFormatInfo, const cobalt::logging::ILogger& log, bool debugLoggingEnabled);
	bool RetrieveOrCreateRenderingContextForWindow(NSView* window, const PixelFormatInfo& pixelFormatInfo, CGLContextObj& renderingContext, size_t& renderingContextIndex, bool makeCurrent);
	bool ActivateRenderingContext(CGLContextObj renderingContext) const;
#endif
	GLuint GenerateVertexArrayObject(size_t renderingContextIndex);

	// Settings methods
	bool DebugLoggingEnabled() const;
	bool UsePixelBufferObjectsForFrameCapture() const;
	bool CaptureWindowFramebuffersFromFrontBuffer() const;

	// Extension methods
	bool OpenGLExtensionPresent(const std::string& name) const;
	bool SpirvShadersSupported() const;

private:
	// Structures
	struct RenderPassEntry
	{
		struct Sorter
		{
			inline bool operator()(const RenderPassEntry& first, const RenderPassEntry& second)
			{
				return (first.sortIndex < second.sortIndex);
			}
		};

		OpenGLRenderPassNode* renderPassNode;
		int sortIndex;
	};
	struct MutableState
	{
#ifdef GL_VERSION_4_3
		using AllObjectTypes = std::variant<OpenGLVertexBuffer*, OpenGLIndexBuffer*, OpenGLTextureBuffer1D*, OpenGLTextureBuffer2D*, OpenGLTextureBuffer3D*, OpenGLTextureBufferCube*, OpenGLTextureBuffer1DArray*, OpenGLTextureBuffer2DArray*, OpenGLTextureBufferCubeArray*, OpenGLTextureSampler1D*, OpenGLTextureSampler2D*, OpenGLTextureSampler3D*, OpenGLTextureSamplerCube*, OpenGLTextureSampler1DArray*, OpenGLTextureSampler2DArray*, OpenGLTextureSamplerCubeArray*, OpenGLFrameBuffer*, OpenGLFrameBufferOutput*, OpenGLStateBuffer*, OpenGLDataArray*, OpenGLTexelArray*, OpenGLDataArrayOutput*, OpenGLTexelArrayOutput*, OpenGLStateBufferLayout*, OpenGLShaderProgram*, OpenGLRenderPassNode*, OpenGLProgramNode*, OpenGLStateGroupNode*, OpenGLRenderableNode*, OpenGLDefaultState*>;
		using MigrateStateObjectTypes = std::variant<OpenGLVertexBuffer*, OpenGLIndexBuffer*, OpenGLTextureBuffer1D*, OpenGLTextureBuffer2D*, OpenGLTextureBuffer3D*, OpenGLTextureBufferCube*, OpenGLTextureBuffer1DArray*, OpenGLTextureBuffer2DArray*, OpenGLTextureBufferCubeArray*, OpenGLTextureSampler1D*, OpenGLTextureSampler2D*, OpenGLTextureSampler3D*, OpenGLTextureSamplerCube*, OpenGLTextureSampler1DArray*, OpenGLTextureSampler2DArray*, OpenGLTextureSamplerCubeArray*, OpenGLFrameBuffer*, OpenGLFrameBufferOutput*, OpenGLStateBuffer*, OpenGLDataArray*, OpenGLTexelArray*, OpenGLDataArrayOutput*, OpenGLTexelArrayOutput*, OpenGLDefaultState*>;
		using BufferUpdateObjectTypes = std::variant<OpenGLVertexBuffer*, OpenGLIndexBuffer*, OpenGLTextureBuffer1D*, OpenGLTextureBuffer2D*, OpenGLTextureBuffer3D*, OpenGLTextureBufferCube*, OpenGLTextureBuffer1DArray*, OpenGLTextureBuffer2DArray*, OpenGLTextureBufferCubeArray*, OpenGLStateBuffer*, OpenGLDataArray*, OpenGLTexelArray*>;
		using BufferTransferObjectTypes = std::variant<OpenGLDataArray*, OpenGLTexelArray*>;
#else
		using AllObjectTypes = std::variant<OpenGLVertexBuffer*, OpenGLIndexBuffer*, OpenGLTextureBuffer1D*, OpenGLTextureBuffer2D*, OpenGLTextureBuffer3D*, OpenGLTextureBufferCube*, OpenGLTextureBuffer1DArray*, OpenGLTextureBuffer2DArray*, OpenGLTextureBufferCubeArray*, OpenGLTextureSampler1D*, OpenGLTextureSampler2D*, OpenGLTextureSampler3D*, OpenGLTextureSamplerCube*, OpenGLTextureSampler1DArray*, OpenGLTextureSampler2DArray*, OpenGLTextureSamplerCubeArray*, OpenGLFrameBuffer*, OpenGLFrameBufferOutput*, OpenGLStateBuffer*, OpenGLStateBufferLayout*, OpenGLShaderProgram*, OpenGLRenderPassNode*, OpenGLProgramNode*, OpenGLStateGroupNode*, OpenGLRenderableNode*, OpenGLDefaultState*>;
		using MigrateStateObjectTypes = std::variant<OpenGLVertexBuffer*, OpenGLIndexBuffer*, OpenGLTextureBuffer1D*, OpenGLTextureBuffer2D*, OpenGLTextureBuffer3D*, OpenGLTextureBufferCube*, OpenGLTextureBuffer1DArray*, OpenGLTextureBuffer2DArray*, OpenGLTextureBufferCubeArray*, OpenGLTextureSampler1D*, OpenGLTextureSampler2D*, OpenGLTextureSampler3D*, OpenGLTextureSamplerCube*, OpenGLTextureSampler1DArray*, OpenGLTextureSampler2DArray*, OpenGLTextureSamplerCubeArray*, OpenGLFrameBuffer*, OpenGLFrameBufferOutput*, OpenGLStateBuffer*, OpenGLDefaultState*>;
		using BufferUpdateObjectTypes = std::variant<OpenGLVertexBuffer*, OpenGLIndexBuffer*, OpenGLTextureBuffer1D*, OpenGLTextureBuffer2D*, OpenGLTextureBuffer3D*, OpenGLTextureBufferCube*, OpenGLTextureBuffer1DArray*, OpenGLTextureBuffer2DArray*, OpenGLTextureBufferCubeArray*, OpenGLStateBuffer*>;
#endif

		std::vector<RenderPassEntry> renderPasses;
		std::vector<MigrateStateObjectTypes> migrateStatePendingObjects;
		std::vector<BufferUpdateObjectTypes> bufferUpdatePendingObjects;
#ifdef GL_VERSION_4_3
		std::vector<BufferTransferObjectTypes> bufferTransferPendingObjects;
#endif
		std::vector<AllObjectTypes> deletePendingObjects;
	};
	struct RenderingContextInfo
	{
		bool slotAllocated = false;
		PixelFormatInfo pixelFormatInfo;
		size_t generationIndex = 0;
		std::vector<GLuint> freeVertexArrayObjectIDs;
#ifdef OPENGL_USE_PLATFORM_WIN32
		HWND hwnd = nullptr;
		HDC deviceContext = nullptr;
		HGLRC renderingContext = nullptr;
#elif defined(OPENGL_USE_EGL)
		EGLDisplay display = EGL_NO_DISPLAY;
		EGLSurface window = EGL_NO_SURFACE;
		EGLContext renderingContext = EGL_NO_CONTEXT;
#elif defined(OPENGL_USE_PLATFORM_XLIB)
		::Display* display = nullptr;
		::Window window;
		GLXContext renderingContext = nullptr;
#elif defined(OPENGL_USE_PLATFORM_APPKIT)
		NSView* window = nullptr;
		CGLContextObj renderingContext = nullptr;
#endif
	};

private:
	// Render methods
	void RenderThread();
	bool PerformInitializeOpenGL();
	void PerformFreeOpenGLResources();
	void PerformPrepareBuildOperation();
	void PerformRenderOperation();
	void PerformSwapOperation();
	void PerformDeleteLastDrawResourcesOperation();
	void PerformDeleteNextDrawResourcesOperation();
	void PerformDeleteDrawResourcesOperation(bool nextDrawDelete);
	void PerformDeletePendingVertexArrayObjects();
	void BindTextures(const std::vector<ITextureBindingInfo*>& bindingEntries, OpenGLShaderProgram* program);
	void BindStateBuffers(const std::vector<StateBufferBindingInfo*>& bindingEntries, OpenGLShaderProgram* program);
	void BindResourceArrays(const std::vector<ResourceArrayBindingInfo*>& bindingEntries, OpenGLShaderProgram* program, bool performReset);
	void UnbindTextures(const std::vector<ITextureBindingInfo*>& bindingEntries, OpenGLShaderProgram* program);
	void UnbindStateBuffers(const std::vector<StateBufferBindingInfo*>& bindingEntries, OpenGLShaderProgram* program);
	void UnbindResourceArrays(const std::vector<ResourceArrayBindingInfo*>& bindingEntries, OpenGLShaderProgram* program);

	// Debug log methods
	static void GLAPIENTRY MessageCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam);
	void MessageCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message) const;

	// Rendering context methods
	void DeleteRenderingContext(size_t renderingContextIndex);
	void InitializeDefaultRenderContextState() const;

private:
	// Constants
	static const GLuint RenderMarkerId = 1000;

private:
	cobalt::logging::ILogger::unique_ptr _log;
#ifdef __linux__
	std::string _renderNodePath;
#ifdef OPENGL_USE_EGL
	int _gbmDeviceFileHandle = -1;
	gbm_device* _gbmDevice = nullptr;
#endif
#endif
	mutable std::mutex _renderThreadMutex;
	mutable std::mutex _buildStateMutex;
	mutable std::condition_variable _notifyRenderThreadTaskPending;
	mutable std::condition_variable _notifyRenderThreadTaskComplete;
	std::condition_variable _notifyRenderThreadStopped;
	std::thread::id _renderThreadID;
	bool _usingSoftwareRenderer = false;
	bool _renderThreadActive = false;
	bool _frameAdvanceInProgress = false;
	bool _buildingInProgress = false;
	bool _drawingInProgress = false;
	bool _drawingComplete = false;
	bool _buildToDrawRequestPending = false;
	bool _renderRequestPending = false;
	bool _swapRequestPending = false;
	bool _deleteLastDrawResourcesRequestPending = false;
	mutable bool _earlyDeleteNextDrawResourcesRequestPending = false;
	bool _initializeOpenGLRequestPending = false;
	bool _initializeOpenGLResult = false;
	bool _finishWhenSwitchingContexts = false;
	bool _flushFramebufferTextureWritesBeforeWindowSampling = false;
	bool _enableDebugLogging = false;
	bool _captureTargetsPresent = false;
	bool _useRenderMarkers = false;
#ifdef OPENGL_USE_PLATFORM_WIN32
	HWND _dummyMainWindowHandle = nullptr;
	HDC _deviceContext = nullptr;
	HGLRC _mainRenderingContext = nullptr;
#elif defined(OPENGL_USE_EGL)
	EGLDisplay _mainDisplay = EGL_NO_DISPLAY;
	EGLSurface _dummySurface = EGL_NO_SURFACE;
	EGLContext _mainRenderingContext = EGL_NO_CONTEXT;
#elif defined(OPENGL_USE_PLATFORM_XLIB)
	::Display* _xDisplay = nullptr;
	::Window _dummyWindow = 0;
	Colormap _dummyWindowColormap = {};
	GLXContext _mainRenderingContext = nullptr;
#elif defined(OPENGL_USE_PLATFORM_APPKIT)
	mutable std::mutex _renderingContextMutex;
	CGLContextObj _mainRenderingContext = nullptr;
#endif
	std::variant<std::monostate
#if defined(OPENGL_USE_PLATFORM_WIN32) || defined(OPENGL_USE_EGL)
	             ,
	             WindowSystemInfoHeadless
#endif
#ifdef COBALT_RENDERER_WIN32_SUPPORT
	             ,
	             WindowSystemInfoWin32
#endif
#ifdef COBALT_RENDERER_XLIB_SUPPORT
	             ,
	             WindowSystemInfoXlib
#endif
#ifdef OPENGL_USE_EGL
#ifdef COBALT_RENDERER_XCB_SUPPORT
	             ,
	             WindowSystemInfoXCB
#endif
#ifdef COBALT_RENDERER_WAYLAND_SUPPORT
	             ,
	             WindowSystemInfoWayland
#endif
#endif
#ifdef COBALT_RENDERER_APPKIT_SUPPORT
	             ,
	             WindowSystemInfoAppKit
#endif
	             >
	  _windowSystemInfo;
	PixelFormatInfo _dummyWindowPixelFormatInfo = {};
	size_t _currentRenderingContextIndex = 0;
	std::vector<RenderingContextInfo> _allocatedRenderingContexts;
	std::set<IGraphicsDevice::Feature> _enabledFeatures;
	std::set<Options> _enabledOptions;
	std::vector<OpenGLFrameBuffer*> _boundWindowFramebuffers;
	std::vector<OpenGLFrameBuffer*> _boundTextureFramebuffers;
	std::vector<OpenGLDataArray*> _boundDataArrays;
	std::vector<OpenGLTexelArray*> _boundTexelArrays;
	std::vector<OpenGLFrameBufferOutput*> _capturedFramebufferOutputsInCurrentFrame;
	std::vector<OpenGLDataArrayOutput*> _capturedDataArrayOutputsInCurrentFrame;
	std::vector<OpenGLTexelArrayOutput*> _capturedTexelArrayOutputsInCurrentFrame;
	std::vector<ContainerObjectsPendingRelease> _containerObjectsReleaseHelpers;
	int _maxUniformBlockSize = 0;
	int _uniformBufferOffsetAlignment = 0;
	int _maxVertexShaderUniformBlocks = 0;
	int _maxFragmentShaderUniformBlocks = 0;
	int _maxGeometryShaderUniformBlocks = 0;
	int _maxComputeShaderUniformBlocks = 0;
	float _maxTextureSamplerAnisotropy = 1.0f;
	unsigned int _buildIndex;
	unsigned int _drawIndex;
	MutableState _state[2];
	std::queue<std::function<void()>> _pendingInvocations;
	std::unordered_set<std::string> _availableExtensions;
};

} // namespace cobalt::graphics
#include "OpenGLRenderer.inl"
