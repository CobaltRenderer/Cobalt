# This file contains the required logic to setup project-wide version information, suitable for embedding into compiled
# assemblies.
include_guard(GLOBAL)

# Determine the current semantic version number based on the version.yml file
set(COBALT_VERSION_MAJOR 0)
set(COBALT_VERSION_MINOR 0)
set(COBALT_VERSION_PATCH 0)
set(COBALT_VERSION_PRERELEASE "")
set(_version_yaml_path "${PROJECT_SOURCE_DIR}/VersionInfo/version.yml")
if(NOT EXISTS "${_version_yaml_path}")
    message(FATAL_ERROR "Version file not found: ${_version_yaml_path}")
endif()
file(READ "${_version_yaml_path}" _version_yaml)
string(REGEX MATCH "version[ \t]*:[ \t]*['\"]?((0|[1-9][0-9]*)\\.((0|[1-9][0-9]*))\\.((0|[1-9][0-9]*))(-[0-9A-Za-z][0-9A-Za-z.-]*)?)['\"]?" _ver_match "${_version_yaml}")
if(NOT _ver_match)
    message(FATAL_ERROR "VersionInfo/version.yml must contain a SemVer value in the form 'version: MAJOR.MINOR.PATCH[-PRERELEASE]'")
endif()

# Determine the major.minor.patch version number based on the version.yml file
set(COBALT_VERSION_LABEL "${CMAKE_MATCH_1}")
set(COBALT_VERSION_MAJOR "${CMAKE_MATCH_2}")
set(COBALT_VERSION_MINOR "${CMAKE_MATCH_3}")
set(COBALT_VERSION_PATCH "${CMAKE_MATCH_5}")
set(COBALT_VERSION_PRERELEASE "${CMAKE_MATCH_7}")

# Generate our build version number from the git commit history
set(COBALT_VERSION_BUILD 0)
set(COBALT_VERSION_BUILD_STRING "${COBALT_VERSION_BUILD}-unknown")
set(COBALT_VERSION_IS_CLEAN_RELEASE OFF)
find_package(Git QUIET)
if(GIT_FOUND AND EXISTS "${PROJECT_SOURCE_DIR}/.git")
    execute_process(
        COMMAND "${GIT_EXECUTABLE}" describe --tags --match "v*" --long --dirty --abbrev=7
        WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
        OUTPUT_VARIABLE GIT_VERSION_DESCRIBE
        RESULT_VARIABLE GIT_DESCRIBE_RESULT
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
    if(GIT_DESCRIBE_RESULT EQUAL 0 AND GIT_VERSION_DESCRIBE MATCHES "^(.+)-([0-9]+)-g([0-9a-f]+)(-dirty)?$")
        set(_git_describe_tag "${CMAKE_MATCH_1}")
        set(_git_describe_distance "${CMAKE_MATCH_2}")
        set(_git_describe_hash "${CMAKE_MATCH_3}")
        set(_git_describe_dirty "${CMAKE_MATCH_4}")
        set(COBALT_VERSION_BUILD "${_git_describe_distance}")
        set(COBALT_VERSION_BUILD_STRING "${_git_describe_distance}-g${_git_describe_hash}")
        if(_git_describe_dirty)
            string(APPEND COBALT_VERSION_BUILD_STRING "-dirty")
        endif()
        if(_git_describe_distance STREQUAL "0" AND NOT _git_describe_dirty AND _git_describe_tag STREQUAL "v${COBALT_VERSION_LABEL}")
            set(COBALT_VERSION_IS_CLEAN_RELEASE ON)
            set(COBALT_VERSION_BUILD_STRING "")
        endif()
    else()
        # Fallback if there are no version tags - count commits back to the start of the history.
        execute_process(
            COMMAND "${GIT_EXECUTABLE}" rev-list --count HEAD
            WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
            OUTPUT_VARIABLE GIT_COMMIT_COUNT
            RESULT_VARIABLE GIT_COMMIT_COUNT_RESULT
            OUTPUT_STRIP_TRAILING_WHITESPACE
            ERROR_QUIET
        )
        execute_process(
            COMMAND "${GIT_EXECUTABLE}" rev-parse --short=7 HEAD
            WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
            OUTPUT_VARIABLE GIT_COMMIT_HASH
            RESULT_VARIABLE GIT_COMMIT_HASH_RESULT
            OUTPUT_STRIP_TRAILING_WHITESPACE
            ERROR_QUIET
        )
        execute_process(
            COMMAND "${GIT_EXECUTABLE}" diff-index --quiet HEAD --
            WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
            RESULT_VARIABLE GIT_DIRTY_RESULT
            ERROR_QUIET
        )
        if(GIT_COMMIT_COUNT_RESULT EQUAL 0 AND
           GIT_COMMIT_HASH_RESULT EQUAL 0 AND
           GIT_COMMIT_COUNT MATCHES "^[0-9]+$" AND
           GIT_COMMIT_HASH MATCHES "^[0-9a-f]+$")
            set(COBALT_VERSION_BUILD ${GIT_COMMIT_COUNT})
            set(COBALT_VERSION_BUILD_STRING "${COBALT_VERSION_BUILD}-g${GIT_COMMIT_HASH}")
            if(NOT GIT_DIRTY_RESULT EQUAL 0)
                string(APPEND COBALT_VERSION_BUILD_STRING "-dirty")
            endif()
        endif()
    endif()
endif()

# Build our version labels
set(COBALT_VERSION_LABEL_LONG "${COBALT_VERSION_MAJOR}.${COBALT_VERSION_MINOR}.${COBALT_VERSION_PATCH}.${COBALT_VERSION_BUILD}")
if(COBALT_VERSION_IS_CLEAN_RELEASE)
    set(COBALT_VERSION_LABEL_FULL "${COBALT_VERSION_LABEL}")
else()
    set(COBALT_VERSION_LABEL_FULL "${COBALT_VERSION_LABEL}+${COBALT_VERSION_BUILD_STRING}")
endif()

# Obtain the current year
string(TIMESTAMP COBALT_VERSION_YEAR "%Y")

# Generate our version info files
set(COBALT_VERSION_INFO_DIR "${CMAKE_BINARY_DIR}/generated")
file(MAKE_DIRECTORY "${COBALT_VERSION_INFO_DIR}")
configure_file(
    "${PROJECT_SOURCE_DIR}/VersionInfo/AssemblyVersionInfo.h.in"
    "${COBALT_VERSION_INFO_DIR}/AssemblyVersionInfo.h"
    @ONLY # use @VAR@ syntax only
)
set(COBALT_RENDERER_INTERFACE_GENERATED_DIR "${COBALT_VERSION_INFO_DIR}/Cobalt/RendererInterface")
set(COBALT_RENDERER_INTERFACE_VERSION_HEADER "${COBALT_RENDERER_INTERFACE_GENERATED_DIR}/CobaltVersionInfo.h")
file(MAKE_DIRECTORY "${COBALT_RENDERER_INTERFACE_GENERATED_DIR}")
configure_file(
    "${PROJECT_SOURCE_DIR}/Libraries/Cobalt/RendererInterface/CobaltVersionInfo.h.in"
    "${COBALT_RENDERER_INTERFACE_VERSION_HEADER}"
    @ONLY # use @VAR@ syntax only
)
file(COPY "${PROJECT_SOURCE_DIR}/VersionInfo/SolutionVersionInfo.h" "${PROJECT_SOURCE_DIR}/VersionInfo/VersionInfo.rc" DESTINATION "${COBALT_VERSION_INFO_DIR}")
set(COBALT_VERSION_HEADER_FILES
    "${COBALT_VERSION_INFO_DIR}/AssemblyVersionInfo.h"
    "${COBALT_VERSION_INFO_DIR}/SolutionVersionInfo.h"
)
set(COBALT_VERSION_RESOURCE_FILE "")
if(WIN32)
    list(APPEND COBALT_VERSION_RESOURCE_FILE "${COBALT_VERSION_INFO_DIR}/VersionInfo.rc")
endif()
set(COBALT_VERSION_FILES
    ${COBALT_VERSION_HEADER_FILES}
    ${COBALT_VERSION_RESOURCE_FILE}
)

# Print the version information
message(STATUS "CobaltRenderer version: ${COBALT_VERSION_LABEL_FULL} (build ${COBALT_VERSION_BUILD}, year ${COBALT_VERSION_YEAR})")
