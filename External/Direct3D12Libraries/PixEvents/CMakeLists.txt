cmake_minimum_required(VERSION 3.20)

project(PixEvents LANGUAGES CXX)
include(FetchContent)

FetchContent_Declare(PixEvents
    GIT_REPOSITORY https://github.com/microsoft/PixEvents.git
    GIT_TAG        3a7e70dde7bf54f02f9d2e9dd6d3350c6cfb962f
    SOURCE_DIR     "${COBALT_EXTERNAL_ROOT}/Cache/PixEvents"
    PATCH_COMMAND git apply "${CMAKE_CURRENT_SOURCE_DIR}/PixEvents.patch"
    UPDATE_DISCONNECTED TRUE
)
FetchContent_MakeAvailable(PixEvents)
set(PIX_EVENTS_SOURCE_DIR "${pixevents_SOURCE_DIR}")

file(GLOB_RECURSE PIXEVENTS_RUNTIME_SOURCES
    "${PIX_EVENTS_SOURCE_DIR}/runtime/dll/shared/*.cpp"
    "${PIX_EVENTS_SOURCE_DIR}/runtime/lib/*.cpp"
    "${PIX_EVENTS_SOURCE_DIR}/shared/*.cpp"
)

# Define the WinPixEventRuntime library
add_library(WinPixEventRuntime SHARED ${PIXEVENTS_RUNTIME_SOURCES})
add_library(Microsoft::WinPixEventRuntime ALIAS WinPixEventRuntime)
target_include_directories(WinPixEventRuntime
    SYSTEM
    PUBLIC
        "${PIX_EVENTS_SOURCE_DIR}/include"
    PRIVATE
        "${PIX_EVENTS_SOURCE_DIR}"
        "${PIX_EVENTS_SOURCE_DIR}/shared"
        "${PIX_EVENTS_SOURCE_DIR}/intermediates"
        "${PIX_EVENTS_SOURCE_DIR}/runtime"
        "${PIX_EVENTS_SOURCE_DIR}/third_party/wil/include"
)
target_compile_definitions(WinPixEventRuntime
    PRIVATE
        _CRT_SECURE_NO_WARNINGS
        WIN32_LEAN_AND_MEAN
        NOMINMAX
        NDEBUG
        USE_PIX
        USE_PIX_ON_ALL_ARCHITECTURES
)
target_compile_options(WinPixEventRuntime PRIVATE
    "/FI${PIX_EVENTS_SOURCE_DIR}/runtime/lib/pch.h"
)
target_link_libraries(WinPixEventRuntime
    PRIVATE
        advapi32
)

# Generate PixEvt.h
set(PIX_ETW_MAN "${PIX_EVENTS_SOURCE_DIR}/runtime/lib/PixEtw.man")
set(PIX_ETW_GEN_DIR "${CMAKE_CURRENT_BINARY_DIR}/PixEtw")
set(PIX_ETW_HEADER "${PIX_ETW_GEN_DIR}/PixEtw.h")
set(PIX_ETW_RC     "${PIX_ETW_GEN_DIR}/PixEtw.rc")
find_program(MC_EXE mc.exe)
if(NOT MC_EXE)
    message(FATAL_ERROR
        "Could not find mc.exe (Windows Message Compiler). "
        "Make sure you're configuring from a VS Developer Command Prompt "
        "or have mc.exe on your PATH."
    )
endif()
add_custom_command(
    OUTPUT  "${PIX_ETW_HEADER}" "${PIX_ETW_RC}"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${PIX_ETW_GEN_DIR}"
    # Optionally set WORKING_DIRECTORY if the .man uses relative paths:
    # WORKING_DIRECTORY "${PIX_EVENTS_SOURCE_DIR}/runtime/lib"
    COMMAND "${MC_EXE}"
            "${PIX_ETW_MAN}"
            -um
            -P ETW_EVENT
            -r "${PIX_ETW_GEN_DIR}"
            -h "${PIX_ETW_GEN_DIR}"
    DEPENDS "${PIX_ETW_MAN}"
    COMMENT "Running mc.exe on PixEtw.man to generate PixEtw.h / PixEtw.rc"
    VERBATIM
)
set_source_files_properties(
    "${PIX_ETW_HEADER}" "${PIX_ETW_RC}"
    PROPERTIES GENERATED TRUE
)
target_sources(WinPixEventRuntime
    PRIVATE
        "${PIX_ETW_HEADER}"   # header (for dependencies)
        "${PIX_ETW_RC}"       # .rc will be compiled as a resource on MSVC
)
target_include_directories(WinPixEventRuntime
    PRIVATE
        "${PIX_ETW_GEN_DIR}"
)

# Include the .def file to define our exports. We can't use the one supplied in the project under
# ${PIX_EVENTS_SOURCE_DIR}/runtime/dll/desktop/WinPixEventRuntime.def, as it includes the
# PIXBeginCapture2 and PIXEndCapture functions unconditionally, and when compiling under clang-cl,
# the linker will fail the build if those aren't present, while MSVC ignores them. We've removed
# the capture API functionality, so we need to stub these out.
set(WINPIX_DEF_FILE "${CMAKE_CURRENT_SOURCE_DIR}/WinPixEventRuntime.def")
target_link_options(WinPixEventRuntime PRIVATE "/DEF:${WINPIX_DEF_FILE}")

# Set the virtual folder for the target
set_project_virtual_folder(WinPixEventRuntime "External/PixEvents")
