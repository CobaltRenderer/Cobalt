include_guard(GLOBAL)

# Internal flag: used only for the helper build that exists just to produce compile_commands.json
option(CLANG_TIDY_COMPILATION_DB_ONLY "Internal: configure only for compilation database" OFF)

# Global list of targets that should be included in clang-tidy.
set_property(GLOBAL PROPERTY CLANG_TIDY_TARGETS "")

# Global list of explicitly excluded files.
set_property(GLOBAL PROPERTY CLANG_TIDY_EXCLUDE_FILES "")

# Global list of header files that should be probed as pure C.
set_property(GLOBAL PROPERTY CLANG_TIDY_PURE_C_HEADER_FILES "")

function(register_clang_tidy_target tgt)
    set(one_value_args USAGE_TARGET)
    cmake_parse_arguments(PARSE_ARGV 1 ARG "" "${one_value_args}" "")

    if(ARG_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR "register_clang_tidy_target(): unexpected arguments: ${ARG_UNPARSED_ARGUMENTS}")
    endif()

    if(NOT TARGET ${tgt})
        message(FATAL_ERROR "register_clang_tidy_target(): Target '${tgt}' does not exist")
    endif()

    get_target_property(tgt_type ${tgt} TYPE)
    if(NOT tgt_type STREQUAL "EXECUTABLE"
       AND NOT tgt_type STREQUAL "STATIC_LIBRARY"
       AND NOT tgt_type STREQUAL "SHARED_LIBRARY"
       AND NOT tgt_type STREQUAL "MODULE_LIBRARY"
       AND NOT tgt_type STREQUAL "OBJECT_LIBRARY")
        message(FATAL_ERROR "register_clang_tidy_target(): '${tgt}' is not a compile target")
    endif()

    if(ARG_USAGE_TARGET)
        set(usage_target "${ARG_USAGE_TARGET}")
    else()
        set(usage_target "${tgt}")
    endif()

    if(NOT TARGET "${usage_target}")
        message(FATAL_ERROR "register_clang_tidy_target(): usage target '${usage_target}' does not exist")
    endif()

    get_property(registered_targets GLOBAL PROPERTY CLANG_TIDY_TARGETS)
    list(FIND registered_targets "${tgt}" registered_target_index)
    if(registered_target_index EQUAL -1)
        set_property(GLOBAL APPEND PROPERTY CLANG_TIDY_TARGETS "${tgt}")
    endif()
    set_property(GLOBAL PROPERTY "CLANG_TIDY_USAGE_TARGET_${tgt}" "${usage_target}")
endfunction()

function(register_clang_format_and_tidy_target tgt)
    set(one_value_args USAGE_TARGET)
    cmake_parse_arguments(PARSE_ARGV 1 ARG "" "${one_value_args}" "")

    if(ARG_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR "register_clang_format_and_tidy_target(): unexpected arguments: ${ARG_UNPARSED_ARGUMENTS}")
    endif()

    register_clang_format_target("${tgt}")

    if(ARG_USAGE_TARGET)
        register_clang_tidy_target("${tgt}" USAGE_TARGET "${ARG_USAGE_TARGET}")
    else()
        register_clang_tidy_target("${tgt}")
    endif()
endfunction()

function(exclude_clang_tidy_files)
    foreach(file IN LISTS ARGN)
        if(NOT IS_ABSOLUTE "${file}")
            get_filename_component(abs "${file}" ABSOLUTE BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
        else()
            set(abs "${file}")
        endif()

        set_property(GLOBAL APPEND PROPERTY CLANG_TIDY_EXCLUDE_FILES "${abs}")
    endforeach()
endfunction()

function(set_clang_tidy_header_files_as_c)
    set(valid_header_suffixes .h .hh .hpp .hxx .pkg)
    foreach(file IN LISTS ARGN)
        get_filename_component(file_ext "${file}" EXT)
        list(FIND valid_header_suffixes "${file_ext}" valid_suffix_index)
        if(valid_suffix_index EQUAL -1)
            continue()
        endif()

        if(NOT IS_ABSOLUTE "${file}")
            get_filename_component(abs "${file}" ABSOLUTE BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
        else()
            set(abs "${file}")
        endif()

        set_property(GLOBAL APPEND PROPERTY CLANG_TIDY_PURE_C_HEADER_FILES "${abs}")
    endforeach()
endfunction()

# Setup step: decide where clang-tidy should look for compile_commands.json.
# Call this BEFORE add_subdirectory() calls so it runs early.
function(cobalt_setup_clang_tidy)
    if(CLANG_TIDY_COMPILATION_DB_ONLY)
        return()
    endif()

    if(CMAKE_EXPORT_COMPILE_COMMANDS)
        set_property(GLOBAL PROPERTY CLANG_TIDY_COMPILATION_DB_DIR "${CMAKE_BINARY_DIR}")
        return()
    endif()

    # Secondary build tree that uses Ninja and exports compile_commands.json
    set(helper_dir "${CMAKE_BINARY_DIR}/build-clang-tidy")

    if(NOT EXISTS "${helper_dir}/CMakeCache.txt")
        message(STATUS "Configuring helper Ninja build in ${helper_dir} for clang-tidy")

        execute_process(
            COMMAND "${CMAKE_COMMAND}"
                    -S "${CMAKE_BINARY_DIR}"
                    -B "${helper_dir}"
                    -G Ninja
                    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
                    -DCLANG_TIDY_COMPILATION_DB_ONLY=ON
            RESULT_VARIABLE _cfg_res
        )
        if(NOT _cfg_res EQUAL 0)
            message(FATAL_ERROR "Failed to configure helper Ninja build for clang-tidy")
        endif()
    endif()

    if(NOT EXISTS "${helper_dir}/compile_commands.json")
        message(FATAL_ERROR "Helper build did not produce compile_commands.json in ${helper_dir}")
    endif()

    set_property(GLOBAL PROPERTY CLANG_TIDY_COMPILATION_DB_DIR "${helper_dir}")
endfunction()

function(cobalt_clang_tidy_copy_target_property destination source property_name)
    get_target_property(property_value "${source}" "${property_name}")
    if(NOT "${property_value}" STREQUAL "property_value-NOTFOUND")
        set_target_properties("${destination}" PROPERTIES "${property_name}" "${property_value}")
    endif()
endfunction()

function(cobalt_clang_tidy_make_header_filter path output_variable)
    cmake_path(CONVERT "${path}" TO_CMAKE_PATH_LIST normalized_path)
    cmake_path(RELATIVE_PATH normalized_path
               BASE_DIRECTORY "${CMAKE_SOURCE_DIR}"
               OUTPUT_VARIABLE relative_path)

    if(relative_path MATCHES "^\\.\\.")
        set(path_regex "${normalized_path}")
    else()
        set(path_regex "${relative_path}")
    endif()

    string(REPLACE "." "\\." path_regex "${path_regex}")
    string(REPLACE "+" "\\+" path_regex "${path_regex}")
    string(REPLACE "(" "\\(" path_regex "${path_regex}")
    string(REPLACE ")" "\\)" path_regex "${path_regex}")
    string(REPLACE "[" "\\[" path_regex "${path_regex}")
    string(REPLACE "]" "\\]" path_regex "${path_regex}")
    string(REPLACE "/" "[/\\\\]" path_regex "${path_regex}")

    set(${output_variable} ".*[/\\\\]${path_regex}$" PARENT_SCOPE)
endfunction()

# On macOS, Homebrew clang-tidy may not automatically know about Apple's SDK
# sysroot or Apple libc++ headers. Add them explicitly so headers such as
# <string>, <vector>, <memory>, etc. can be found.
function(cobalt_append_apple_clang_tidy_system_args out_var)
    if(NOT APPLE)
        return()
    endif()

    set(_args "${${out_var}}")

    set(_sdk_path "")

    # Respect CMAKE_OSX_SYSROOT if the project/user already set it.
    if(CMAKE_OSX_SYSROOT)
        if(IS_ABSOLUTE "${CMAKE_OSX_SYSROOT}" AND EXISTS "${CMAKE_OSX_SYSROOT}")
            set(_sdk_path "${CMAKE_OSX_SYSROOT}")
        else()
            execute_process(
                COMMAND xcrun --sdk "${CMAKE_OSX_SYSROOT}" --show-sdk-path
                OUTPUT_VARIABLE _xcrun_sdk_path
                RESULT_VARIABLE _xcrun_sdk_result
                OUTPUT_STRIP_TRAILING_WHITESPACE
                ERROR_QUIET
            )

            if(_xcrun_sdk_result EQUAL 0 AND EXISTS "${_xcrun_sdk_path}")
                set(_sdk_path "${_xcrun_sdk_path}")
            endif()
        endif()
    endif()

    # Fallback to the default active macOS SDK.
    if(NOT _sdk_path)
        execute_process(
            COMMAND xcrun --show-sdk-path
            OUTPUT_VARIABLE _xcrun_default_sdk_path
            RESULT_VARIABLE _xcrun_default_sdk_result
            OUTPUT_STRIP_TRAILING_WHITESPACE
            ERROR_QUIET
        )

        if(_xcrun_default_sdk_result EQUAL 0 AND EXISTS "${_xcrun_default_sdk_path}")
            set(_sdk_path "${_xcrun_default_sdk_path}")
        endif()
    endif()

    if(_sdk_path)
        list(APPEND _args
            "--extra-arg=-isysroot"
            "--extra-arg=${_sdk_path}"
        )
        message(STATUS "clang-tidy macOS SDK sysroot: ${_sdk_path}")
    else()
        message(WARNING "Could not determine macOS SDK path with xcrun; clang-tidy may not find system headers")
    endif()

    # Find Apple's clang++ and derive the libc++ include directory from the
    # Xcode / Command Line Tools toolchain.
    execute_process(
        COMMAND xcrun --find clang++
        OUTPUT_VARIABLE _apple_clangxx
        RESULT_VARIABLE _apple_clangxx_result
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )

    if(_apple_clangxx_result EQUAL 0 AND EXISTS "${_apple_clangxx}")
        get_filename_component(_apple_clangxx_bin_dir "${_apple_clangxx}" DIRECTORY)
        get_filename_component(_libcxx_include "${_apple_clangxx_bin_dir}/../include/c++/v1" ABSOLUTE)

        if(EXISTS "${_libcxx_include}")
            list(APPEND _args
                "--extra-arg=-isystem"
                "--extra-arg=${_libcxx_include}"
            )
            message(STATUS "clang-tidy macOS libc++ include: ${_libcxx_include}")
        else()
            message(WARNING "Could not find Apple libc++ headers at ${_libcxx_include}; clang-tidy may not find C++ standard headers")
        endif()
    else()
        message(WARNING "Could not find Apple clang++ with xcrun; clang-tidy may not find C++ standard headers")
    endif()

    if(CMAKE_OSX_DEPLOYMENT_TARGET)
        list(APPEND _args
            "--extra-arg=-mmacosx-version-min=${CMAKE_OSX_DEPLOYMENT_TARGET}"
        )
    endif()

    set(${out_var} "${_args}" PARENT_SCOPE)
endfunction()

# Finalise clang-tidy rules AFTER all targets/subdirectories are added.
function(cobalt_finalize_clang_tidy)
    # Find compile_commands.json directory
    get_property(CLANG_TIDY_COMPILATION_DB_DIR GLOBAL PROPERTY CLANG_TIDY_COMPILATION_DB_DIR)
    if(NOT CLANG_TIDY_COMPILATION_DB_DIR)
        message(STATUS "No clang-tidy compilation database directory set; clang-tidy targets will not be created")
        return()
    endif()

    get_property(CLANG_TIDY_TARGETS GLOBAL PROPERTY CLANG_TIDY_TARGETS)
    list(REMOVE_DUPLICATES CLANG_TIDY_TARGETS)
    if(NOT CLANG_TIDY_TARGETS)
        message(STATUS "No targets registered for clang-tidy")
        return()
    endif()

    # Only run clang-tidy directly on real translation units.
    set(TIDY_SUFFIXES .c .cc .cxx .cpp)
    set(ALL_TIDY_SOURCES "")
    # Newer clang-tidy versions default to reporting diagnostics from every
    # non-system header. Keep source translation-unit checks source-only; the
    # generated header probes below opt in to exact header filters.
    set(CLANG_TIDY_SOURCE_HEADER_FILTER "^$")

    # Header-like files do not have compile_commands.json entries, so clang-tidy
    # cannot use the right target context when run on them directly. Generate a
    # build-only wrapper TU per header-like file and attach it to an excluded
    # object library that mirrors the owning target's compile context.
    set(CLANG_TIDY_HEADER_SUFFIXES .h .hh .hpp .hxx .pkg)
    set(CLANG_TIDY_HEADER_PROBE_ROOT "${CMAKE_BINARY_DIR}/clang-tidy-header-probes")
    set(CLANG_TIDY_NO_INL "__COBALT_CLANG_TIDY_NO_INL__")
    set(CLANG_TIDY_PROBED_HEADERS "")
    set(CLANG_TIDY_HEADER_PROBE_SOURCES "")
    set(CLANG_TIDY_HEADER_SOURCE_FILES "")
    set(CLANG_TIDY_HEADER_INL_FILES "")

    file(MAKE_DIRECTORY "${CLANG_TIDY_HEADER_PROBE_ROOT}")

    get_property(EXCLUDED_FILES GLOBAL PROPERTY CLANG_TIDY_EXCLUDE_FILES)
    set(NORMALIZED_EXCLUDED_FILES "")
    foreach(excluded_file IN LISTS EXCLUDED_FILES)
        cmake_path(CONVERT "${excluded_file}" TO_CMAKE_PATH_LIST excluded_file)
        list(APPEND NORMALIZED_EXCLUDED_FILES "${excluded_file}")
    endforeach()

    # Find clang-tidy, allowing users to override CLANG_TIDY_EXE if they want a specific binary.
    if(NOT CLANG_TIDY_EXE)
        # On macOS, prefer Homebrew LLVM locations because Homebrew's llvm formula
        # is keg-only and clang-tidy may not be on PATH.
        if(APPLE)
            find_program(CLANG_TIDY_EXE
                NAMES clang-tidy
                PATHS
                    /opt/homebrew/opt/llvm/bin
                    /usr/local/opt/llvm/bin
                NO_DEFAULT_PATH
            )
        endif()

        # Fallback for non-Apple systems, or for Apple systems where clang-tidy
        # is already available on PATH or installed somewhere else.
        if(NOT CLANG_TIDY_EXE)
            find_program(CLANG_TIDY_EXE NAMES clang-tidy)
        endif()
    endif()

    if(NOT CLANG_TIDY_EXE)
        message(STATUS "clang-tidy not found; clang-tidy targets will not be created")
        return()
    endif()

    # Build a list of --extra-arg=... entries for clang-tidy, from project warning flags,
    # language settings, and platform-specific system header locations.
    set(CLANG_TIDY_COMMON_ARGS "")
    list(APPEND CLANG_TIDY_COMMON_ARGS "--quiet")
    list(APPEND CLANG_TIDY_COMMON_ARGS "--config-file=${CMAKE_SOURCE_DIR}/.clang-tidy")

    # Homebrew clang-tidy on macOS often needs explicit Apple SDK/libc++ paths
    # to resolve system headers such as <string>.
    cobalt_append_apple_clang_tidy_system_args(CLANG_TIDY_COMMON_ARGS)

    if(MSVC AND DEFINED COBALT_CLANG_WARNING_FLAGS)
        foreach(flag IN LISTS COBALT_CLANG_WARNING_FLAGS)
            list(APPEND CLANG_TIDY_COMMON_ARGS "--extra-arg=${flag}")
        endforeach()
    endif()

    if(DEFINED PROJECT_CLANG_WARNING_FLAGS)
        foreach(flag IN LISTS PROJECT_CLANG_WARNING_FLAGS)
            list(APPEND CLANG_TIDY_COMMON_ARGS "--extra-arg=${flag}")
        endforeach()
    endif()

    # Directory to drop "stamp" files into (to give each custom command an output)
    set(CLANG_TIDY_STAMP_DIR "${CMAKE_BINARY_DIR}/clang-tidy-stamps")
    file(MAKE_DIRECTORY "${CLANG_TIDY_STAMP_DIR}")

    set(CLANG_TIDY_CHECK_OUTPUTS "")
    set(CLANG_TIDY_FIX_OUTPUTS "")
    set(CLANG_TIDY_GENERATED_DEPENDENCIES "")

    # libpng may generate pnglibconf.h at build time when an AWK implementation
    # is available. Standalone clang-tidy runs do not build png_static first, so
    # make each tidy command wait for libpng's generated public headers.
    if(TARGET png_genfiles)
        list(APPEND CLANG_TIDY_GENERATED_DEPENDENCIES png_genfiles)
    endif()

    foreach(target IN LISTS CLANG_TIDY_TARGETS)
        get_target_property(target_sources "${target}" SOURCES)
        if(NOT target_sources)
            continue()
        endif()

        get_target_property(target_source_dir "${target}" SOURCE_DIR)
        if("${target_source_dir}" STREQUAL "target_source_dir-NOTFOUND")
            set(target_source_dir "${CMAKE_CURRENT_SOURCE_DIR}")
        endif()

        get_property(target_usage_target GLOBAL PROPERTY "CLANG_TIDY_USAGE_TARGET_${target}")
        if(NOT target_usage_target)
            set(target_usage_target "${target}")
        endif()

        set(target_header_probe_sources "")

        foreach(source_file IN LISTS target_sources)
            if(source_file MATCHES "^\\$<")
                continue()
            endif()

            get_filename_component(source_ext "${source_file}" EXT)
            list(FIND TIDY_SUFFIXES "${source_ext}" tidy_source_index)
            list(FIND CLANG_TIDY_HEADER_SUFFIXES "${source_ext}" header_suffix_index)
            if(tidy_source_index EQUAL -1 AND header_suffix_index EQUAL -1)
                continue()
            endif()

            if(NOT IS_ABSOLUTE "${source_file}")
                get_filename_component(source_file "${target_source_dir}/${source_file}" ABSOLUTE)
            endif()
            cmake_path(NORMAL_PATH source_file OUTPUT_VARIABLE source_file)
            cmake_path(CONVERT "${source_file}" TO_CMAKE_PATH_LIST source_file)

            cmake_path(RELATIVE_PATH source_file
                       BASE_DIRECTORY "${CMAKE_SOURCE_DIR}"
                       OUTPUT_VARIABLE source_relative_path)
            if(source_relative_path MATCHES "^\\.\\.")
                continue()
            endif()

            if(NORMALIZED_EXCLUDED_FILES)
                list(FIND NORMALIZED_EXCLUDED_FILES "${source_file}" excluded_index)
                if(NOT excluded_index EQUAL -1)
                    continue()
                endif()
            endif()

            if(NOT tidy_source_index EQUAL -1)
                list(APPEND ALL_TIDY_SOURCES "${source_file}")
            endif()

            if(header_suffix_index EQUAL -1)
                continue()
            endif()

            list(FIND CLANG_TIDY_PROBED_HEADERS "${source_file}" duplicate_header_index)
            if(NOT duplicate_header_index EQUAL -1)
                continue()
            endif()

            string(MD5 header_probe_id "${target}:${source_file}")
            get_property(PURE_C_HEADER_FILES GLOBAL PROPERTY CLANG_TIDY_PURE_C_HEADER_FILES)
            list(FIND PURE_C_HEADER_FILES "${source_file}" pure_c_header_index)
            if(pure_c_header_index EQUAL -1)
                set(header_probe_ext ".cpp")
            else()
                set(header_probe_ext ".c")
            endif()

            set(header_probe_source "${CLANG_TIDY_HEADER_PROBE_ROOT}/${target}_${header_probe_id}${header_probe_ext}")
            if(header_probe_ext STREQUAL ".c")
                file(WRITE "${header_probe_source}" "// Generated by cmake/ClangTidy.cmake for clang-tidy header checks.\n#include \"${source_file}\"\ntypedef int cobalt_clang_tidy_header_probe_anchor_t;\n")
            else()
                file(WRITE "${header_probe_source}" "// Generated by cmake/ClangTidy.cmake for clang-tidy header checks.\n#include \"${source_file}\"\n")
            endif()
            set_source_files_properties("${header_probe_source}" PROPERTIES GENERATED TRUE)

            get_filename_component(header_dir "${source_file}" DIRECTORY)
            get_filename_component(header_name_we "${source_file}" NAME_WE)
            set(header_inl_file "${header_dir}/${header_name_we}.inl")
            if(NOT EXISTS "${header_inl_file}")
                set(header_inl_file "${CLANG_TIDY_NO_INL}")
            endif()

            list(APPEND CLANG_TIDY_PROBED_HEADERS "${source_file}")
            list(APPEND CLANG_TIDY_HEADER_PROBE_SOURCES "${header_probe_source}")
            list(APPEND CLANG_TIDY_HEADER_SOURCE_FILES "${source_file}")
            list(APPEND CLANG_TIDY_HEADER_INL_FILES "${header_inl_file}")
            list(APPEND target_header_probe_sources "${header_probe_source}")
        endforeach()

        if(target_header_probe_sources)
            set(header_probe_target "clang-tidy-header-probes-${target}")
            add_library("${header_probe_target}" OBJECT EXCLUDE_FROM_ALL ${target_header_probe_sources})
            set_target_properties("${header_probe_target}" PROPERTIES
                EXCLUDE_FROM_DEFAULT_BUILD TRUE
                FOLDER "CustomTargets/clang-tidy-header-probes"
            )

            foreach(property_name
                COMPILE_DEFINITIONS
                COMPILE_FEATURES
                COMPILE_OPTIONS
                INCLUDE_DIRECTORIES
                INTERPROCEDURAL_OPTIMIZATION
                MSVC_RUNTIME_LIBRARY
                POSITION_INDEPENDENT_CODE
                SYSTEM_INCLUDE_DIRECTORIES
                C_STANDARD
                C_STANDARD_REQUIRED
                C_EXTENSIONS
                CXX_EXTENSIONS
                CXX_STANDARD
                CXX_STANDARD_REQUIRED
            )
                cobalt_clang_tidy_copy_target_property("${header_probe_target}" "${target}" "${property_name}")
            endforeach()

            get_target_property(link_libraries "${target}" LINK_LIBRARIES)
            if(link_libraries AND NOT "${link_libraries}" STREQUAL "link_libraries-NOTFOUND")
                target_link_libraries("${header_probe_target}" PRIVATE ${link_libraries})
            endif()

            if(NOT "${target_usage_target}" STREQUAL "${target}")
                target_link_libraries("${header_probe_target}" PRIVATE "${target_usage_target}")
            endif()
        endif()
    endforeach()

    list(REMOVE_DUPLICATES ALL_TIDY_SOURCES)

    if(NOT ALL_TIDY_SOURCES AND NOT CLANG_TIDY_HEADER_PROBE_SOURCES)
        message(STATUS "No source files selected for clang-tidy")
        return()
    endif()

    foreach(src IN LISTS ALL_TIDY_SOURCES)
        # Nice project-root-relative path for messages
        cmake_path(CONVERT "${src}" TO_CMAKE_PATH_LIST src_norm)
        cmake_path(RELATIVE_PATH src_norm
                   BASE_DIRECTORY "${CMAKE_SOURCE_DIR}"
                   OUTPUT_VARIABLE rel)

        # Make a stable id for this file (to name the stamp)
        string(MD5 tidy_id "${src}")
        set(check_stamp "${CLANG_TIDY_STAMP_DIR}/check_${tidy_id}.stamp")
        set(fix_stamp   "${CLANG_TIDY_STAMP_DIR}/fix_${tidy_id}.stamp")

        # Build common arg list: clang-tidy [common args] -p <db> -format-style=none <file>
        set(check_cmd "${CLANG_TIDY_EXE}")
        foreach(a IN LISTS CLANG_TIDY_COMMON_ARGS)
            list(APPEND check_cmd "${a}")
        endforeach()
        if(CLANG_TIDY_COMPILATION_DB_DIR)
            list(APPEND check_cmd -p "${CLANG_TIDY_COMPILATION_DB_DIR}")
        endif()
        list(APPEND check_cmd -format-style=none "--header-filter=${CLANG_TIDY_SOURCE_HEADER_FILTER}" "${src}")

        # Same but with -fix/-fix-errors
        set(fix_cmd "${CLANG_TIDY_EXE}")
        foreach(a IN LISTS CLANG_TIDY_COMMON_ARGS)
            list(APPEND fix_cmd "${a}")
        endforeach()
        if(CLANG_TIDY_COMPILATION_DB_DIR)
            list(APPEND fix_cmd -p "${CLANG_TIDY_COMPILATION_DB_DIR}")
        endif()
        list(APPEND fix_cmd -format-style=none -fix -fix-errors "--header-filter=${CLANG_TIDY_SOURCE_HEADER_FILTER}" "${src}")

        # One custom command for "check" for this file
        add_custom_command(
            OUTPUT  "${check_stamp}"
#            COMMAND ${CMAKE_COMMAND} -E echo "[clang-tidy] ${rel}"
            COMMAND ${check_cmd}
            COMMAND ${CMAKE_COMMAND} -E touch "${check_stamp}"
            DEPENDS "${src}" ${CLANG_TIDY_GENERATED_DEPENDENCIES}
            COMMENT "clang-tidy: ${rel}"
            VERBATIM
        )

        # One custom command for "fix" for this file
        add_custom_command(
            OUTPUT  "${fix_stamp}"
#            COMMAND ${CMAKE_COMMAND} -E echo "[clang-tidy-fix] ${rel}"
            COMMAND ${fix_cmd}
            COMMAND ${CMAKE_COMMAND} -E touch "${fix_stamp}"
            DEPENDS "${src}" ${CLANG_TIDY_GENERATED_DEPENDENCIES}
            COMMENT "clang-tidy-fix: ${rel}"
            VERBATIM
        )

        list(APPEND CLANG_TIDY_CHECK_OUTPUTS "${check_stamp}")
        list(APPEND CLANG_TIDY_FIX_OUTPUTS   "${fix_stamp}")
    endforeach()

    list(LENGTH CLANG_TIDY_HEADER_PROBE_SOURCES header_probe_count)
    if(header_probe_count GREATER 0)
        math(EXPR header_probe_last_index "${header_probe_count} - 1")

        foreach(header_probe_index RANGE 0 ${header_probe_last_index})
            list(GET CLANG_TIDY_HEADER_PROBE_SOURCES ${header_probe_index} header_probe_source)
            list(GET CLANG_TIDY_HEADER_SOURCE_FILES ${header_probe_index} header_source_file)
            list(GET CLANG_TIDY_HEADER_INL_FILES ${header_probe_index} header_inl_file)

            cmake_path(RELATIVE_PATH header_source_file
                       BASE_DIRECTORY "${CMAKE_SOURCE_DIR}"
                       OUTPUT_VARIABLE header_relative_path)

            string(MD5 header_id "header:${header_source_file}")
            set(check_stamp "${CLANG_TIDY_STAMP_DIR}/check_header_${header_id}.stamp")
            set(fix_stamp   "${CLANG_TIDY_STAMP_DIR}/fix_header_${header_id}.stamp")

            cobalt_clang_tidy_make_header_filter("${header_source_file}" header_filter)

            set(header_dependencies "${header_probe_source}" "${header_source_file}")
            if(NOT "${header_inl_file}" STREQUAL "${CLANG_TIDY_NO_INL}")
                cobalt_clang_tidy_make_header_filter("${header_inl_file}" inl_filter)
                string(APPEND header_filter "|${inl_filter}")
                list(APPEND header_dependencies "${header_inl_file}")
            endif()

            set(check_cmd "${CLANG_TIDY_EXE}")
            foreach(a IN LISTS CLANG_TIDY_COMMON_ARGS)
                list(APPEND check_cmd "${a}")
            endforeach()
            if(CLANG_TIDY_COMPILATION_DB_DIR)
                list(APPEND check_cmd -p "${CLANG_TIDY_COMPILATION_DB_DIR}")
            endif()
            list(APPEND check_cmd -format-style=none "--header-filter=${header_filter}" "${header_probe_source}")

            set(fix_cmd "${CLANG_TIDY_EXE}")
            foreach(a IN LISTS CLANG_TIDY_COMMON_ARGS)
                list(APPEND fix_cmd "${a}")
            endforeach()
            if(CLANG_TIDY_COMPILATION_DB_DIR)
                list(APPEND fix_cmd -p "${CLANG_TIDY_COMPILATION_DB_DIR}")
            endif()
            list(APPEND fix_cmd -format-style=none -fix -fix-errors "--header-filter=${header_filter}" "${header_probe_source}")

            add_custom_command(
                OUTPUT  "${check_stamp}"
                COMMAND ${check_cmd}
                COMMAND ${CMAKE_COMMAND} -E touch "${check_stamp}"
                DEPENDS ${header_dependencies} ${CLANG_TIDY_GENERATED_DEPENDENCIES}
                COMMENT "clang-tidy header: ${header_relative_path}"
                VERBATIM
            )

            add_custom_command(
                OUTPUT  "${fix_stamp}"
                COMMAND ${fix_cmd}
                COMMAND ${CMAKE_COMMAND} -E touch "${fix_stamp}"
                DEPENDS ${header_dependencies} ${CLANG_TIDY_GENERATED_DEPENDENCIES}
                COMMENT "clang-tidy header fix: ${header_relative_path}"
                VERBATIM
            )

            list(APPEND CLANG_TIDY_CHECK_OUTPUTS "${check_stamp}")
            list(APPEND CLANG_TIDY_FIX_OUTPUTS   "${fix_stamp}")
        endforeach()
    endif()

    # Umbrella targets: build all stamps
    if(COBALT_CLANG_TIDY_ON_ALL)
        add_custom_target(clang-tidy-check ALL
            DEPENDS ${CLANG_TIDY_CHECK_OUTPUTS}
            COMMENT "Running clang-tidy (check only) on selected target sources"
        )
    else()
        add_custom_target(clang-tidy-check
            DEPENDS ${CLANG_TIDY_CHECK_OUTPUTS}
            COMMENT "Running clang-tidy (check only) on selected target sources"
        )
    endif()
    set_target_properties(clang-tidy-check PROPERTIES FOLDER "CustomTargets")

    add_custom_target(clang-tidy-fix
        DEPENDS ${CLANG_TIDY_FIX_OUTPUTS}
        COMMENT "Running clang-tidy (apply fixes) on selected target sources"
    )
    set_target_properties(clang-tidy-fix PROPERTIES FOLDER "CustomTargets")
endfunction()
