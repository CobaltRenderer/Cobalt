# This is the top-level CMake file for the Cobalt Renderer project
cmake_minimum_required(VERSION 3.21)

# Define our project and language settings
project(CobaltRenderer LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
enable_language(ASM) # For third party library compilation
if(APPLE)
    enable_language(OBJCXX)
endif()
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_BUILD_WITH_INSTALL_RPATH ON)

# Define our project settings
option(COBALT_BUILD_TESTS "Specifies whether the test projects will be compiled." ON)
option(COBALT_BUILD_UTILITIES "Specifies whether the utility projects will be compiled." ON)
option(COBALT_BUILD_EXAMPLES "Specifies whether the example projects will be compiled." ON)
option(COBALT_OPENGL_USE_EGL "Specifies whether the OpenGL renderers should use EGL for Linux (supporting Xlib, XCB, and Wayland), or GLX (Xlib only)." ON)
option(COBALT_BUILD_VULKAN_VALIDATION_LAYERS "Specifies whether the Vulkan validation layers will be compiled and output to the SDK." OFF)
option(COBALT_USE_SDL3 "Specifies whether SDL3 will be used when building Cobalt. Since SDL sits above the API, this only affects example projects." ON)
option(COBALT_USE_LOCAL_SDL3 "Specifies whether SDL3 will be found locally on the system rather than downloaded and built." OFF)
option(COBALT_RUN_MSVC_STATIC_ANALYSIS "Specifies whether Visual Studio static analysis is being run during the build process." OFF)
option(COBALT_CLANG_FORMAT_ON_ALL "Run clang-format (check) in the default build" ON)
option(COBALT_CLANG_TIDY_ON_ALL "Run clang-tidy (check) in the default build" OFF)
option(COBALT_INHERIT_SYSTEM_INSTALL_PATH "Inherit the system-default install path rather than setting it internally" OFF)

# Enable project folders
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Add our modules folder to the module search path
list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")

# Set the install directory. We prefer to install to a private output location, however there is opt-in support for
# using the system install location.
if(COBALT_INHERIT_SYSTEM_INSTALL_PATH)
    set(COBALT_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}" CACHE PATH "Install path prefix for Cobalt SDK" FORCE)
else()
    set(COBALT_INSTALL_PREFIX "${PROJECT_SOURCE_DIR}/Output" CACHE PATH "Install path prefix for Cobalt SDK")
    set(CMAKE_INSTALL_PREFIX "${COBALT_INSTALL_PREFIX}" CACHE PATH "Install path prefix" FORCE)
endif()
message(STATUS "Set install dir to ${CMAKE_INSTALL_PREFIX}")

# Ensure that a configuration is set by default
if(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "Build type")
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS Debug RelWithDebInfo)
endif()

# Configure address sanitizer configurations
include(AddressSanitizer)

# Detect target architecture
string(TOLOWER "${CMAKE_SYSTEM_PROCESSOR}" _cpu)
if(_cpu STREQUAL "amd64" OR _cpu STREQUAL "x86_64" OR _cpu STREQUAL "x64")
    set(COBALT_ARCH       "x64")
    set(COBALT_ARCH_SHORT "64")
elseif(_cpu STREQUAL "x86" OR _cpu STREQUAL "i686" OR _cpu STREQUAL "i386")
    set(COBALT_ARCH       "x86")
    set(COBALT_ARCH_SHORT "32")
elseif(_cpu STREQUAL "aarch64" OR _cpu STREQUAL "arm64")
    set(COBALT_ARCH       "arm64")
    set(COBALT_ARCH_SHORT "arm64")
elseif(_cpu MATCHES "^arm")
    set(COBALT_ARCH       "arm")
    set(COBALT_ARCH_SHORT "arm")
else()
    message(WARNING "Unknown architecture '${CMAKE_SYSTEM_PROCESSOR}'")
    set(COBALT_ARCH       "${CMAKE_SYSTEM_PROCESSOR}")
    set(COBALT_ARCH_SHORT "${CMAKE_SYSTEM_PROCESSOR}")
endif()
message(STATUS "Detected target arch = ${COBALT_ARCH},${COBALT_ARCH_SHORT}")

# Configure various source and output paths for the project
set(COBALT_ASSETS_DIR  "${PROJECT_SOURCE_DIR}/Assets")
set(COBALT_SCRIPTS_DIR "${PROJECT_SOURCE_DIR}/Scripts")
set(COBALT_OUTPUT_ROOT "${CMAKE_INSTALL_PREFIX}")
set(COBALT_SDK_ROOT    "${COBALT_OUTPUT_ROOT}/SDK_$<CONFIG>")
set(COBALT_SDK_BIN_DIR "${COBALT_SDK_ROOT}/Bin/${COBALT_ARCH}")
set(COBALT_SDK_LIB_DIR "${COBALT_SDK_ROOT}/Lib/${COBALT_ARCH}")
set(COBALT_INTERNAL_INCLUDE_DIR "${PROJECT_SOURCE_DIR}/Libraries")
set(COBALT_SDK_INCLUDE_DIR "${COBALT_SDK_ROOT}/Include/Cobalt")
set(COBALT_PROJECT_LICENSE_FILE "${PROJECT_SOURCE_DIR}/LICENSE.txt")
set(COBALT_THIRD_PARTY_LICENSES_FILE "${PROJECT_SOURCE_DIR}/THIRD_PARTY_LICENSES.md")
set(COBALT_VERSION_YAML_FILE "${PROJECT_SOURCE_DIR}/VersionInfo/version.yml")
include(FileHelperFunctions)

# Define a helper variable to give us a platform-independent shell script extension variable
if(WIN32)
    set(COBALT_SHELL_SCRIPT_EXT ".cmd")
else()
    set(COBALT_SHELL_SCRIPT_EXT ".sh")
endif()

# Generate our project level version information
include(VersionGeneration)

# Add SDK documentation generation targets
set(DOCS_SOURCE_ROOT "${PROJECT_SOURCE_DIR}/Documentation")
set(DOCS_OUTPUT_ROOT "${COBALT_SDK_ROOT}/Documentation")
include(DocToolchain)
cobalt_setup_doc_toolchain(DOC_PYTHON DOC_XSLTPROC)
if(DEFINED DOC_PYTHON AND DEFINED DOC_XSLTPROC)
    add_custom_target(documentation
        COMMAND "${DOC_PYTHON}" -u
                "${COBALT_SCRIPTS_DIR}/BuildDocumentation.py"
                --xslt-tool "${DOC_XSLTPROC}"
                --source-root "${DOCS_SOURCE_ROOT}"
                --output-root "${DOCS_OUTPUT_ROOT}"
                --toc-xml "${PROJECT_SOURCE_DIR}/Libraries/Cobalt/RendererInterface/Documentation/TOCRoot.xml"
                --project-xml "${PROJECT_SOURCE_DIR}/Libraries/Cobalt/RendererInterface/Documentation/Project.xml"
                --xml-input-dirs
                    "${PROJECT_SOURCE_DIR}/Libraries/Cobalt/RendererInterface/Documentation"
                    "${PROJECT_SOURCE_DIR}/Libraries/Cobalt/Marshalling/Documentation"
                    "${PROJECT_SOURCE_DIR}/Libraries/Cobalt/LoggingInterface/Documentation"
                --resource-dirs
                    "${DOCS_SOURCE_ROOT}/DocResources"
                    "${PROJECT_SOURCE_DIR}/Libraries/Cobalt/RendererInterface/Documentation/DocResources"
                    "${PROJECT_SOURCE_DIR}/Libraries/Cobalt/Marshalling/Documentation/DocResources"
                    "${PROJECT_SOURCE_DIR}/Libraries/Cobalt/LoggingInterface/Documentation/DocResources"
        WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
        COMMENT "Generating SDK documentation"
        VERBATIM
    )
    set_target_properties(documentation PROPERTIES FOLDER "CustomTargets")
endif()

# Include external libraries this project depends on
set(COBALT_EXTERNAL_ROOT "${CMAKE_BINARY_DIR}/External" CACHE PATH "Root directory of external dependencies")
add_subdirectory(External)

# Initialize formatting/static analysis
include(ClangFormat)
include(ClangTidy)
cobalt_setup_clang_tidy()
include(VisualStudioStaticAnalysis)
cobalt_setup_visual_studio_static_analysis()

# Configure compiler warning settings
include(CompilerWarningLevel)

# Configure compiler optimization settings
include(CompilerOptimizationSettings)

# Recurse into the source directories we need to compile
add_subdirectory(Libraries)
add_subdirectory(Renderers)
if(COBALT_BUILD_UTILITIES)
    add_subdirectory(Utilities)
endif()
if(COBALT_BUILD_TESTS)
    add_subdirectory(Testing)
endif()
if(COBALT_BUILD_EXAMPLES)
    add_subdirectory(Examples)
endif()

# Finalise formatting/static analysis
cobalt_finalize_clang_format()
cobalt_finalize_clang_tidy()

# Add custom testing targets
if(COBALT_BUILD_TESTS)
    if(WIN32)
    add_custom_target(run-all-unit-tests
        COMMAND "${CMAKE_COMMAND}" --install "${CMAKE_BINARY_DIR}" --config "$<CONFIG>"
        COMMAND cmd.exe /c "${COBALT_SCRIPTS_DIR}/RunAllUnitTests.cmd" "${COBALT_OUTPUT_ROOT}/AutomatedTests_${COBALT_ARCH}_$<CONFIG>" "${COBALT_OUTPUT_ROOT}/TestResults/UnitTestResults"
        USES_TERMINAL
        DEPENDS AutomatedTests
    )
    add_custom_target(run-all-performance-tests
        COMMAND "${CMAKE_COMMAND}" --install "${CMAKE_BINARY_DIR}" --config "$<CONFIG>"
        COMMAND cmd.exe /c "${COBALT_SCRIPTS_DIR}/RunAllPerformanceTests.cmd" "${COBALT_OUTPUT_ROOT}/AutomatedTests_${COBALT_ARCH}_$<CONFIG>" "${COBALT_OUTPUT_ROOT}/TestResults/PerformanceTestResults"
        USES_TERMINAL
        DEPENDS AutomatedTests
    )
    else()
    add_custom_target(run-all-unit-tests
        COMMAND "${CMAKE_COMMAND}" --install "${CMAKE_BINARY_DIR}" --config "$<CONFIG>"
        COMMAND "${COBALT_SCRIPTS_DIR}/RunAllUnitTests.sh" "${COBALT_OUTPUT_ROOT}/AutomatedTests_${COBALT_ARCH}_$<CONFIG>" "${COBALT_OUTPUT_ROOT}/TestResults/UnitTestResults"
        USES_TERMINAL
        DEPENDS AutomatedTests
    )
    add_custom_target(run-all-performance-tests
        COMMAND "${CMAKE_COMMAND}" --install "${CMAKE_BINARY_DIR}" --config "$<CONFIG>"
        COMMAND "${COBALT_SCRIPTS_DIR}/RunAllPerformanceTests.sh" "${COBALT_OUTPUT_ROOT}/AutomatedTests_${COBALT_ARCH}_$<CONFIG>" "${COBALT_OUTPUT_ROOT}/TestResults/PerformanceTestResults"
        USES_TERMINAL
        DEPENDS AutomatedTests
    )
    endif()
    set_target_properties(run-all-unit-tests PROPERTIES FOLDER "CustomTargets")
    set_target_properties(run-all-performance-tests PROPERTIES FOLDER "CustomTargets")
endif()

# Add unit test code coverage report target
if(COBALT_BUILD_TESTS AND MSVC)
    if(DEFINED DOC_PYTHON AND NOT "${DOC_PYTHON}" STREQUAL "")
        set(COBALT_CODE_COVERAGE_PYTHON "${DOC_PYTHON}")
    else()
        find_package(Python3 COMPONENTS Interpreter REQUIRED)
        set(COBALT_CODE_COVERAGE_PYTHON "${Python3_EXECUTABLE}")
    endif()
    add_custom_target(run-code-coverage
        COMMAND "${CMAKE_COMMAND}" --install "${CMAKE_BINARY_DIR}" --config "$<CONFIG>"
        COMMAND "${COBALT_CODE_COVERAGE_PYTHON}" -u
                "${COBALT_SCRIPTS_DIR}/BuildCodeCoverageReport.py"
                --source-root "${PROJECT_SOURCE_DIR}"
                --test-root "${COBALT_OUTPUT_ROOT}/AutomatedTests_${COBALT_ARCH}_$<CONFIG>"
                --output-root "${COBALT_OUTPUT_ROOT}/CodeCoverageReport"
                --configuration "$<CONFIG>"
        WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
        USES_TERMINAL
        DEPENDS AutomatedTests
        COMMENT "Running renderer plugin unit test code coverage"
        VERBATIM
    )
    set_target_properties(run-code-coverage PROPERTIES FOLDER "CustomTargets")
endif()

# Add CMake package files for SDK consumers
include(CMakePackageConfigHelpers)
set(COBALT_PACKAGE_VERSION "${COBALT_VERSION_MAJOR}.${COBALT_VERSION_MINOR}.${COBALT_VERSION_PATCH}")
set(COBALT_PACKAGE_GENERATED_DIR "${CMAKE_BINARY_DIR}/generated/CobaltRendererPackage")
set(COBALT_PACKAGE_INSTALL_DIR "${COBALT_SDK_ROOT}/cmake")
file(MAKE_DIRECTORY "${COBALT_PACKAGE_GENERATED_DIR}")
configure_package_config_file(
    "${PROJECT_SOURCE_DIR}/cmake/CobaltRendererConfig.cmake.in"
    "${COBALT_PACKAGE_GENERATED_DIR}/CobaltRendererConfig.cmake"
    INSTALL_DESTINATION "cmake"
    NO_SET_AND_CHECK_MACRO
    NO_CHECK_REQUIRED_COMPONENTS_MACRO
)
write_basic_package_version_file(
    "${COBALT_PACKAGE_GENERATED_DIR}/CobaltRendererConfigVersion.cmake"
    VERSION "${COBALT_PACKAGE_VERSION}"
    COMPATIBILITY SameMajorVersion
)
install(
    FILES
        "${COBALT_PACKAGE_GENERATED_DIR}/CobaltRendererConfig.cmake"
        "${COBALT_PACKAGE_GENERATED_DIR}/CobaltRendererConfigVersion.cmake"
    DESTINATION "${COBALT_PACKAGE_INSTALL_DIR}"
)

# Add additional files to our SDK output directory
install(
    FILES
        "${COBALT_PROJECT_LICENSE_FILE}"
        "${COBALT_THIRD_PARTY_LICENSES_FILE}"
        "${COBALT_VERSION_YAML_FILE}"
    DESTINATION "${COBALT_SDK_ROOT}"
)

# Add a target to build our external examples (currently only SDLExample)
if(COBALT_BUILD_EXAMPLES)
    if(COBALT_USE_SDL3)
        add_custom_target(build-standalone-examples
            COMMAND "${CMAKE_COMMAND}"
                    -S "${PROJECT_SOURCE_DIR}/Examples/SDLExample/Standalone"
                    -B "${CMAKE_BINARY_DIR}/StandaloneExamples/SDLExample_$<CONFIG>"
                    -G "${CMAKE_GENERATOR}"
                    "-DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}"
                    "-DCMAKE_BUILD_TYPE=$<CONFIG>"
                    "-DCOBALT_RENDERER_SDK_ROOT=${COBALT_SDK_ROOT}"
                    "-DSDL3_DIR=${SDL3_BINARY_DIR}"
            COMMAND "${CMAKE_COMMAND}" --build "${CMAKE_BINARY_DIR}/StandaloneExamples/SDLExample_$<CONFIG>" --target SDLExample --config "$<CONFIG>"
            USES_TERMINAL
            COMMENT "Building standalone examples against the installed SDK"
            VERBATIM
        )
        set_target_properties(build-standalone-examples PROPERTIES FOLDER "CustomTargets")
    endif()
endif()
