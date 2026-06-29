# SPIRV-Cross
set(SPIRV_CROSS_CLI          OFF CACHE BOOL "" FORCE)
set(SPIRV_CROSS_ENABLE_C_API OFF CACHE BOOL "" FORCE)
set(SPIRV_CROSS_SKIP_INSTALL ON CACHE BOOL "" FORCE)
set(SPIRV_CROSS_FORCE_PIC    ON CACHE BOOL "" FORCE)
set(SPIRV_CROSS_ENABLE_TESTS OFF CACHE BOOL "" FORCE)
set(SPIRV_CROSS_CLI          OFF CACHE BOOL "" FORCE)
set(SPIRV_CROSS_SHARED       OFF CACHE BOOL "" FORCE)
set(SPIRV_CROSS_STATIC       ON CACHE BOOL "" FORCE)
FetchContent_Declare(SPIRVCross
    GIT_REPOSITORY "https://github.com/KhronosGroup/SPIRV-Cross.git"
#    GIT_TAG ""
    GIT_TAG "9daa7fe0a36dbe5c5b810d74d2cc43b202407d57"
    SOURCE_DIR "${COBALT_EXTERNAL_ROOT}/Cache/SPIRV-Cross"
)
FetchContent_MakeAvailable(SPIRVCross)
set_project_virtual_folder(spirv-cross-core "External/SPIRV-Cross")
set_project_virtual_folder(spirv-cross-cpp "External/SPIRV-Cross")
set_project_virtual_folder(spirv-cross-glsl "External/SPIRV-Cross")
set_project_virtual_folder(spirv-cross-hlsl "External/SPIRV-Cross")
set_project_virtual_folder(spirv-cross-msl "External/SPIRV-Cross")
set_project_virtual_folder(spirv-cross-reflect "External/SPIRV-Cross")
set_project_virtual_folder(spirv-cross-util "External/SPIRV-Cross")
add_library(spirv-crossBundle INTERFACE)
target_link_libraries(spirv-crossBundle INTERFACE
    spirv-cross-core
    spirv-cross-cpp
    spirv-cross-glsl
    spirv-cross-hlsl
    spirv-cross-msl
    spirv-cross-reflect
    spirv-cross-util
)

# SPIRV-Headers
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
set(SPIRV_HEADERS_SKIP_INSTALL  ON  CACHE BOOL "" FORCE)
set(SPIRV_HEADERS_SKIP_EXAMPLES ON  CACHE BOOL "" FORCE)
FetchContent_Declare(SPIRVHeaders
    GIT_REPOSITORY "https://github.com/KhronosGroup/SPIRV-Headers.git"
#    GIT_TAG "4f7b471f1a66b6d06462cd4ba57628cc0cd087d7"
    GIT_TAG "sdk-1.3.216.0"
    SOURCE_DIR "${COBALT_EXTERNAL_ROOT}/Cache/SPIRV-Headers"
)
FetchContent_MakeAvailable(SPIRVHeaders)
set_project_virtual_folder(SPIRV-Headers-example "External/SPIRV-Headers")
set_project_virtual_folder(install-headers "External/SPIRV-Headers")

# SPIRV-Tools
set(SPIRV_SKIP_EXECUTABLES   ON  CACHE BOOL "" FORCE)
set(SPIRV_TOOLS_BUILD_STATIC ON  CACHE BOOL "" FORCE)
set(SKIP_SPIRV_TOOLS_INSTALL ON  CACHE BOOL "" FORCE)
FetchContent_Declare(SPIRVTools
    GIT_REPOSITORY "https://github.com/KhronosGroup/SPIRV-Tools.git"
#    GIT_TAG "dd4b663e13c07fea4fbb3f70c1c91c86731099f7"
    GIT_TAG "sdk-1.3.216.0"
    SOURCE_DIR "${COBALT_EXTERNAL_ROOT}/Cache/SPIRV-Tools"
    PATCH_COMMAND git apply "${CMAKE_CURRENT_SOURCE_DIR}/SPIRV-Tools.patch"
    UPDATE_DISCONNECTED TRUE
)
FetchContent_MakeAvailable(SPIRVTools)
set_project_virtual_folder(core_tables "External/SPIRV-Tools")
set_project_virtual_folder(enum_string_mapping "External/SPIRV-Tools")
set_project_virtual_folder(extinst_tables "External/SPIRV-Tools")
set_project_virtual_folder(spirv-tools-pkg-config "External/SPIRV-Tools")
set_project_virtual_folder(spirv-tools-build-version "External/SPIRV-Tools/build")
set_project_virtual_folder(spirv-tools-header-DebugInfo "External/SPIRV-Tools/build")
set_project_virtual_folder(spirv-tools-header-NonSemanticShaderDebugInfo100 "External/SPIRV-Tools/build")
set_project_virtual_folder(spirv-tools-header-OpenCLDebugInfo100 "External/SPIRV-Tools/build")
set_project_virtual_folder(spv-tools-cldi100 "External/SPIRV-Tools/build")
set_project_virtual_folder(spv-tools-clspvreflection "External/SPIRV-Tools/build")
set_project_virtual_folder(spv-tools-debuginfo "External/SPIRV-Tools/build")
set_project_virtual_folder(spv-tools-shdi100 "External/SPIRV-Tools/build")
set_project_virtual_folder(spv-tools-spv-amd-gs "External/SPIRV-Tools/build")
set_project_virtual_folder(spv-tools-spv-amd-sb "External/SPIRV-Tools/build")
set_project_virtual_folder(spv-tools-spv-amd-sevp "External/SPIRV-Tools/build")
set_project_virtual_folder(spv-tools-spv-amd-stm "External/SPIRV-Tools/build")
set_project_virtual_folder(spv-tools-vkspreflection "External/SPIRV-Tools/build")
set_project_virtual_folder(SPIRV-Tools-diff "External/SPIRV-Tools/libraries")
set_project_virtual_folder(SPIRV-Tools-link "External/SPIRV-Tools/libraries")
set_project_virtual_folder(SPIRV-Tools-lint "External/SPIRV-Tools/libraries")
set_project_virtual_folder(SPIRV-Tools-opt "External/SPIRV-Tools/libraries")
set_project_virtual_folder(SPIRV-Tools-reduce "External/SPIRV-Tools/libraries")
set_project_virtual_folder(SPIRV-Tools-shared "External/SPIRV-Tools/libraries")
set_project_virtual_folder(SPIRV-Tools-static "External/SPIRV-Tools/libraries")
set_project_virtual_folder(spirv-tools-vimsyntax "External/SPIRV-Tools/utilities")
set_project_virtual_folder(spirv-tools-shared-pkg-config "External/SPIRV-Tools/utilities")
add_library(SPIRV-ToolsBundle INTERFACE)
target_link_libraries(SPIRV-ToolsBundle INTERFACE
    SPIRV-Tools-static
    SPIRV-Tools-opt
)
# To prevent failures compiling some third libraries on newer versions of Clang, and GCC, we need this currently.
# Updating SPIRV-Tools to a newer version will likely address this.
if(CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_compile_options(SPIRV-Tools-opt PRIVATE -Wno-error=deprecated-declarations)
    target_compile_options(SPIRV-Tools-opt PRIVATE -Wno-error=deprecated-copy)
endif()
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(SPIRV-Tools-opt PRIVATE -Wno-maybe-uninitialized)
endif()

# glslang
set(ENABLE_GLSLANG_BINARIES OFF CACHE BOOL "" FORCE)
set(ENABLE_SPVREMAPPER      OFF CACHE BOOL "" FORCE)
set(GLSLANG_TESTS           OFF CACHE BOOL "" FORCE)
set(SKIP_GLSLANG_INSTALL    ON  CACHE BOOL "" FORCE)
set(ENABLE_GLSLANG_BINARIES OFF CACHE BOOL "" FORCE)
FetchContent_Declare(glslangLib
    GIT_REPOSITORY "https://github.com/KhronosGroup/glslang.git"
#    GIT_TAG "14.2.0"
    GIT_TAG "11.10.0"
    SOURCE_DIR "${COBALT_EXTERNAL_ROOT}/Cache/glslang"
)
FetchContent_MakeAvailable(glslangLib)
add_library(glslangBundle INTERFACE)
set_project_virtual_folder(GenericCodeGen "External/glslang")
set_project_virtual_folder(glslang "External/glslang")
set_project_virtual_folder(MachineIndependent "External/glslang")
set_project_virtual_folder(OSDependent "External/glslang")
set_project_virtual_folder(SPIRV "External/glslang")
set_project_virtual_folder(HLSL "External/glslang")
set_project_virtual_folder(OGLCompiler "External/glslang")
target_link_libraries(glslangBundle INTERFACE
    GenericCodeGen
    glslang
    MachineIndependent
    OSDependent
    SPIRV
)
