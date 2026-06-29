# Specify our source files
set(CBindings_SOURCES
    "DataArray.cpp"
    "DataArrayOutput.cpp"
    "DefaultState.cpp"
    "FrameBuffer.cpp"
    "FrameBufferOutput.cpp"
    "GraphicsDevice.cpp"
    "GraphicsDeviceEnumerator.cpp"
    "IndexAttribute.cpp"
    "IndexBuffer.cpp"
    "ProgramNode.cpp"
    "RenderableNode.cpp"
    "Renderer.cpp"
    "RendererPlugin.cpp"
    "RenderPassNode.cpp"
    "ResourceArray.cpp"
    "ShaderProgram.cpp"
    "StateBuffer.cpp"
    "StateBufferLayout.cpp"
    "StateContainer.cpp"
    "StateGroupNode.cpp"
    "TexelArray.cpp"
    "TexelArrayOutput.cpp"
    "TextureBuffer.cpp"
    "TextureBuffer1D.cpp"
    "TextureBuffer1DArray.cpp"
    "TextureBuffer2D.cpp"
    "TextureBuffer2DArray.cpp"
    "TextureBuffer3D.cpp"
    "TextureBufferCube.cpp"
    "TextureBufferCubeArray.cpp"
    "TextureSampler1D.cpp"
    "TextureSampler1DArray.cpp"
    "TextureSampler2D.cpp"
    "TextureSampler2DArray.cpp"
    "TextureSampler3D.cpp"
    "TextureSamplerCube.cpp"
    "TextureSamplerCubeArray.cpp"
    "TransferBatch.cpp"
    "VertexAttribute.cpp"
    "VertexBuffer.cpp"
)
set(CBindings_HEADERS
    "CBindings.pkg"
    "PlatformBindings.pkg"
    "Macros.h"
    "DataArray.h"
    "DataArrayOutput.h"
    "DefaultState.h"
    "FrameBuffer.h"
    "FrameBufferOutput.h"
    "GraphicsDevice.h"
    "GraphicsDeviceEnumerator.h"
    "IndexAttribute.h"
    "IndexBuffer.h"
    "ProgramNode.h"
    "RenderableNode.h"
    "Renderer.h"
    "RendererPlugin.h"
    "RenderPassNode.h"
    "ResourceArray.h"
    "Result.h"
    "ShaderProgram.h"
    "ShaderSourceInfoAIR.h"
    "ShaderSourceInfoBase.h"
    "ShaderSourceInfoDXBC.h"
    "ShaderSourceInfoDXIL.h"
    "ShaderSourceInfoGLSL.h"
    "ShaderSourceInfoHLSL.h"
    "ShaderSourceInfoMSL.h"
    "ShaderSourceInfoSPIRV.h"
    "ShaderSourceInfoSPIRVAssembly.h"
    "ShaderTargetInfoBase.h"
    "ShaderTargetInfoDirect3D.h"
    "ShaderTargetInfoOpenGL.h"
    "StateBuffer.h"
    "StateBufferLayout.h"
    "StateContainer.h"
    "StateGroupNode.h"
    "TexelArray.h"
    "TexelArrayOutput.h"
    "TextureBuffer.h"
    "TextureBuffer1D.h"
    "TextureBuffer1DArray.h"
    "TextureBuffer2D.h"
    "TextureBuffer2DArray.h"
    "TextureBuffer3D.h"
    "TextureBufferCube.h"
    "TextureBufferCubeArray.h"
    "TextureSampler.h"
    "TextureSampler1D.h"
    "TextureSampler1DArray.h"
    "TextureSampler2D.h"
    "TextureSampler2DArray.h"
    "TextureSampler3D.h"
    "TextureSamplerCube.h"
    "TextureSamplerCubeArray.h"
    "TransferBatch.h"
    "VertexAttribute.h"
    "VertexBuffer.h"
    "WindowSystemInfoBase.h"
    "WindowSystemInfoHeadless.h"
    "WindowSystemInfoWin32.h"
    "WindowSystemInfoXlib.h"
    "WindowSystemInfoXCB.h"
    "WindowSystemInfoWayland.h"
    "WindowSystemInfoAppKit.h"
    "WindowInfoBase.h"
    "WindowInfoHeadless.h"
    "WindowInfoWin32.h"
    "WindowInfoXlib.h"
    "WindowInfoXCB.h"
    "WindowInfoWayland.h"
    "WindowInfoAppKit.h"
)
set_clang_tidy_header_files_as_c(${CBindings_HEADERS})

# Define our project targets
add_library(CobaltCBindings SHARED ${CBindings_SOURCES} ${CBindings_HEADERS})
add_library(Cobalt::CobaltCBindings ALIAS CobaltCBindings)
add_library(CobaltCBindingsStatic STATIC ${CBindings_SOURCES} ${CBindings_HEADERS})
add_library(Cobalt::CobaltCBindingsStatic ALIAS CobaltCBindingsStatic)
register_clang_format_and_tidy_target(CobaltCBindings)
cobalt_apply_compiler_warning_settings(CobaltCBindings)
target_compile_definitions(CobaltCBindings PRIVATE BUILDING_COBALT_CBINDINGS=1)
target_include_directories(CobaltCBindings PUBLIC
    "${COBALT_INTERNAL_INCLUDE_DIR}"
    "${COBALT_VERSION_INFO_DIR}"
    "${COBALT_RENDERER_INTERFACE_GENERATED_DIR}"
)
cobalt_apply_compiler_warning_settings(CobaltCBindingsStatic)
target_compile_definitions(CobaltCBindingsStatic PUBLIC COBALT_CBINDINGS_STATIC=1)
set_target_properties(CobaltCBindingsStatic PROPERTIES
    INTERPROCEDURAL_OPTIMIZATION FALSE
    INTERPROCEDURAL_OPTIMIZATION_RELEASE FALSE
    INTERPROCEDURAL_OPTIMIZATION_RELWITHDEBINFO FALSE
)
target_include_directories(CobaltCBindingsStatic PUBLIC
    "${COBALT_INTERNAL_INCLUDE_DIR}"
    "${COBALT_VERSION_INFO_DIR}"
    "${COBALT_RENDERER_INTERFACE_GENERATED_DIR}"
)

# Specify our project dependencies
target_link_libraries(CobaltCBindings
    PRIVATE
        Cobalt::RendererInterface
)
target_link_libraries(CobaltCBindingsStatic
    PRIVATE
        Cobalt::RendererInterface
)

# Transfer required files to their output locations
cobalt_configure_sdk_install(CobaltCBindings)
cobalt_configure_sdk_install(CobaltCBindingsStatic)
cobalt_install_sdk_headers(CBindings ${CBindings_HEADERS})

# Define our virtual project folder and file structure
set_target_properties(CobaltCBindings CobaltCBindingsStatic PROPERTIES FOLDER "Libraries/Public")
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${CBindings_SOURCES} ${CBindings_HEADERS})
source_group("Header Files" FILES
    "CBindings.pkg"
    "PlatformBindings.pkg"
    "Macros.h"
    "Result.h"
)
source_group("RenderTree" FILES
    "DefaultState.cpp"
    "DefaultState.h"
    "ProgramNode.cpp"
    "ProgramNode.h"
    "RenderPassNode.cpp"
    "RenderPassNode.h"
    "RenderableNode.cpp"
    "RenderableNode.h"
    "StateContainer.cpp"
    "StateContainer.h"
    "StateGroupNode.cpp"
    "StateGroupNode.h"
)
source_group("Renderer" FILES
    "GraphicsDevice.cpp"
    "GraphicsDevice.h"
    "GraphicsDeviceEnumerator.cpp"
    "GraphicsDeviceEnumerator.h"
    "Renderer.cpp"
    "Renderer.h"
    "RendererPlugin.cpp"
    "RendererPlugin.h"
)
source_group("Resources/Batching" FILES
    "TransferBatch.cpp"
    "TransferBatch.h"
)
source_group("Resources/Data" FILES
    "DataArray.cpp"
    "DataArray.h"
    "DataArrayOutput.cpp"
    "DataArrayOutput.h"
    "ResourceArray.cpp"
    "ResourceArray.h"
    "StateBuffer.cpp"
    "StateBuffer.h"
    "StateBufferLayout.cpp"
    "StateBufferLayout.h"
    "TexelArray.cpp"
    "TexelArray.h"
    "TexelArrayOutput.cpp"
    "TexelArrayOutput.h"
)
source_group("Resources/FrameBuffers" FILES
    "FrameBuffer.cpp"
    "FrameBuffer.h"
    "FrameBufferOutput.cpp"
    "FrameBufferOutput.h"
)
source_group("Resources/Geometry" FILES
    "IndexAttribute.cpp"
    "IndexAttribute.h"
    "IndexBuffer.cpp"
    "IndexBuffer.h"
    "VertexAttribute.cpp"
    "VertexAttribute.h"
    "VertexBuffer.cpp"
    "VertexBuffer.h"
)
source_group("Resources/Programs" FILES
    "ShaderProgram.cpp"
    "ShaderProgram.h"
)
source_group("Resources/Programs/ShaderSourceInfo" FILES
    "ShaderSourceInfoAIR.h"
    "ShaderSourceInfoBase.h"
    "ShaderSourceInfoDXBC.h"
    "ShaderSourceInfoDXIL.h"
    "ShaderSourceInfoGLSL.h"
    "ShaderSourceInfoHLSL.h"
    "ShaderSourceInfoMSL.h"
    "ShaderSourceInfoSPIRV.h"
    "ShaderSourceInfoSPIRVAssembly.h"
)
source_group("Resources/Programs/ShaderTargetInfo" FILES
    "ShaderTargetInfoBase.h"
    "ShaderTargetInfoDirect3D.h"
    "ShaderTargetInfoOpenGL.h"
)
source_group("Resources/Textures" FILES
    "TextureBuffer.cpp"
    "TextureBuffer.h"
    "TextureBuffer1D.cpp"
    "TextureBuffer1D.h"
    "TextureBuffer1DArray.cpp"
    "TextureBuffer1DArray.h"
    "TextureBuffer2D.cpp"
    "TextureBuffer2D.h"
    "TextureBuffer2DArray.cpp"
    "TextureBuffer2DArray.h"
    "TextureBuffer3D.cpp"
    "TextureBuffer3D.h"
    "TextureBufferCube.cpp"
    "TextureBufferCube.h"
    "TextureBufferCubeArray.cpp"
    "TextureBufferCubeArray.h"
    "TextureSampler.h"
    "TextureSampler1D.cpp"
    "TextureSampler1D.h"
    "TextureSampler1DArray.cpp"
    "TextureSampler1DArray.h"
    "TextureSampler2D.cpp"
    "TextureSampler2D.h"
    "TextureSampler2DArray.cpp"
    "TextureSampler2DArray.h"
    "TextureSampler3D.cpp"
    "TextureSampler3D.h"
    "TextureSamplerCube.cpp"
    "TextureSamplerCube.h"
    "TextureSamplerCubeArray.cpp"
    "TextureSamplerCubeArray.h"
)
source_group("Platform Bindings" FILES
    "WindowSystemInfoBase.h"
    "WindowSystemInfoHeadless.h"
    "WindowSystemInfoWin32.h"
    "WindowSystemInfoXlib.h"
    "WindowSystemInfoXCB.h"
    "WindowSystemInfoWayland.h"
    "WindowSystemInfoAppKit.h"
    "WindowInfoBase.h"
    "WindowInfoHeadless.h"
    "WindowInfoWin32.h"
    "WindowInfoXlib.h"
    "WindowInfoXCB.h"
    "WindowInfoWayland.h"
    "WindowInfoAppKit.h"
)
