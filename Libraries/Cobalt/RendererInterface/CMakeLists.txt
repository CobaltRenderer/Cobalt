# Specify our source files
set(RendererInterface_HEADERS
    "RendererInterface.pkg"
    "PlatformBindings.pkg"
    "Deleter.h"
    "IDataArray.h"
    "IDataArray.inl"
    "IDataArrayOutput.h"
    "IDefaultState.h"
    "IFrameBuffer.h"
    "IFrameBuffer.inl"
    "IFrameBufferOutput.h"
    "IFrameBufferOutput.inl"
    "IGraphicsDevice.h"
    "IGraphicsDeviceEnumerator.h"
    "IGraphicsDeviceEnumerator.inl"
    "IIndexAttribute.h"
    "IIndexAttribute.inl"
    "IIndexBuffer.h"
    "IIndexBuffer.inl"
    "IModuleHandle.h"
    "IndexAttribute.h"
    "IndexAttribute.inl"
    "IProgramNode.h"
    "IProgramNode.inl"
    "IRenderableNode.h"
    "IRenderableNode.inl"
    "IRenderer.h"
    "IRenderer.inl"
    "IRendererPlugin.h"
    "IRenderPassNode.h"
    "IResourceArray.h"
    "IResourceArray.inl"
    "IShaderProgram.h"
    "IStateBuffer.h"
    "IStateBuffer.inl"
    "IStateBufferLayout.h"
    "IStateContainer.h"
    "IStateContainer.inl"
    "IStateGroupNode.h"
    "ITexelArray.h"
    "ITexelArray.inl"
    "ITexelArrayOutput.h"
    "ITexelArrayOutput.inl"
    "ITextureBuffer.h"
    "ITextureBuffer.inl"
    "ITextureBuffer1D.h"
    "ITextureBuffer1D.inl"
    "ITextureBuffer1DArray.h"
    "ITextureBuffer1DArray.inl"
    "ITextureBuffer2D.h"
    "ITextureBuffer2D.inl"
    "ITextureBuffer2DArray.h"
    "ITextureBuffer2DArray.inl"
    "ITextureBuffer3D.h"
    "ITextureBuffer3D.inl"
    "ITextureBufferCube.h"
    "ITextureBufferCube.inl"
    "ITextureBufferCubeArray.h"
    "ITextureBufferCubeArray.inl"
    "ITextureSampler.h"
    "ITextureSampler1D.h"
    "ITextureSampler1DArray.h"
    "ITextureSampler2D.h"
    "ITextureSampler2DArray.h"
    "ITextureSampler3D.h"
    "ITextureSamplerCube.h"
    "ITextureSamplerCubeArray.h"
    "ITransferBatch.h"
    "IVertexAttribute.h"
    "IVertexAttribute.inl"
    "IVertexBuffer.h"
    "IVertexBuffer.inl"
    "MatrixTypes.h"
    "ModuleHandlePosix.h"
    "ModuleHandlePosix.inl"
    "ModuleHandleWin32.h"
    "ModuleHandleWin32.inl"
    "PlatformHelpersSDL3.h"
    "PlatformHelpersSDL3.inl"
    "RawIndexAttribute.h"
    "RawIndexAttribute.inl"
    "RawVertexAttribute.h"
    "RawVertexAttribute.inl"
    "ReadOnlyIndexAttribute.h"
    "ReadOnlyIndexAttribute.inl"
    "ReadOnlyVertexAttribute.h"
    "ReadOnlyVertexAttribute.inl"
    "ReadWriteMutex.h"
    "ReadWriteMutex.inl"
    "RendererPlugin.h"
    "RendererPlugin.inl"
    "RenderPluginEnumerator.h"
    "RenderPluginEnumerator.inl"
    "ShaderSourceInfoBaseAIR.h"
    "ShaderSourceInfoBaseDXBC.h"
    "ShaderSourceInfoBaseDXIL.h"
    "ShaderSourceInfoBaseGLSL.h"
    "ShaderSourceInfoBaseHLSL.h"
    "ShaderSourceInfoBaseMSL.h"
    "ShaderSourceInfoBaseSPIRV.h"
    "ShaderSourceInfoBaseSPIRVAssembly.h"
    "ShaderSourceInfoAIR.h"
    "ShaderSourceInfoDXBC.h"
    "ShaderSourceInfoDXIL.h"
    "ShaderSourceInfoGLSL.h"
    "ShaderSourceInfoHLSL.h"
    "ShaderSourceInfoMSL.h"
    "ShaderSourceInfoSPIRV.h"
    "ShaderSourceInfoSPIRVAssembly.h"
    "ShaderTargetInfoDirect3D.h"
    "ShaderTargetInfoDirect3D.inl"
    "ShaderTargetInfoOpenGL.h"
    "ShaderTargetInfoOpenGL.inl"
    "SuccessToken.h"
    "SuccessToken.inl"
    "Tokens.h"
    "VectorTypes.h"
    "VertexAttribute.h"
    "VertexAttribute.inl"
    "WindowInfoAppKit.h"
    "WindowInfoHeadless.h"
    "WindowInfoSDL3.h"
    "WindowInfoWayland.h"
    "WindowInfoWin32.h"
    "WindowInfoXCB.h"
    "WindowInfoXlib.h"
    "Win32Headers.h"
    "XlibHeaders.h"
    "WindowSystemInfoAppKit.h"
    "WindowSystemInfoHeadless.h"
    "WindowSystemInfoSDL3.h"
    "WindowSystemInfoWayland.h"
    "WindowSystemInfoWin32.h"
    "WindowSystemInfoXCB.h"
    "WindowSystemInfoXlib.h"
)
set(RendererInterface_GENERATED_HEADERS
    "${COBALT_RENDERER_INTERFACE_VERSION_HEADER}"
)
exclude_clang_format_files("${COBALT_RENDERER_INTERFACE_VERSION_HEADER}")
exclude_clang_tidy_files("${COBALT_RENDERER_INTERFACE_VERSION_HEADER}")

# Define our project targets
add_library(RendererInterface_Lib INTERFACE)
add_library(Cobalt::RendererInterface ALIAS RendererInterface_Lib)
target_include_directories(RendererInterface_Lib INTERFACE
    "${COBALT_INTERNAL_INCLUDE_DIR}"
    "${COBALT_VERSION_INFO_DIR}"
    "${COBALT_RENDERER_INTERFACE_GENERATED_DIR}"
)

# Dummy targets to show in the IDE and register for clang-format
add_library(RendererInterface STATIC ${RendererInterface_HEADERS} ${RendererInterface_GENERATED_HEADERS})
register_clang_format_and_tidy_target(RendererInterface USAGE_TARGET Cobalt::RendererInterface)
set_target_properties(RendererInterface PROPERTIES LINKER_LANGUAGE CXX EXCLUDE_FROM_ALL TRUE)
set_target_properties(RendererInterface PROPERTIES FOLDER "Libraries/Public")

# Specify our project dependencies
target_link_libraries(RendererInterface_Lib INTERFACE
    Cobalt::Debug
    Cobalt::LoggingInterface
    Cobalt::Marshalling
    Cobalt::MathTypes
)

# Transfer required files to their output locations
cobalt_install_sdk_headers(RendererInterface ${RendererInterface_HEADERS})
cobalt_install_sdk_headers(RendererInterface ${RendererInterface_GENERATED_HEADERS})

# Define our virtual project folder and file structure
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${RendererInterface_HEADERS})
source_group("Autogenerated" FILES ${RendererInterface_GENERATED_HEADERS})
source_group("Data Types" FILES
    "VectorTypes.h"
    "MatrixTypes.h"
    "Deleter.h"
    "Tokens.h"
)
source_group("Helper Types/ReadWriteMutex" FILES
    "ReadWriteMutex.h"
    "ReadWriteMutex.inl"
)
source_group("Helper Types/SuccessToken" FILES
    "SuccessToken.h"
    "SuccessToken.inl"
)
source_group("Render Tree/IDefaultState" FILES
    "IDefaultState.h"
)
source_group("Render Tree/IProgramNode" FILES
    "IProgramNode.h"
    "IProgramNode.inl"
)
source_group("Render Tree/IRenderPassNode" FILES
    "IRenderPassNode.h"
)
source_group("Render Tree/IRenderableNode" FILES
    "IRenderableNode.h"
    "IRenderableNode.inl"
)
source_group("Render Tree/IStateContainer" FILES
    "IStateContainer.h"
    "IStateContainer.inl"
)
source_group("Render Tree/IStateGroupNode" FILES
    "IStateGroupNode.h"
)
source_group("Renderer/IGraphicsDevice" FILES
    "IGraphicsDevice.h"
)
source_group("Renderer/IGraphicsDeviceEnumerator" FILES
    "IGraphicsDeviceEnumerator.h"
    "IGraphicsDeviceEnumerator.inl"
)
source_group("Renderer/IRenderer" FILES
    "IRenderer.h"
    "IRenderer.inl"
)
source_group("Renderer/IRendererPlugin" FILES
    "IRendererPlugin.h"
)
source_group("Renderer/RendererPlugin" FILES
    "RendererPlugin.h"
    "RendererPlugin.inl"
)
source_group("Resources/Batching" FILES
    "ITransferBatch.h"
)
source_group("Resources/Data/IDataArray" FILES
    "IDataArray.h"
    "IDataArray.inl"
)
source_group("Resources/Data/IDataArrayOutput" FILES
    "IDataArrayOutput.h"
)
source_group("Resources/Data/IResourceArray" FILES
    "IResourceArray.h"
    "IResourceArray.inl"
)
source_group("Resources/Data/IStateBuffer" FILES
    "IStateBuffer.h"
    "IStateBuffer.inl"
)
source_group("Resources/Data/IStateBufferLayout" FILES
    "IStateBufferLayout.h"
)
source_group("Resources/Data/ITexelArray" FILES
    "ITexelArray.h"
    "ITexelArray.inl"
)
source_group("Resources/Data/ITexelArrayOutput" FILES
    "ITexelArrayOutput.h"
    "ITexelArrayOutput.inl"
)
source_group("Resources/FrameBuffers/IFrameBuffer" FILES
    "IFrameBuffer.h"
    "IFrameBuffer.inl"
)
source_group("Resources/FrameBuffers/IFrameBufferOutput" FILES
    "IFrameBufferOutput.h"
    "IFrameBufferOutput.inl"
)
source_group("Resources/Geometry/IIndexAttribute" FILES
    "IIndexAttribute.h"
    "IIndexAttribute.inl"
)
source_group("Resources/Geometry/IIndexBuffer" FILES
    "IIndexBuffer.h"
    "IIndexBuffer.inl"
)
source_group("Resources/Geometry/IVertexAttribute" FILES
    "IVertexAttribute.h"
    "IVertexAttribute.inl"
)
source_group("Resources/Geometry/IVertexBuffer" FILES
    "IVertexBuffer.h"
    "IVertexBuffer.inl"
)
source_group("Resources/Geometry/IndexAttribute" FILES
    "IndexAttribute.h"
    "IndexAttribute.inl"
)
source_group("Resources/Geometry/RawIndexAttribute" FILES
    "RawIndexAttribute.h"
    "RawIndexAttribute.inl"
)
source_group("Resources/Geometry/RawVertexAttribute" FILES
    "RawVertexAttribute.h"
    "RawVertexAttribute.inl"
)
source_group("Resources/Geometry/ReadOnlyIndexAttribute" FILES
    "ReadOnlyIndexAttribute.h"
    "ReadOnlyIndexAttribute.inl"
)
source_group("Resources/Geometry/ReadOnlyVertexAttribute" FILES
    "ReadOnlyVertexAttribute.h"
    "ReadOnlyVertexAttribute.inl"
)
source_group("Resources/Geometry/VertexAttribute" FILES
    "VertexAttribute.h"
    "VertexAttribute.inl"
)
source_group("Resources/Images/ITextureBuffer" FILES
    "ITextureBuffer.h"
    "ITextureBuffer.inl"
)
source_group("Resources/Images/ITextureBuffer1D" FILES
    "ITextureBuffer1D.h"
    "ITextureBuffer1D.inl"
)
source_group("Resources/Images/ITextureBuffer1DArray" FILES
    "ITextureBuffer1DArray.h"
    "ITextureBuffer1DArray.inl"
)
source_group("Resources/Images/ITextureBuffer2D" FILES
    "ITextureBuffer2D.h"
    "ITextureBuffer2D.inl"
)
source_group("Resources/Images/ITextureBuffer2DArray" FILES
    "ITextureBuffer2DArray.h"
    "ITextureBuffer2DArray.inl"
)
source_group("Resources/Images/ITextureBuffer3D" FILES
    "ITextureBuffer3D.h"
    "ITextureBuffer3D.inl"
)
source_group("Resources/Images/ITextureBufferCube" FILES
    "ITextureBufferCube.h"
    "ITextureBufferCube.inl"
)
source_group("Resources/Images/ITextureBufferCubeArray" FILES
    "ITextureBufferCubeArray.h"
    "ITextureBufferCubeArray.inl"
)
source_group("Resources/Programs/IShaderProgram" FILES
    "IShaderProgram.h"
)
source_group("Resources/Programs/ShaderSourceInfo" FILES
    "ShaderSourceInfoBaseAIR.h"
    "ShaderSourceInfoBaseDXBC.h"
    "ShaderSourceInfoBaseDXIL.h"
    "ShaderSourceInfoBaseGLSL.h"
    "ShaderSourceInfoBaseHLSL.h"
    "ShaderSourceInfoBaseMSL.h"
    "ShaderSourceInfoBaseSPIRV.h"
    "ShaderSourceInfoBaseSPIRVAssembly.h"
    "ShaderSourceInfoAIR.h"
    "ShaderSourceInfoDXBC.h"
    "ShaderSourceInfoDXIL.h"
    "ShaderSourceInfoGLSL.h"
    "ShaderSourceInfoHLSL.h"
    "ShaderSourceInfoMSL.h"
    "ShaderSourceInfoSPIRV.h"
    "ShaderSourceInfoSPIRVAssembly.h"
)
source_group("Resources/Programs/ShaderTargetInfo" FILES
    "ShaderTargetInfoDirect3D.h"
    "ShaderTargetInfoDirect3D.inl"
    "ShaderTargetInfoOpenGL.h"
    "ShaderTargetInfoOpenGL.inl"
)
source_group("Resources/Samplers/ITextureSampler" FILES
    "ITextureSampler.h"
)
source_group("Resources/Samplers/ITextureSampler1D" FILES
    "ITextureSampler1D.h"
)
source_group("Resources/Samplers/ITextureSampler1DArray" FILES
    "ITextureSampler1DArray.h"
)
source_group("Resources/Samplers/ITextureSampler2D" FILES
    "ITextureSampler2D.h"
)
source_group("Resources/Samplers/ITextureSampler2DArray" FILES
    "ITextureSampler2DArray.h"
)
source_group("Resources/Samplers/ITextureSampler3D" FILES
    "ITextureSampler3D.h"
)
source_group("Resources/Samplers/ITextureSamplerCube" FILES
    "ITextureSamplerCube.h"
)
source_group("Resources/Samplers/ITextureSamplerCubeArray" FILES
    "ITextureSamplerCubeArray.h"
)
source_group("Platform Bindings" FILES
    "IModuleHandle.h"
    "ModuleHandlePosix.h"
    "ModuleHandlePosix.inl"
    "ModuleHandleWin32.h"
    "ModuleHandleWin32.inl"
    "PlatformHelpersSDL3.h"
    "PlatformHelpersSDL3.inl"
    "RenderPluginEnumerator.h"
    "RenderPluginEnumerator.inl"
    "WindowInfoWin32.h"
    "Win32Headers.h"
    "WindowInfoHeadless.h"
    "WindowInfoXlib.h"
    "WindowInfoXCB.h"
    "WindowInfoWayland.h"
    "WindowInfoAppKit.h"
    "WindowInfoSDL3.h"
    "XlibHeaders.h"
    "WindowSystemInfoHeadless.h"
    "WindowSystemInfoWin32.h"
    "WindowSystemInfoXlib.h"
    "WindowSystemInfoXCB.h"
    "WindowSystemInfoWayland.h"
    "WindowSystemInfoAppKit.h"
    "WindowSystemInfoSDL3.h"
)
