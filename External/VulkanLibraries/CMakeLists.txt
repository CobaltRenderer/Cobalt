# Vulkan-Headers
FetchContent_Declare(Vulkan-HeadersPackage
    GIT_REPOSITORY "https://github.com/KhronosGroup/Vulkan-Headers.git"
    GIT_TAG "v1.3.261"
    SOURCE_DIR "${COBALT_EXTERNAL_ROOT}/Cache/Vulkan-Headers"
)
FetchContent_MakeAvailable(Vulkan-HeadersPackage)
set(VULKAN_HEADERS_CONFIG_DIR "${CMAKE_BINARY_DIR}/vulkan-headers-config")
file(MAKE_DIRECTORY "${VULKAN_HEADERS_CONFIG_DIR}")
file(WRITE "${VULKAN_HEADERS_CONFIG_DIR}/VulkanHeadersConfig.cmake" "
include_guard()
if(NOT TARGET Vulkan::Headers)
  add_library(Vulkan::Headers INTERFACE IMPORTED GLOBAL)
  set_target_properties(Vulkan::Headers PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES \"${COBALT_EXTERNAL_ROOT}/Cache/Vulkan-Headers/include\")
endif()
if(NOT TARGET Vulkan::Registry)
  add_library(Vulkan::Registry INTERFACE IMPORTED GLOBAL)
  set_target_properties(Vulkan::Registry PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES \"${COBALT_EXTERNAL_ROOT}/Cache/Vulkan-Headers\")
endif()
set(VulkanHeaders_VERSION \"1.3.261\")
")
set(VulkanHeaders_DIR "${VULKAN_HEADERS_CONFIG_DIR}" CACHE PATH "" FORCE)

# Vulkan-Loader
set(BUILD_LOADER_DEPS OFF CACHE BOOL "" FORCE)
FetchContent_Declare(Vulkan-Loader
    GIT_REPOSITORY "https://github.com/KhronosGroup/Vulkan-Loader.git"
    GIT_TAG "v1.3.261"
    SOURCE_DIR "${COBALT_EXTERNAL_ROOT}/Cache/Vulkan-Loader"
    PATCH_COMMAND git apply "${CMAKE_CURRENT_SOURCE_DIR}/Vulkan-Loader.patch"
    UPDATE_DISCONNECTED TRUE
)
FetchContent_MakeAvailable(Vulkan-Loader)
if(TARGET asm_offset)
    target_compile_options(asm_offset PRIVATE $<$<CXX_COMPILER_ID:GNU,Clang>:-Wno-typedef-redefinition>)
    set_project_virtual_folder(asm_offset "External/Vulkan-Loader")
endif()
target_compile_options(vulkan PRIVATE $<$<CXX_COMPILER_ID:GNU,Clang>:-Wno-typedef-redefinition>)
set_project_virtual_folder(vulkan "External/Vulkan-Loader")
set_project_virtual_folder(loader-unknown-chain "External/Vulkan-Loader")
set_project_virtual_folder(loader-opt "External/Vulkan-Loader")
set_project_virtual_folder(loader_asm_gen_files "External/Vulkan-Loader")

# VulkanMemoryAllocator
FetchContent_Declare(VulkanMemoryAllocator
    GIT_REPOSITORY "https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator.git"
    GIT_TAG        "v3.0.1"
    SOURCE_DIR     "${COBALT_EXTERNAL_ROOT}/Cache/VulkanMemoryAllocator"
)
FetchContent_Populate(VulkanMemoryAllocator)
add_library(VulkanMemoryAllocator INTERFACE IMPORTED)
target_include_directories(VulkanMemoryAllocator SYSTEM INTERFACE "${COBALT_EXTERNAL_ROOT}/Cache/VulkanMemoryAllocator/include")

# VulkanBundle
add_library(VulkanBundle INTERFACE)
target_link_libraries(VulkanBundle INTERFACE
    Vulkan-Headers
    vulkan
    VulkanMemoryAllocator
)

# On macOS, we don't want to assume the Vulkan SDK is installed or rely on system paths, however things are made very
# difficult by the fact the only official distributions for the Vulkan loader and MoltenVK appear within the Vulkan SDK,
# which is only availble as a zipped app installer bundle, which we can't unpack. Worse still, if we rely on the unzip
# feature built into cmake, it extracts the zip, then automatically tries to extract the contained app bundle too,
# making it useless. We therefore have to manually extract the zip, and run the actual SDK installer in a sandboxed mode
# to a target directory to get the handful of files we need.
if(APPLE)
    set(VULKAN_SDK_VERSION "1.4.341.0")
    set(VULKAN_SDK_URL "https://sdk.lunarg.com/sdk/download/${VULKAN_SDK_VERSION}/mac/vulkan_sdk.zip")

    set(VULKAN_SDK_APP_DIR "${CMAKE_BINARY_DIR}/_deps/vulkan-sdk-app")
    set(VULKAN_SDK_INSTALL_ROOT "${CMAKE_BINARY_DIR}/_deps/vulkan-sdk-install")

    FetchContent_Declare(
        vulkan_sdk_zip
        URL "${VULKAN_SDK_URL}"
        SOURCE_DIR "${COBALT_EXTERNAL_ROOT}/Cache/VulkanSDK"
        DOWNLOAD_NO_EXTRACT TRUE
    )
    FetchContent_Populate(vulkan_sdk_zip)

    # ---- unzip stage ----
    if(NOT EXISTS "${VULKAN_SDK_APP_DIR}/.extracted")
        file(MAKE_DIRECTORY "${VULKAN_SDK_APP_DIR}")
        file(GLOB VULKAN_SDK_ZIP_FILES "${vulkan_sdk_zip_SOURCE_DIR}/*.zip")
        list(GET VULKAN_SDK_ZIP_FILES 0 VULKAN_SDK_ZIP_FILE)

        execute_process(
            COMMAND "${CMAKE_COMMAND}" -E tar xf "${VULKAN_SDK_ZIP_FILE}"
            WORKING_DIRECTORY "${VULKAN_SDK_APP_DIR}"
            RESULT_VARIABLE _unzip_result
        )

        if(NOT _unzip_result EQUAL 0)
            message(FATAL_ERROR "Failed to unzip ${VULKAN_SDK_ZIP_FILE}")
        endif()

        file(WRITE "${VULKAN_SDK_APP_DIR}/.extracted" "")
    endif()

    # ---- locate app ----
    file(GLOB VULKAN_SDK_APPS "${VULKAN_SDK_APP_DIR}/*.app")

    if(VULKAN_SDK_APPS STREQUAL "")
        message(FATAL_ERROR "No .app found after extraction")
    endif()

    list(GET VULKAN_SDK_APPS 0 VULKAN_SDK_APP)

    # ---- install stage ----
    if(NOT EXISTS "${VULKAN_SDK_INSTALL_ROOT}/macOS/include")
        file(GLOB VULKAN_SDK_INSTALLER_EXES "${VULKAN_SDK_APP}/Contents/MacOS/*")

        list(GET VULKAN_SDK_INSTALLER_EXES 0 VULKAN_SDK_INSTALLER_EXE)

        execute_process(
            COMMAND "${VULKAN_SDK_INSTALLER_EXE}"
                --root "${VULKAN_SDK_INSTALL_ROOT}"
                --accept-licenses
                --default-answer
                --confirm-command
                install
            RESULT_VARIABLE _install_result
        )

        if(NOT _install_result EQUAL 0)
            message(FATAL_ERROR "Failed to install Vulkan SDK")
        endif()
    endif()

    set(COBALT_MACOS_VULKAN_SDK_INSTALL_PATH "${VULKAN_SDK_INSTALL_ROOT}/macOS" CACHE PATH "")
endif()

# Vulkan-ValidationLayers
# We build this as an external project with self-contained dependencies, specifically so we can build with the very
# latest validation layers, even when we're on a different/older Vulkan SDK version. We can get away with a straight
# ExternalProject build here, because we don't have any actual dependencies on the build output here, we just need to be
# able to find its output shared library and copy it into our install directory. Note that we had to patch the project
# to allow dependencies to be pulled in and built once, then cached, without triggering a lot of invalidation on each
# build, so we added in the UPDATE_DEPS_ONCE variable. Note also that the script-based approach the library takes to
# building its dependencies is very slow and inefficient. Since validation layers are an optional conveniece for
# development of the Vulkan renderer itself, we don't enable this by default.
if(COBALT_BUILD_VULKAN_VALIDATION_LAYERS)
    set(COBALT_EXTERNAL_VULKAN_LAYERS "${COBALT_EXTERNAL_ROOT}/Cache/Vulkan-ValidationLayers")
    if(WIN32)
        set(COBALT_EXTERNAL_VULKAN_LAYERS_OUTPUT_PATH "${COBALT_EXTERNAL_VULKAN_LAYERS}/bin" CACHE PATH "")
    else()
        set(COBALT_EXTERNAL_VULKAN_LAYERS_OUTPUT_PATH "${COBALT_EXTERNAL_VULKAN_LAYERS}/lib" CACHE PATH "")
    endif()
    ExternalProject_Add(Vulkan-ValidationLayers
        GIT_REPOSITORY   https://github.com/KhronosGroup/Vulkan-ValidationLayers.git
        GIT_TAG          v1.4.336
        SOURCE_DIR       "${COBALT_EXTERNAL_ROOT}/Cache/VVL" # There are deep paths here, so we shorten the dir name.
        INSTALL_DIR      "${COBALT_EXTERNAL_ROOT}/Cache/Vulkan-ValidationLayers"
        PATCH_COMMAND git apply "${CMAKE_CURRENT_SOURCE_DIR}/Vulkan-ValidationLayers.patch"
        UPDATE_DISCONNECTED TRUE
        BUILD_ALWAYS     FALSE
        INSTALL_BYPRODUCTS "${COBALT_EXTERNAL_VULKAN_LAYERS_OUTPUT_PATH}/${CMAKE_SHARED_LIBRARY_PREFIX}VkLayer_khronos_validation${CMAKE_SHARED_LIBRARY_SUFFIX}"
        BUILD_BYPRODUCTS "${COBALT_EXTERNAL_VULKAN_LAYERS_OUTPUT_PATH}/${CMAKE_SHARED_LIBRARY_PREFIX}VkLayer_khronos_validation${CMAKE_SHARED_LIBRARY_SUFFIX}"
        BUILD_COMMAND    ${CMAKE_COMMAND} --build . --parallel 1
        CMAKE_ARGS
            -DUPDATE_DEPS=ON
            -DUPDATE_DEPS_ONCE=ON
            -DBUILD_TESTS=OFF
            -DCMAKE_POLICY_VERSION_MINIMUM=3.5
            -DCMAKE_BUILD_TYPE=RelWithDebInfo
            -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>
            -DCMAKE_FIND_USE_SYSTEM_PACKAGE_REGISTRY=OFF
            -DCMAKE_FIND_USE_PACKAGE_REGISTRY=OFF
            -DCMAKE_IGNORE_PATH=/usr/local
    )
    set_project_virtual_folder(Vulkan-ValidationLayers "External/Vulkan-ValidationLayers")
endif()
