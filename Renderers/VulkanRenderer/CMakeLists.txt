# Specify our source files
set(VulkanRenderer_SOURCES
    "BindingHelpers.cpp"
    "VulkanDataArrayOutput.cpp"
    "VulkanDefaultState.cpp"
    "VulkanFrameBuffer.cpp"
    "VulkanFrameBufferOutput.cpp"
    "VulkanGraphicsDevice.cpp"
    "VulkanGraphicsDeviceEnumerator.cpp"
    "VulkanIndexBuffer.cpp"
    "VulkanInstanceData.cpp"
    "VulkanRendererFactory.cpp"
    "VulkanMemoryManager.cpp"
    "VulkanProgramNode.cpp"
    "VulkanRenderableNode.cpp"
    "VulkanRenderer.cpp"
    "VulkanRenderPassNode.cpp"
    "VulkanDataArray.cpp"
    "VulkanShaderProgram.cpp"
    "VulkanStateBuffer.cpp"
    "VulkanStateBufferLayout.cpp"
    "VulkanStateGroupNode.cpp"
    "VulkanTexelArray.cpp"
    "VulkanTexelArrayOutput.cpp"
    "VulkanTransferBatch.cpp"
    "VulkanVertexBuffer.cpp"
)
set(VulkanRenderer_HEADERS
    "BindingHelpers.h"
    "BindingHelpers.inl"
    "VulkanDataArrayOutput.h"
    "VulkanDefaultState.h"
    "VulkanFrameBuffer.h"
    "VulkanFrameBufferOutput.h"
    "VulkanGraphicsDevice.h"
    "VulkanGraphicsDeviceEnumerator.h"
    "VulkanHeaders.h"
    "VulkanIndexBuffer.h"
    "VulkanInstanceData.h"
    "VulkanRendererFactory.h"
    "VulkanMemoryManager.h"
    "VulkanNode.h"
    "VulkanNode.inl"
    "VulkanProgramNode.h"
    "VulkanRenderableNode.h"
    "VulkanRenderer.h"
    "VulkanRenderer.inl"
    "VulkanRenderPassNode.h"
    "VulkanDataArray.h"
    "VulkanShaderProgram.h"
    "VulkanStateBuffer.h"
    "VulkanStateBufferLayout.h"
    "VulkanStateBufferLayout.inl"
    "VulkanStateContainer.h"
    "VulkanStateContainer.inl"
    "VulkanStateGroupNode.h"
    "VulkanTexelArray.h"
    "VulkanTexelArrayOutput.h"
    "VulkanTextureBuffer.h"
    "VulkanTextureBuffer.inl"
    "VulkanTextureBuffer1D.h"
    "VulkanTextureBuffer1D.inl"
    "VulkanTextureBuffer1DArray.h"
    "VulkanTextureBuffer1DArray.inl"
    "VulkanTextureBuffer2D.h"
    "VulkanTextureBuffer2D.inl"
    "VulkanTextureBuffer2DArray.h"
    "VulkanTextureBuffer2DArray.inl"
    "VulkanTextureBuffer3D.h"
    "VulkanTextureBuffer3D.inl"
    "VulkanTextureBufferCube.h"
    "VulkanTextureBufferCube.inl"
    "VulkanTextureBufferCubeArray.h"
    "VulkanTextureBufferCubeArray.inl"
    "VulkanTextureSampler.h"
    "VulkanTextureSampler.inl"
    "VulkanTextureSampler1D.h"
    "VulkanTextureSampler1D.inl"
    "VulkanTextureSampler1DArray.h"
    "VulkanTextureSampler1DArray.inl"
    "VulkanTextureSampler2D.h"
    "VulkanTextureSampler2D.inl"
    "VulkanTextureSampler2DArray.h"
    "VulkanTextureSampler2DArray.inl"
    "VulkanTextureSampler3D.h"
    "VulkanTextureSampler3D.inl"
    "VulkanTextureSamplerCube.h"
    "VulkanTextureSamplerCube.inl"
    "VulkanTextureSamplerCubeArray.h"
    "VulkanTextureSamplerCubeArray.inl"
    "VulkanTransferBatch.h"
    "VulkanVertexBuffer.h"
)

# Override some files as objective C++ for Cocoa support on macos
if (APPLE)
    set_source_files_properties(VulkanFrameBuffer.cpp PROPERTIES LANGUAGE OBJCXX)
endif()

# Define our project targets
add_library(VulkanRenderer SHARED ${VulkanRenderer_SOURCES} ${VulkanRenderer_HEADERS} ${COBALT_VERSION_FILES})
add_library(Cobalt::VulkanRenderer ALIAS VulkanRenderer)
register_clang_format_and_tidy_target(VulkanRenderer)
cobalt_apply_compiler_warning_settings(VulkanRenderer)
add_rpath(VulkanRenderer ".")

# Set custom compilation/linker settings
target_include_directories(VulkanRenderer PRIVATE ${COBALT_VERSION_INFO_DIR})

# Specify our project dependencies
target_link_libraries(VulkanRenderer
    PRIVATE
        Cobalt::RendererInterface
        Cobalt::RendererSupport
        Cobalt::ShaderSupport
        Cobalt::TextureFormatConversion
        VulkanBundle
)
if(APPLE)
    target_link_libraries(VulkanRenderer
        PRIVATE
            "-framework Cocoa"
            "-framework QuartzCore"
    )
elseif(UNIX)
    target_link_libraries(VulkanRenderer
        PRIVATE
            X11
            xcb
    )
endif()

# Transfer required files to their output locations
cobalt_configure_sdk_install(VulkanRenderer)
cobalt_install_sdk_headers("Renderers" "VulkanRendererFactory.h")
if(COBALT_BUILD_VULKAN_VALIDATION_LAYERS)
    cobalt_install_generated_bin("${COBALT_EXTERNAL_VULKAN_LAYERS_OUTPUT_PATH}/${CMAKE_SHARED_LIBRARY_PREFIX}VkLayer_khronos_validation${CMAKE_SHARED_LIBRARY_SUFFIX}" "${COBALT_SDK_BIN_DIR}")
    if(WIN32)
        cobalt_install_generated_bin("${COBALT_EXTERNAL_VULKAN_LAYERS_OUTPUT_PATH}/VkLayer_khronos_validation.json" "${COBALT_SDK_BIN_DIR}")
    else()
        cobalt_install_generated_bin("${COBALT_EXTERNAL_VULKAN_LAYERS_OUTPUT_PATH}/../share/vulkan/explicit_layer.d/VkLayer_khronos_validation.json" "${COBALT_SDK_BIN_DIR}")
    endif()
endif()
if(APPLE)
    # Set paths to the Vulkan loader and MoltenVK target files from the SDK
    set(VULKAN_LOADER_DYLIB "${COBALT_MACOS_VULKAN_SDK_INSTALL_PATH}/lib/libvulkan.1.dylib")
    set(MOLTENVK_DYLIB "${COBALT_MACOS_VULKAN_SDK_INSTALL_PATH}/lib/libMoltenVK.dylib")
    set(MOLTENVK_ICD_JSON "${COBALT_MACOS_VULKAN_SDK_INSTALL_PATH}/share/vulkan/icd.d/MoltenVK_icd.json")
    set(VULKAN_LAYER_DIR "${COBALT_MACOS_VULKAN_SDK_INSTALL_PATH}/share/vulkan/explicit_layer.d")
    set(VULKAN_VALIDATION_LAYER_JSON "${VULKAN_LAYER_DIR}/VkLayer_khronos_validation.json")
    set(VULKAN_VALIDATION_LAYER_DYLIB "${COBALT_MACOS_VULKAN_SDK_INSTALL_PATH}/lib/libVkLayer_khronos_validation.dylib")

    # The json files contain relative paths to targets, so we patch them here to account for the target files being
    # directly next to them in the same directory in our distribution.
    function(cobalt_patch_vulkan_json input_file output_file)
        file(READ "${input_file}" contents)
        string(REGEX REPLACE "\"library_path\"[ \t]*:[ \t]*\"[^\"]*/([^/\"]+)\"" "\"library_path\": \"\\1\"" contents "${contents}")
        file(WRITE "${output_file}" "${contents}")
    endfunction()
    set(VULKAN_STAGING_DIR "${CMAKE_BINARY_DIR}/generated/vulkan")
    file(MAKE_DIRECTORY "${VULKAN_STAGING_DIR}")
    set(PATCHED_MOLTENVK_ICD_JSON "${VULKAN_STAGING_DIR}/MoltenVK_icd.json")
    set(PATCHED_VALIDATION_LAYER_JSON "${VULKAN_STAGING_DIR}/VkLayer_khronos_validation.json")
    cobalt_patch_vulkan_json("${MOLTENVK_ICD_JSON}" "${PATCHED_MOLTENVK_ICD_JSON}")
    cobalt_patch_vulkan_json("${VULKAN_VALIDATION_LAYER_JSON}" "${PATCHED_VALIDATION_LAYER_JSON}")

    # The libvulcan.1.dylib file is a symlink on disk, so we resolve it here down to an actual file.
    file(REAL_PATH "${VULKAN_LOADER_DYLIB}" VULKAN_LOADER_DYLIB_REAL)
    configure_file("${VULKAN_LOADER_DYLIB_REAL}" "${VULKAN_STAGING_DIR}/libvulkan.1.dylib" COPYONLY)

    # Copy all the files we need to get a self-contained Vulkan loader with MoltenVK for macOS
    cobalt_install_generated_bin("${VULKAN_STAGING_DIR}/libvulkan.1.dylib" "${COBALT_SDK_BIN_DIR}")
    cobalt_install_generated_bin("${MOLTENVK_DYLIB}" "${COBALT_SDK_BIN_DIR}")
    cobalt_install_generated_bin("${VULKAN_VALIDATION_LAYER_DYLIB}" "${COBALT_SDK_BIN_DIR}")
    cobalt_install_generated_bin("${PATCHED_MOLTENVK_ICD_JSON}" "${COBALT_SDK_BIN_DIR}")
    cobalt_install_generated_bin("${PATCHED_VALIDATION_LAYER_JSON}" "${COBALT_SDK_BIN_DIR}")
endif()

# Define our virtual project folder and file structure
set_target_properties(VulkanRenderer PROPERTIES FOLDER "Renderers")
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${VulkanRenderer_SOURCES} ${VulkanRenderer_HEADERS})
make_paths_relative(COBALT_VERSION_FILES_RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${COBALT_VERSION_FILES})
source_group("Autogenerated" FILES ${COBALT_VERSION_FILES_RELATIVE})
source_group("Render Tree/Binding Helpers" FILES
    "BindingHelpers.h"
    "BindingHelpers.cpp"
    "BindingHelpers.inl"
)
source_group("Render Tree/VulkanDefaultState" FILES
    "VulkanDefaultState.h"
    "VulkanDefaultState.cpp"
)
source_group("Render Tree/VulkanNode" FILES
    "VulkanNode.h"
    "VulkanNode.inl"
)
source_group("Render Tree/VulkanProgramNode" FILES
    "VulkanProgramNode.h"
    "VulkanProgramNode.cpp"
)
source_group("Render Tree/VulkanRenderPassNode" FILES
    "VulkanRenderPassNode.h"
    "VulkanRenderPassNode.cpp"
)
source_group("Render Tree/VulkanRenderableNode" FILES
    "VulkanRenderableNode.h"
    "VulkanRenderableNode.cpp"
)
source_group("Render Tree/VulkanStateContainer" FILES
    "VulkanStateContainer.h"
    "VulkanStateContainer.inl"
)
source_group("Render Tree/VulkanStateGroupNode" FILES
    "VulkanStateGroupNode.h"
    "VulkanStateGroupNode.cpp"
)
source_group("Renderer/VulkanGraphicsDevice" FILES
    "VulkanGraphicsDevice.h"
    "VulkanGraphicsDevice.cpp"
)
source_group("Renderer/VulkanGraphicsDeviceEnumerator" FILES
    "VulkanGraphicsDeviceEnumerator.h"
    "VulkanGraphicsDeviceEnumerator.cpp"
)
source_group("Renderer/VulkanInstanceData" FILES
    "VulkanInstanceData.h"
    "VulkanInstanceData.cpp"
)
source_group("Renderer/VulkanRenderer" FILES
    "VulkanRenderer.h"
    "VulkanRenderer.cpp"
    "VulkanRenderer.inl"
)
source_group("Resource Management/VulkanMemoryManager" FILES
    "VulkanMemoryManager.h"
    "VulkanMemoryManager.cpp"
)
source_group("Resources/Batching" FILES
    "VulkanTransferBatch.h"
    "VulkanTransferBatch.cpp"
)
source_group("Resources/Data/VulkanDataArray" FILES
    "VulkanDataArray.h"
    "VulkanDataArray.cpp"
)
source_group("Resources/Data/VulkanDataArrayOutput" FILES
    "VulkanDataArrayOutput.h"
    "VulkanDataArrayOutput.cpp"
)
source_group("Resources/Data/VulkanStateBuffer" FILES
    "VulkanStateBuffer.h"
    "VulkanStateBuffer.cpp"
)
source_group("Resources/Data/VulkanStateBufferLayout" FILES
    "VulkanStateBufferLayout.h"
    "VulkanStateBufferLayout.cpp"
    "VulkanStateBufferLayout.inl"
)
source_group("Resources/Data/VulkanTexelArray" FILES
    "VulkanTexelArray.h"
    "VulkanTexelArray.cpp"
)
source_group("Resources/Data/VulkanTexelArrayOutput" FILES
    "VulkanTexelArrayOutput.h"
    "VulkanTexelArrayOutput.cpp"
)
source_group("Resources/FrameBuffers/VulkanFrameBuffer" FILES
    "VulkanFrameBuffer.h"
    "VulkanFrameBuffer.cpp"
)
source_group("Resources/FrameBuffers/VulkanFrameBufferOutput" FILES
    "VulkanFrameBufferOutput.h"
    "VulkanFrameBufferOutput.cpp"
)
source_group("Resources/Geometry/VulkanIndexBuffer" FILES
    "VulkanIndexBuffer.h"
    "VulkanIndexBuffer.cpp"
)
source_group("Resources/Geometry/VulkanVertexBuffer" FILES
    "VulkanVertexBuffer.h"
    "VulkanVertexBuffer.cpp"
)
source_group("Resources/Images/VulkanTextureBuffer" FILES
    "VulkanTextureBuffer.h"
    "VulkanTextureBuffer.inl"
)
source_group("Resources/Images/VulkanTextureBuffer1D" FILES
    "VulkanTextureBuffer1D.h"
    "VulkanTextureBuffer1D.inl"
)
source_group("Resources/Images/VulkanTextureBuffer1DArray" FILES
    "VulkanTextureBuffer1DArray.h"
    "VulkanTextureBuffer1DArray.inl"
)
source_group("Resources/Images/VulkanTextureBuffer2D" FILES
    "VulkanTextureBuffer2D.h"
    "VulkanTextureBuffer2D.inl"
)
source_group("Resources/Images/VulkanTextureBuffer2DArray" FILES
    "VulkanTextureBuffer2DArray.h"
    "VulkanTextureBuffer2DArray.inl"
)
source_group("Resources/Images/VulkanTextureBuffer3D" FILES
    "VulkanTextureBuffer3D.h"
    "VulkanTextureBuffer3D.inl"
)
source_group("Resources/Images/VulkanTextureBufferCube" FILES
    "VulkanTextureBufferCube.h"
    "VulkanTextureBufferCube.inl"
)
source_group("Resources/Images/VulkanTextureBufferCubeArray" FILES
    "VulkanTextureBufferCubeArray.h"
    "VulkanTextureBufferCubeArray.inl"
)
source_group("Resources/Programs/VulkanShaderProgram" FILES
    "VulkanShaderProgram.h"
    "VulkanShaderProgram.cpp"
)
source_group("Resources/Samplers/VulkanTextureSampler" FILES
    "VulkanTextureSampler.h"
    "VulkanTextureSampler.inl"
)
source_group("Resources/Samplers/VulkanTextureSampler1D" FILES
    "VulkanTextureSampler1D.h"
    "VulkanTextureSampler1D.inl"
)
source_group("Resources/Samplers/VulkanTextureSampler1DArray" FILES
    "VulkanTextureSampler1DArray.h"
    "VulkanTextureSampler1DArray.inl"
)
source_group("Resources/Samplers/VulkanTextureSampler2D" FILES
    "VulkanTextureSampler2D.h"
    "VulkanTextureSampler2D.inl"
)
source_group("Resources/Samplers/VulkanTextureSampler2DArray" FILES
    "VulkanTextureSampler2DArray.h"
    "VulkanTextureSampler2DArray.inl"
)
source_group("Resources/Samplers/VulkanTextureSampler3D" FILES
    "VulkanTextureSampler3D.h"
    "VulkanTextureSampler3D.inl"
)
source_group("Resources/Samplers/VulkanTextureSamplerCube" FILES
    "VulkanTextureSamplerCube.h"
    "VulkanTextureSamplerCube.inl"
)
source_group("Resources/Samplers/VulkanTextureSamplerCubeArray" FILES
    "VulkanTextureSamplerCubeArray.h"
    "VulkanTextureSamplerCubeArray.inl"
)
