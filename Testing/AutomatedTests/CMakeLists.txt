# Specify our source files
set(AutomatedTests_SOURCES
    "PerformanceTests/BalancedBatchPolyCount.cpp"
    "PerformanceTests/ConstantFrameCaptureBuffer.cpp"
    "PerformanceTests/ConstantFrameCaptureWindow.cpp"
    "PerformanceTests/LargeBatchCount.cpp"
    "PerformanceTests/LargePolyCount.cpp"
    "PerformanceTests/MinimalScene.cpp"
    "UnitTests/AppendStructuredBuffer.cpp"
    "UnitTests/BasicCompute.cpp"
    "UnitTests/BasicScene.cpp"
    "UnitTests/Buffer.cpp"
    "UnitTests/BufferOffsets.cpp"
    "UnitTests/BunnyNormals.cpp"
    "UnitTests/CompressedTextures.cpp"
    "UnitTests/ConstantStateValues.cpp"
    "UnitTests/StateValueArrayBindings.cpp"
    "UnitTests/ConsumeStructuredBuffer.cpp"
    "UnitTests/DataArrayGpuTransfer.cpp"
    "UnitTests/DeviceEnumeration.cpp"
    "UnitTests/DeviceLimitsTests.cpp"
    "UnitTests/FrameBufferAttachmentLifecycle.cpp"
    "UnitTests/FrameBufferMultiSampling.cpp"
    "UnitTests/FrameBufferClearColor.cpp"
    "UnitTests/FrameBufferClearDepth.cpp"
    "UnitTests/FrameBufferClearStencil.cpp"
    "UnitTests/FrameBufferOutputFormatCoverage.cpp"
    "UnitTests/FrameBufferOutputLifecycle.cpp"
    "UnitTests/FrameBufferViewportAndScissor.cpp"
    "UnitTests/IndirectDrawFromCompute.cpp"
    "UnitTests/IndirectDraw.cpp"
    "UnitTests/IndirectMultiDrawFromCompute.cpp"
    "UnitTests/InstanceMode.cpp"
    "UnitTests/ManualVertexBufferLayout.cpp"
    "UnitTests/DynamicGeometryUpdates.cpp"
    "UnitTests/MipmappingTexture1D.cpp"
    "UnitTests/MipmappingTexture2D.cpp"
    "UnitTests/MipmappingTexture3D.cpp"
    "UnitTests/MipmappingTextureCube.cpp"
    "UnitTests/MipmappingTexture1DArray.cpp"
    "UnitTests/MipmappingTexture2DArray.cpp"
    "UnitTests/MipmappingTextureCubeArray.cpp"
    "UnitTests/CobaltCube.cpp"
    "UnitTests/OnGpuCopy.cpp"
    "UnitTests/PrimitiveModes.cpp"
    "UnitTests/PrimitiveRestart.cpp"
    "UnitTests/RawVertexAttributeTest.cpp"
    "UnitTests/RenderableAttributeBinding.cpp"
    "UnitTests/RenderTreeChildCollectionContracts.cpp"
    "UnitTests/RenderPassSortOrder.cpp"
    "UnitTests/RenderTreeLifecycle.cpp"
    "UnitTests/ResourceBindingVariants.cpp"
    "UnitTests/ResourceBindingLifecycle.cpp"
    "UnitTests/RWBuffer.cpp"
    "UnitTests/GeometryShaders.cpp"
    "UnitTests/ShaderReflectionQueries.cpp"
    "UnitTests/ShaderEntryPointSelection.cpp"
    "UnitTests/BlendState.cpp"
    "UnitTests/SeparateBlendState.cpp"
    "UnitTests/DepthState.cpp"
    "UnitTests/PolygonFillMode.cpp"
    "UnitTests/PolygonWindingOrderAndCullMode.cpp"
    "UnitTests/StencilState.cpp"
    "UnitTests/StateBufferArrayBindings.cpp"
    "UnitTests/StateBufferLayoutContracts.cpp"
    "UnitTests/StateBufferPaging.cpp"
    "UnitTests/StateBufferWithNoLayout.cpp"
    "UnitTests/StateValueTypeApplication.cpp"
    "UnitTests/StateValueLifecycle.cpp"
    "UnitTests/StructuredBuffer.cpp"
    "UnitTests/TextureBufferMetadata.cpp"
    "UnitTests/TransferBatchDataArray.cpp"
    "UnitTests/TransferBatchGeometry.cpp"
    "UnitTests/TransferBatchLifecycle.cpp"
    "UnitTests/TransferBatchTexelArray.cpp"
    "UnitTests/TransferBatchTextureBuffer2D.cpp"
    "UnitTests/RWStructuredBuffer.cpp"
    "UnitTests/RGBTriangle.cpp"
    "UnitTests/TexelArrayGpuTransfer.cpp"
    "UnitTests/TextureBuffer1D.cpp"
    "UnitTests/TextureBuffer2D.cpp"
    "UnitTests/TextureBuffer2DArray.cpp"
    "UnitTests/TextureBuffer3D.cpp"
    "UnitTests/TextureUpdateCoverage.cpp"
    "UnitTests/TextureBufferCube.cpp"
    "UnitTests/TextureBuffer1DArray.cpp"
    "UnitTests/TextureBufferCubeArray.cpp"
    "UnitTests/TextureSampler1D.cpp"
    "UnitTests/TextureSampler2D.cpp"
    "UnitTests/TextureSampler3D.cpp"
    "UnitTests/TextureSamplerCube.cpp"
    "UnitTests/TextureSampler1DArray.cpp"
    "UnitTests/TextureSampler2DArray.cpp"
    "UnitTests/TextureSamplerCubeArray.cpp"
    "UnitTests/Transparency.cpp"
    "UnitTests/TrivialDxbc.cpp"
    "UnitTests/TrivialDxil.cpp"
    "UnitTests/TrivialGlsl.cpp"
    "UnitTests/TrivialHlsl.cpp"
    "UnitTests/TrivialMsl.cpp"
    "UnitTests/TrivialSpirvAssembly.cpp"
    "UnitTests/TrivialSpirv.cpp"
    "UnitTests/VertexAttributeTypeConversion.cpp"
    "UnitTests/VertexBufferChangeTest.cpp"
    "UnitTests/WriteIndexBufferFromCompute.cpp"
    "UnitTests/WriteVertexBufferFromCompute.cpp"
    "GeometryHelper.cpp"
    "PerformanceTestBase.cpp"
    "PseudoRandomGenerator.cpp"
    "StanfordBunnyVertices.cpp"
    "StanfordBunnyIndices.cpp"
    "TextureHelper.cpp"
    "TexturedQuadSceneHelper.cpp"
    "TransformHelper.cpp"
    "UnitTestBase.cpp"
    "MipmappingHelper.cpp"
    "main.cpp"
    "TestManager.cpp"
    "TestRegistry.cpp"
    "TestSession.cpp"
    "UtahTeapotVertices.cpp"
)
if (WIN32)
    list(APPEND AutomatedTests_SOURCES "Window.Win32.cpp")
    list(APPEND AutomatedTests_SOURCES "MenuItem.Win32.cpp")
    list(APPEND AutomatedTests_SOURCES "UIThreadInvocation.Win32.cpp")
elseif(APPLE)
    list(APPEND AutomatedTests_SOURCES "Window.Cocoa.mm")
    list(APPEND AutomatedTests_SOURCES "MenuItem.Cocoa.mm")
    list(APPEND AutomatedTests_SOURCES "MenuItem.Posix.cpp")
    list(APPEND AutomatedTests_SOURCES "UIThreadInvocation.Cocoa.cpp")
else()
    list(APPEND AutomatedTests_SOURCES "Window.X11.cpp")
    list(APPEND AutomatedTests_SOURCES "MenuItem.Posix.cpp")
    list(APPEND AutomatedTests_SOURCES "UIThreadInvocation.X11.cpp")
endif()
set(AutomatedTests_HEADERS
    "GeometryHelper.h"
    "IThreadInvocation.h"
    "IThreadInvocation.inl"
    "PerformanceTestBase.h"
    "PseudoRandomGenerator.h"
    "TextureHelper.h"
    "TexturedQuadSceneHelper.h"
    "TransformHelper.h"
    "UnicodeConversion.h"
    "UnitTestBase.h"
    "MipmappingHelper.h"
    "Debug.h"
    "IUnitTest.h"
    "Configuration.h"
    "ITestSession.h"
    "MenuItem.h"
    "StringHelpers.h"
    "TestManager.h"
    "TestRegistry.h"
    "TestSession.h"
    "Window.h"
)
if (WIN32)
    list(APPEND AutomatedTests_HEADERS "UIThreadInvocation.Win32.h")
elseif(APPLE)
    list(APPEND AutomatedTests_HEADERS "UIThreadInvocation.Cocoa.h")
else()
    list(APPEND AutomatedTests_HEADERS "UIThreadInvocation.X11.h")
endif()
set(AutomatedTests_RESOURCES "")
if(WIN32)
    list(APPEND AutomatedTests_RESOURCES "AutomatedTests.manifest")
    list(APPEND AutomatedTests_RESOURCES "AutomatedTests.rc")
endif()

# Override some files as objective C++ for Cocoa support on macos
if (APPLE)
    set_source_files_properties(main.cpp PROPERTIES LANGUAGE OBJCXX)
    set_source_files_properties(TestSession.cpp PROPERTIES LANGUAGE OBJCXX)
endif()

# Define our project targets
add_executable(AutomatedTests ${AutomatedTests_SOURCES} ${AutomatedTests_HEADERS} ${AutomatedTests_RESOURCES} ${COBALT_VERSION_HEADER_FILES})
register_clang_format_and_tidy_target(AutomatedTests)
cobalt_apply_compiler_warning_settings(AutomatedTests)
add_rpath(AutomatedTests ".")
add_rpath(AutomatedTests "Renderers")

# Set custom compilation/linker settings
target_include_directories(AutomatedTests PRIVATE
    ${COBALT_VERSION_INFO_DIR}
    ${COBALT_ASSETS_DIR}
)

# Specify our project dependencies
target_link_libraries(AutomatedTests
    PRIVATE
        Cobalt::CrashHandling
        Cobalt::ImageDiff
        Cobalt::RendererInterface
        half
        gli
        glm
)
if (WIN32)
    add_dependencies(
        AutomatedTests
        Cobalt::Direct3D11Renderer
        Cobalt::Direct3D12Renderer
        Cobalt::OpenGL3Renderer
        Cobalt::OpenGL4Renderer
        Cobalt::VulkanRenderer
    )
elseif(APPLE)
    add_dependencies(
        AutomatedTests
        Cobalt::OpenGL3Renderer
        Cobalt::VulkanRenderer
    )
    target_link_libraries(AutomatedTests
        PRIVATE
            "-framework ApplicationServices"
            "-framework Cocoa"
            "-framework QuartzCore"
            "-framework Metal"
    )
else()
    add_dependencies(
        AutomatedTests
        Cobalt::OpenGL3Renderer
        Cobalt::OpenGL4Renderer
        Cobalt::VulkanRenderer
    )
    target_link_libraries(AutomatedTests
        PRIVATE
            X11
            X11Xcb
            xdg-shell-protocol
#            CAIRO::cairo
    )
endif()

# Transfer required files to their output locations
cobalt_stage_deps(AutomatedTests "")
cobalt_stage_generated_bin(AutomatedTests "${CMAKE_CURRENT_SOURCE_DIR}/ReferenceImages/" "ReferenceImages")
cobalt_stage_generated_bin(AutomatedTests "${CMAKE_CURRENT_SOURCE_DIR}/Resources/" "Resources")
cobalt_stage_generated_bin(AutomatedTests "${COBALT_SDK_BIN_DIR}/" "Renderers")
cobalt_stage_generated_bin(AutomatedTests "${COBALT_SCRIPTS_DIR}/RunAllUnitTests${COBALT_SHELL_SCRIPT_EXT}" "")
cobalt_stage_generated_bin(AutomatedTests "${COBALT_SCRIPTS_DIR}/RunAllPerformanceTests${COBALT_SHELL_SCRIPT_EXT}" "")
set(TARGET_INSTALL_DIR "${COBALT_OUTPUT_ROOT}/AutomatedTests_${COBALT_ARCH}_$<CONFIG>")
cobalt_install_with_deps(AutomatedTests "${TARGET_INSTALL_DIR}/")
cobalt_install_generated_bin("${CMAKE_CURRENT_SOURCE_DIR}/ReferenceImages/" "${TARGET_INSTALL_DIR}/ReferenceImages")
cobalt_install_generated_bin("${CMAKE_CURRENT_SOURCE_DIR}/Resources/" "${TARGET_INSTALL_DIR}/Resources")
cobalt_install_generated_bin("${COBALT_SDK_BIN_DIR}/" "${TARGET_INSTALL_DIR}/Renderers")
cobalt_install_generated_bin("${COBALT_SCRIPTS_DIR}/RunAllUnitTests${COBALT_SHELL_SCRIPT_EXT}" "${TARGET_INSTALL_DIR}")
cobalt_install_generated_bin("${COBALT_SCRIPTS_DIR}/RunAllPerformanceTests${COBALT_SHELL_SCRIPT_EXT}" "${TARGET_INSTALL_DIR}")
cobalt_install_generated_bin("${COBALT_PROJECT_LICENSE_FILE}" "${TARGET_INSTALL_DIR}")
cobalt_install_generated_bin("${COBALT_THIRD_PARTY_LICENSES_FILE}" "${TARGET_INSTALL_DIR}")

# Define our virtual project folder and file structure
set_target_properties(AutomatedTests PROPERTIES FOLDER "Testing")
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${AutomatedTests_SOURCES} ${AutomatedTests_HEADERS} ${AutomatedTests_RESOURCES})
make_paths_relative(COBALT_VERSION_FILES_RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${COBALT_VERSION_FILES})
source_group("Autogenerated" FILES ${COBALT_VERSION_FILES_RELATIVE})
source_group("Framework" FILES
    "TestSession.h"
    "TestRegistry.h"
    "TestManager.h"
    "ITestSession.h"
    "IUnitTest.h"
    "UnitTestBase.h"
    "PerformanceTestBase.h"
    "TestSession.cpp"
    "TestRegistry.cpp"
    "TestManager.cpp"
    "UnitTestBase.cpp"
    "PerformanceTestBase.cpp"
)
source_group("Support" FILES
    "Debug.h"
    "StringHelpers.h"
    "UnicodeConversion.h"
)
source_group("Support/Geometry" FILES
    "GeometryHelper.h"
    "GeometryHelper.cpp"
    "StanfordBunnyIndices.cpp"
    "StanfordBunnyVertices.cpp"
    "UtahTeapotVertices.cpp"
)
source_group("Support/PseudoRandom" FILES
    "PseudoRandomGenerator.h"
    "PseudoRandomGenerator.cpp"
)
source_group("Support/Texture" FILES
    "TextureHelper.h"
    "TextureHelper.cpp"
)
source_group("Support/Scenes" FILES
    "TexturedQuadSceneHelper.h"
    "TexturedQuadSceneHelper.cpp"
    "MipmappingHelper.h"
    "MipmappingHelper.cpp"
)
source_group("Support/Threading" FILES
    "IThreadInvocation.h"
    "Win32MessageThreadInvocation.h"
    "IThreadInvocation.inl"
)
if (WIN32)
    source_group("Support/Threading" FILES "UIThreadInvocation.Win32.h")
    source_group("Support/Threading" FILES "UIThreadInvocation.Win32.cpp")
elseif(APPLE)
    source_group("Support/Threading" FILES "UIThreadInvocation.Cocoa.h")
    source_group("Support/Threading" FILES "UIThreadInvocation.Cocoa.cpp")
else()
    source_group("Support/Threading" FILES "UIThreadInvocation.X11.h")
    source_group("Support/Threading" FILES "UIThreadInvocation.X11.cpp")
endif()
source_group("Support/Transform" FILES
    "TransformHelper.h"
    "TransformHelper.cpp"
)
source_group("Support/Windows" FILES
    "MenuItem.h"
    "Window.h"
    "MenuItem.cpp"
)
if (WIN32)
    source_group("Support/Windows" FILES "MenuItem.Win32.cpp")
    source_group("Support/Windows" FILES "Window.Win32.cpp")
elseif(APPLE)
    source_group("Support/Windows" FILES "MenuItem.Posix.cpp")
    source_group("Support/Windows" FILES "MenuItem.Cocoa.mm")
    source_group("Support/Windows" FILES "Window.Cocoa.mm")
else()
    source_group("Support/Windows" FILES "MenuItem.Posix.cpp")
    source_group("Support/Windows" FILES "Window.X11.cpp")
endif()
source_group("Tests/Performance Tests" FILES
    "PerformanceTests/BalancedBatchPolyCount.cpp"
    "PerformanceTests/LargeBatchCount.cpp"
    "PerformanceTests/LargePolyCount.cpp"
    "PerformanceTests/MinimalScene.cpp"
    "PerformanceTests/ConstantFrameCaptureWindow.cpp"
    "PerformanceTests/ConstantFrameCaptureBuffer.cpp"
)
source_group("Tests/Unit Tests/Compute" FILES
    "UnitTests/BasicCompute.cpp"
    "UnitTests/WriteVertexBufferFromCompute.cpp"
    "UnitTests/WriteIndexBufferFromCompute.cpp"
)
source_group("Tests/Unit Tests/Device" FILES
    "UnitTests/DeviceEnumeration.cpp"
    "UnitTests/DeviceLimitsTests.cpp"
)
source_group("Tests/Unit Tests/Framebuffer" FILES
    "UnitTests/FrameBufferAttachmentLifecycle.cpp"
    "UnitTests/FrameBufferMultiSampling.cpp"
    "UnitTests/FrameBufferClearColor.cpp"
    "UnitTests/FrameBufferClearDepth.cpp"
    "UnitTests/FrameBufferClearStencil.cpp"
    "UnitTests/FrameBufferOutputFormatCoverage.cpp"
    "UnitTests/FrameBufferOutputLifecycle.cpp"
    "UnitTests/FrameBufferViewportAndScissor.cpp"
)
source_group("Tests/Unit Tests/Renderable" FILES
    "UnitTests/RenderableAttributeBinding.cpp"
    "UnitTests/PrimitiveModes.cpp"
    "UnitTests/PrimitiveRestart.cpp"
    "UnitTests/BufferOffsets.cpp"
    "UnitTests/IndirectDrawFromCompute.cpp"
    "UnitTests/IndirectDraw.cpp"
    "UnitTests/IndirectMultiDrawFromCompute.cpp"
    "UnitTests/InstanceMode.cpp"
)
source_group("Tests/Unit Tests/Resources/DataArray" FILES
    "UnitTests/AppendStructuredBuffer.cpp"
    "UnitTests/ConsumeStructuredBuffer.cpp"
    "UnitTests/RWStructuredBuffer.cpp"
    "UnitTests/StructuredBuffer.cpp"
    "UnitTests/DataArrayGpuTransfer.cpp"
)
source_group("Tests/Unit Tests/Resources/Batching" FILES
    "UnitTests/TransferBatchDataArray.cpp"
    "UnitTests/TransferBatchGeometry.cpp"
    "UnitTests/TransferBatchLifecycle.cpp"
    "UnitTests/TransferBatchTexelArray.cpp"
    "UnitTests/TransferBatchTextureBuffer2D.cpp"
)
source_group("Tests/Unit Tests/Resources/Images" FILES
    "UnitTests/CompressedTextures.cpp"
    "UnitTests/TextureBufferMetadata.cpp"
    "UnitTests/TextureBuffer1D.cpp"
    "UnitTests/TextureBuffer2D.cpp"
    "UnitTests/TextureBuffer2DArray.cpp"
    "UnitTests/TextureBuffer3D.cpp"
    "UnitTests/TextureUpdateCoverage.cpp"
    "UnitTests/TextureBufferCube.cpp"
    "UnitTests/TextureBuffer1DArray.cpp"
    "UnitTests/TextureBufferCubeArray.cpp"
)
source_group("Tests/Unit Tests/Resources/ImageSamplers" FILES
    "UnitTests/TextureSampler1D.cpp"
    "UnitTests/TextureSampler2D.cpp"
    "UnitTests/TextureSampler3D.cpp"
    "UnitTests/TextureSamplerCube.cpp"
    "UnitTests/TextureSampler1DArray.cpp"
    "UnitTests/TextureSampler2DArray.cpp"
    "UnitTests/TextureSamplerCubeArray.cpp"
)
source_group("Tests/Unit Tests/Resources/Mipmapping" FILES
    "UnitTests/MipmappingTexture1D.cpp"
    "UnitTests/MipmappingTexture2D.cpp"
    "UnitTests/MipmappingTexture3D.cpp"
    "UnitTests/MipmappingTextureCube.cpp"
    "UnitTests/MipmappingTexture1DArray.cpp"
    "UnitTests/MipmappingTexture2DArray.cpp"
    "UnitTests/MipmappingTextureCubeArray.cpp"
)
source_group("Tests/Unit Tests/Resources/StateBuffer" FILES
    "UnitTests/StateBufferArrayBindings.cpp"
    "UnitTests/StateBufferLayoutContracts.cpp"
    "UnitTests/StateBufferPaging.cpp"
    "UnitTests/StateBufferWithNoLayout.cpp"
)
source_group("Tests/Unit Tests/Resources/StateValue" FILES
    "UnitTests/ConstantStateValues.cpp"
    "UnitTests/ResourceBindingVariants.cpp"
    "UnitTests/ResourceBindingLifecycle.cpp"
    "UnitTests/StateValueArrayBindings.cpp"
    "UnitTests/StateValueTypeApplication.cpp"
    "UnitTests/StateValueLifecycle.cpp"
)
source_group("Tests/Unit Tests/Resources/TexelArray" FILES
    "UnitTests/Buffer.cpp"
    "UnitTests/RWBuffer.cpp"
    "UnitTests/TexelArrayGpuTransfer.cpp"
)
source_group("Tests/Unit Tests/Resources/VertexBuffer" FILES
    "UnitTests/VertexAttributeTypeConversion.cpp"
    "UnitTests/RawVertexAttributeTest.cpp"
    "UnitTests/VertexBufferChangeTest.cpp"
    "UnitTests/ManualVertexBufferLayout.cpp"
    "UnitTests/DynamicGeometryUpdates.cpp"
)
source_group("Tests/Unit Tests/Scene" FILES
    "UnitTests/BasicScene.cpp"
    "UnitTests/RGBTriangle.cpp"
    "UnitTests/BunnyNormals.cpp"
    "UnitTests/CobaltCube.cpp"
    "UnitTests/RenderTreeChildCollectionContracts.cpp"
    "UnitTests/RenderPassSortOrder.cpp"
    "UnitTests/RenderTreeLifecycle.cpp"
    "UnitTests/Transparency.cpp"
    "UnitTests/OnGpuCopy.cpp"
)
source_group("Tests/Unit Tests/Shader" FILES
    "UnitTests/GeometryShaders.cpp"
    "UnitTests/ShaderReflectionQueries.cpp"
    "UnitTests/ShaderEntryPointSelection.cpp"
    "UnitTests/TrivialDxbc.cpp"
    "UnitTests/TrivialDxil.cpp"
    "UnitTests/TrivialGlsl.cpp"
    "UnitTests/TrivialHlsl.cpp"
    "UnitTests/TrivialMsl.cpp"
    "UnitTests/TrivialSpirvAssembly.cpp"
    "UnitTests/TrivialSpirv.cpp"
)
source_group("Tests/Unit Tests/StateGroup" FILES
    "UnitTests/BlendState.cpp"
    "UnitTests/DepthState.cpp"
    "UnitTests/PolygonFillMode.cpp"
    "UnitTests/PolygonWindingOrderAndCullMode.cpp"
    "UnitTests/SeparateBlendState.cpp"
    "UnitTests/StencilState.cpp"
)
