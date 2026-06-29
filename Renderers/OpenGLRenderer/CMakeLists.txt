# Specify our source files
set(OpenGL3Renderer_SOURCES
    "glad3.3/src/glad.cpp"
    "BindingHelpers.cpp"
    "OpenGL3RendererFactory.cpp"
    "OpenGLDefaultState.cpp"
    "OpenGLGraphicsDevice.cpp"
    "OpenGLGraphicsDeviceEnumerator.cpp"
    "OpenGLFrameBuffer.cpp"
    "OpenGLFrameBufferOutput.cpp"
    "OpenGLRenderer.cpp"
    "OpenGLStateGroupNode.cpp"
    "OpenGLIndexBuffer.cpp"
    "OpenGLProgramNode.cpp"
    "OpenGLRenderableNode.cpp"
    "OpenGLRenderPassNode.cpp"
    "OpenGLShaderProgram.cpp"
    "OpenGLStateBuffer.cpp"
    "OpenGLStateBufferLayout.cpp"
    "OpenGLTransferBatch.cpp"
    "OpenGLVertexBuffer.cpp"
)
set(OpenGL3Renderer_HEADERS
    "BindingHelpers.h"
    "BindingHelpers.inl"
    "OpenGL3RendererFactory.h"
    "OpenGLDebug.h"
    "OpenGLDefaultState.h"
    "OpenGLFrameBuffer.h"
    "OpenGLFrameBufferOutput.h"
    "OpenGLGraphicsDevice.h"
    "OpenGLGraphicsDeviceEnumerator.h"
    "OpenGLHeaders.h"
    "OpenGLNode.h"
    "OpenGLNode.inl"
    "OpenGLProgramNode.h"
    "OpenGLRenderer.h"
    "OpenGLRenderer.inl"
    "OpenGLIndexBuffer.h"
    "OpenGLRenderableNode.h"
    "OpenGLRenderPassNode.h"
    "OpenGLShaderProgram.h"
    "OpenGLStateBuffer.h"
    "OpenGLStateBufferLayout.h"
    "OpenGLStateContainer.h"
    "OpenGLStateContainer.inl"
    "OpenGLStateGroupNode.h"
    "OpenGLTextureBuffer.h"
    "OpenGLTextureBuffer.inl"
    "OpenGLTextureBuffer1D.h"
    "OpenGLTextureBuffer1D.inl"
    "OpenGLTextureBuffer1DArray.h"
    "OpenGLTextureBuffer1DArray.inl"
    "OpenGLTextureBuffer2D.h"
    "OpenGLTextureBuffer2D.inl"
    "OpenGLTextureBuffer2DArray.h"
    "OpenGLTextureBuffer2DArray.inl"
    "OpenGLTextureBuffer3D.h"
    "OpenGLTextureBuffer3D.inl"
    "OpenGLTextureBufferCube.h"
    "OpenGLTextureBufferCube.inl"
    "OpenGLTextureBufferCubeArray.h"
    "OpenGLTextureBufferCubeArray.inl"
    "OpenGLTextureSampler.h"
    "OpenGLTextureSampler.inl"
    "OpenGLTextureSampler1D.h"
    "OpenGLTextureSampler1D.inl"
    "OpenGLTextureSampler1DArray.h"
    "OpenGLTextureSampler1DArray.inl"
    "OpenGLTextureSampler2D.h"
    "OpenGLTextureSampler2D.inl"
    "OpenGLTextureSampler2DArray.h"
    "OpenGLTextureSampler2DArray.inl"
    "OpenGLTextureSampler3D.h"
    "OpenGLTextureSampler3D.inl"
    "OpenGLTextureSamplerCube.h"
    "OpenGLTextureSamplerCube.inl"
    "OpenGLTextureSamplerCubeArray.h"
    "OpenGLTextureSamplerCubeArray.inl"
    "OpenGLTransferBatch.h"
    "OpenGLVertexBuffer.h"
)
set(OpenGL4Renderer_SOURCES
    "glad4.3/src/glad.cpp"
    "BindingHelpers.cpp"
    "OpenGL4RendererFactory.cpp"
    "OpenGLDataArrayOutput.cpp"
    "OpenGLDefaultState.cpp"
    "OpenGLFrameBuffer.cpp"
    "OpenGLFrameBufferOutput.cpp"
    "OpenGLGraphicsDevice.cpp"
    "OpenGLGraphicsDeviceEnumerator.cpp"
    "OpenGLRenderer.cpp"
    "OpenGLDataArray.cpp"
    "OpenGLStateGroupNode.cpp"
    "OpenGLIndexBuffer.cpp"
    "OpenGLProgramNode.cpp"
    "OpenGLRenderableNode.cpp"
    "OpenGLRenderPassNode.cpp"
    "OpenGLShaderProgram.cpp"
    "OpenGLStateBuffer.cpp"
    "OpenGLStateBufferLayout.cpp"
    "OpenGLTexelArray.cpp"
    "OpenGLTexelArrayOutput.cpp"
    "OpenGLTransferBatch.cpp"
    "OpenGLVertexBuffer.cpp"
)
set(OpenGL4Renderer_HEADERS
    "BindingHelpers.h"
    "BindingHelpers.inl"
    "OpenGL4RendererFactory.h"
    "OpenGLDataArray.h"
    "OpenGLDataArrayOutput.h"
    "OpenGLDebug.h"
    "OpenGLDefaultState.h"
    "OpenGLFrameBuffer.h"
    "OpenGLFrameBufferOutput.h"
    "OpenGLGraphicsDevice.h"
    "OpenGLGraphicsDeviceEnumerator.h"
    "OpenGLHeaders.h"
    "OpenGLIndexBuffer.h"
    "OpenGLNode.h"
    "OpenGLNode.inl"
    "OpenGLProgramNode.h"
    "OpenGLRenderer.h"
    "OpenGLRenderer.inl"
    "OpenGLRenderableNode.h"
    "OpenGLRenderPassNode.h"
    "OpenGLShaderProgram.h"
    "OpenGLStateGroupNode.h"
    "OpenGLStateBuffer.h"
    "OpenGLStateBufferLayout.h"
    "OpenGLStateContainer.h"
    "OpenGLStateContainer.inl"
    "OpenGLTexelArray.h"
    "OpenGLTexelArrayOutput.h"
    "OpenGLTextureBuffer.h"
    "OpenGLTextureBuffer.inl"
    "OpenGLTextureBuffer1D.h"
    "OpenGLTextureBuffer1D.inl"
    "OpenGLTextureBuffer1DArray.h"
    "OpenGLTextureBuffer1DArray.inl"
    "OpenGLTextureBuffer2D.h"
    "OpenGLTextureBuffer2D.inl"
    "OpenGLTextureBuffer2DArray.h"
    "OpenGLTextureBuffer2DArray.inl"
    "OpenGLTextureBuffer3D.h"
    "OpenGLTextureBuffer3D.inl"
    "OpenGLTextureBufferCube.h"
    "OpenGLTextureBufferCube.inl"
    "OpenGLTextureBufferCubeArray.h"
    "OpenGLTextureBufferCubeArray.inl"
    "OpenGLTextureSampler.h"
    "OpenGLTextureSampler.inl"
    "OpenGLTextureSampler1D.h"
    "OpenGLTextureSampler1D.inl"
    "OpenGLTextureSampler1DArray.h"
    "OpenGLTextureSampler1DArray.inl"
    "OpenGLTextureSampler2D.h"
    "OpenGLTextureSampler2D.inl"
    "OpenGLTextureSampler2DArray.h"
    "OpenGLTextureSampler2DArray.inl"
    "OpenGLTextureSampler3D.h"
    "OpenGLTextureSampler3D.inl"
    "OpenGLTextureSamplerCube.h"
    "OpenGLTextureSamplerCube.inl"
    "OpenGLTextureSamplerCubeArray.h"
    "OpenGLTextureSamplerCubeArray.inl"
    "OpenGLTransferBatch.h"
    "OpenGLVertexBuffer.h"
)

# Override some files as objective C++ for Cocoa support on macos
if (APPLE)
    set_source_files_properties(OpenGLFrameBuffer.cpp PROPERTIES LANGUAGE OBJCXX)
endif()

# Define our project targets
add_library(OpenGL3Renderer SHARED ${OpenGL3Renderer_SOURCES} ${OpenGL3Renderer_HEADERS} ${COBALT_VERSION_FILES})
add_library(Cobalt::OpenGL3Renderer ALIAS OpenGL3Renderer)
register_clang_format_and_tidy_target(OpenGL3Renderer)
cobalt_apply_compiler_warning_settings(OpenGL3Renderer)
exclude_clang_format_files("${CMAKE_CURRENT_SOURCE_DIR}/glad3.3/src/glad.cpp")
exclude_clang_tidy_files("${CMAKE_CURRENT_SOURCE_DIR}/glad3.3/src/glad.cpp")
add_rpath(OpenGL3Renderer ".")

# Set custom compilation/linker settings
target_compile_definitions(OpenGL3Renderer PRIVATE OPENGL_VERSION_MAJOR=3 OPENGL_VERSION_MINOR=3)
if(UNIX AND NOT APPLE AND COBALT_OPENGL_USE_EGL)
    target_compile_definitions(OpenGL3Renderer PRIVATE OPENGL_USE_EGL)
endif()
target_include_directories(OpenGL3Renderer PRIVATE "glad3.3/include")
target_include_directories(OpenGL3Renderer INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_include_directories(OpenGL3Renderer PRIVATE ${COBALT_VERSION_INFO_DIR})

# Specify our project dependencies
target_link_libraries(OpenGL3Renderer
    PRIVATE
        Cobalt::RendererInterface
        Cobalt::RendererSupport
        Cobalt::ShaderSupport
        Cobalt::TextureFormatConversion
        OpenGLRegistry
)
if(WIN32)
    target_link_libraries(OpenGL3Renderer
        PRIVATE
            opengl32
    )
elseif(APPLE)
    target_link_libraries(OpenGL3Renderer
        PRIVATE
            "-framework OpenGL"
            "-framework Cocoa"
    )
else()
    target_link_libraries(OpenGL3Renderer
        PRIVATE
            GL
            X11
    )
    if(COBALT_OPENGL_USE_EGL)
        target_link_libraries(OpenGL3Renderer
            PRIVATE
                EGL
                xcb
                wayland-client
                wayland-egl
                gbm
        )
    endif()
endif()

# Transfer required files to their output locations
cobalt_configure_sdk_install(OpenGL3Renderer)
cobalt_install_sdk_headers("Renderers" "OpenGL3RendererFactory.h")

# Define our virtual project folder and file structure
set_target_properties(OpenGL3Renderer PROPERTIES FOLDER "Renderers")

# Only include OpenGL 4.3 renderer for non-Apple targets. OpenGL support is capped at 4.1 on macOS, with no geometry or
# compute shader support.
if(NOT APPLE)
    # Define our project targets
    add_library(OpenGL4Renderer SHARED ${OpenGL4Renderer_SOURCES} ${OpenGL4Renderer_HEADERS} ${COBALT_VERSION_FILES})
    add_library(Cobalt::OpenGL4Renderer ALIAS OpenGL4Renderer)
    register_clang_format_and_tidy_target(OpenGL4Renderer)
    cobalt_apply_compiler_warning_settings(OpenGL4Renderer)
    exclude_clang_format_files("${CMAKE_CURRENT_SOURCE_DIR}/glad4.3/src/glad.cpp")
    exclude_clang_tidy_files("${CMAKE_CURRENT_SOURCE_DIR}/glad4.3/src/glad.cpp")
    add_rpath(OpenGL4Renderer ".")

    # Set custom compilation/linker settings
    target_compile_definitions(OpenGL4Renderer PRIVATE OPENGL_VERSION_MAJOR=4 OPENGL_VERSION_MINOR=3)
    if(UNIX AND NOT APPLE AND COBALT_OPENGL_USE_EGL)
        target_compile_definitions(OpenGL4Renderer PRIVATE OPENGL_USE_EGL)
    endif()
    target_include_directories(OpenGL4Renderer PRIVATE "glad4.3/include")
    target_include_directories(OpenGL4Renderer INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/..)
    target_include_directories(OpenGL4Renderer PRIVATE ${COBALT_VERSION_INFO_DIR})

    # Specify our project dependencies
    target_link_libraries(OpenGL4Renderer
        PRIVATE
            Cobalt::RendererInterface
            Cobalt::RendererSupport
            Cobalt::ShaderSupport
            Cobalt::TextureFormatConversion
            OpenGLRegistry
    )
    if(WIN32)
        target_link_libraries(OpenGL4Renderer
            PRIVATE
                opengl32
        )
    else()
        target_link_libraries(OpenGL4Renderer
            PRIVATE
                GL
                X11
        )
        if(COBALT_OPENGL_USE_EGL)
            target_link_libraries(OpenGL4Renderer
                PRIVATE
                    EGL
                    xcb
                    wayland-client
                    wayland-egl
                    gbm
            )
        endif()
    endif()

    # Transfer required files to their output locations
    cobalt_configure_sdk_install(OpenGL4Renderer)
    cobalt_install_sdk_headers("Renderers" "OpenGL4RendererFactory.h")

    # Define our virtual project folder and file structure
    set_target_properties(OpenGL4Renderer PROPERTIES FOLDER "Renderers")
endif()

# Define our virtual project folder and file structure
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${OpenGL3Renderer_SOURCES} ${OpenGL3Renderer_HEADERS})
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${OpenGL4Renderer_SOURCES} ${OpenGL4Renderer_HEADERS})
make_paths_relative(COBALT_VERSION_FILES_RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${COBALT_VERSION_FILES})
source_group("Autogenerated" FILES ${COBALT_VERSION_FILES_RELATIVE})
source_group("Render Tree/Binding Helpers" FILES
    "BindingHelpers.cpp"
    "BindingHelpers.h"
    "BindingHelpers.inl"
)
source_group("Render Tree/OpenGLDefaultState" FILES
    "OpenGLDefaultState.cpp"
    "OpenGLDefaultState.h"
)
source_group("Render Tree/OpenGLNode" FILES
    "OpenGLNode.h"
    "OpenGLNode.inl"
)
source_group("Render Tree/OpenGLProgramNode" FILES
    "OpenGLProgramNode.cpp"
    "OpenGLProgramNode.h"
)
source_group("Render Tree/OpenGLRenderPassNode" FILES
    "OpenGLRenderPassNode.cpp"
    "OpenGLRenderPassNode.h"
)
source_group("Render Tree/OpenGLRenderableNode" FILES
    "OpenGLRenderableNode.cpp"
    "OpenGLRenderableNode.h"
)
source_group("Render Tree/OpenGLStateContainer" FILES
    "OpenGLStateContainer.h"
    "OpenGLStateContainer.inl"
)
source_group("Render Tree/OpenGLStateGroupNode" FILES
    "OpenGLStateGroupNode.cpp"
    "OpenGLStateGroupNode.h"
)
source_group("Renderer/OpenGLGraphicsDevice" FILES
    "OpenGLGraphicsDevice.cpp"
    "OpenGLGraphicsDevice.h"
)
source_group("Renderer/OpenGLGraphicsDeviceEnumerator" FILES
    "OpenGLGraphicsDeviceEnumerator.cpp"
    "OpenGLGraphicsDeviceEnumerator.h"
)
source_group("Renderer/OpenGLRenderer" FILES
    "OpenGLRenderer.cpp"
    "OpenGLRenderer.h"
    "OpenGLRenderer.inl"
)
source_group("Resources/Batching" FILES
    "OpenGLTransferBatch.cpp"
    "OpenGLTransferBatch.h"
)
source_group("Resources/Data/OpenGLDataArray" FILES
    "OpenGLDataArray.cpp"
    "OpenGLDataArray.h"
)
source_group("Resources/Data/OpenGLDataArrayOutput" FILES
    "OpenGLDataArrayOutput.cpp"
    "OpenGLDataArrayOutput.h"
)
source_group("Resources/Data/OpenGLStateBuffer" FILES
    "OpenGLStateBuffer.cpp"
    "OpenGLStateBuffer.h"
)
source_group("Resources/Data/OpenGLStateBufferLayout" FILES
    "OpenGLStateBufferLayout.cpp"
    "OpenGLStateBufferLayout.h"
)
source_group("Resources/Data/OpenGLTexelArray" FILES
    "OpenGLTexelArray.cpp"
    "OpenGLTexelArray.h"
)
source_group("Resources/Data/OpenGLTexelArrayOutput" FILES
    "OpenGLTexelArrayOutput.cpp"
    "OpenGLTexelArrayOutput.h"
)
source_group("Resources/FrameBuffers/OpenGLFrameBuffer" FILES
    "OpenGLFrameBuffer.cpp"
    "OpenGLFrameBuffer.h"
)
source_group("Resources/FrameBuffers/OpenGLFrameBufferOutput" FILES
    "OpenGLFrameBufferOutput.cpp"
    "OpenGLFrameBufferOutput.h"
)
source_group("Resources/Geometry/OpenGLIndexBuffer" FILES
    "OpenGLIndexBuffer.cpp"
    "OpenGLIndexBuffer.h"
)
source_group("Resources/Geometry/OpenGLVertexBuffer" FILES
    "OpenGLVertexBuffer.cpp"
    "OpenGLVertexBuffer.h"
)
source_group("Resources/Images/OpenGLTextureBuffer" FILES
    "OpenGLTextureBuffer.h"
    "OpenGLTextureBuffer.inl"
)
source_group("Resources/Images/OpenGLTextureBuffer1D" FILES
    "OpenGLTextureBuffer1D.h"
    "OpenGLTextureBuffer1D.inl"
)
source_group("Resources/Images/OpenGLTextureBuffer1DArray" FILES
    "OpenGLTextureBuffer1DArray.h"
    "OpenGLTextureBuffer1DArray.inl"
)
source_group("Resources/Images/OpenGLTextureBuffer2D" FILES
    "OpenGLTextureBuffer2D.h"
    "OpenGLTextureBuffer2D.inl"
)
source_group("Resources/Images/OpenGLTextureBuffer2DArray" FILES
    "OpenGLTextureBuffer2DArray.h"
    "OpenGLTextureBuffer2DArray.inl"
)
source_group("Resources/Images/OpenGLTextureBuffer3D" FILES
    "OpenGLTextureBuffer3D.h"
    "OpenGLTextureBuffer3D.inl"
)
source_group("Resources/Images/OpenGLTextureBufferCube" FILES
    "OpenGLTextureBufferCube.h"
    "OpenGLTextureBufferCube.inl"
)
source_group("Resources/Images/OpenGLTextureBufferCubeArray" FILES
    "OpenGLTextureBufferCubeArray.h"
    "OpenGLTextureBufferCubeArray.inl"
)
source_group("Resources/Programs/OpenGLShaderProgram" FILES
    "OpenGLShaderProgram.cpp"
    "OpenGLShaderProgram.h"
)
source_group("Resources/Samplers/OpenGLTextureSampler" FILES
    "OpenGLTextureSampler.h"
    "OpenGLTextureSampler.inl"
)
source_group("Resources/Samplers/OpenGLTextureSampler1D" FILES
    "OpenGLTextureSampler1D.h"
    "OpenGLTextureSampler1D.inl"
)
source_group("Resources/Samplers/OpenGLTextureSampler1DArray" FILES
    "OpenGLTextureSampler1DArray.h"
    "OpenGLTextureSampler1DArray.inl"
)
source_group("Resources/Samplers/OpenGLTextureSampler2D" FILES
    "OpenGLTextureSampler2D.h"
    "OpenGLTextureSampler2D.inl"
)
source_group("Resources/Samplers/OpenGLTextureSampler2DArray" FILES
    "OpenGLTextureSampler2DArray.h"
    "OpenGLTextureSampler2DArray.inl"
)
source_group("Resources/Samplers/OpenGLTextureSampler3D" FILES
    "OpenGLTextureSampler3D.h"
    "OpenGLTextureSampler3D.inl"
)
source_group("Resources/Samplers/OpenGLTextureSamplerCube" FILES
    "OpenGLTextureSamplerCube.h"
    "OpenGLTextureSamplerCube.inl"
)
source_group("Resources/Samplers/OpenGLTextureSamplerCubeArray" FILES
    "OpenGLTextureSamplerCubeArray.h"
    "OpenGLTextureSamplerCubeArray.inl"
)
