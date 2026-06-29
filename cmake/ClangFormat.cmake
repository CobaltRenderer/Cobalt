include_guard(GLOBAL)

# Global list of targets that should be included in clang-format.
set_property(GLOBAL PROPERTY CLANG_FORMAT_TARGETS "")

# Global list of explicitly excluded files
set_property(GLOBAL PROPERTY CLANG_FORMAT_EXCLUDE_FILES "")

# Register a target whose sources should be formatted.
function(register_clang_format_target tgt)
    if(NOT TARGET ${tgt})
        message(FATAL_ERROR "register_clang_format_target(): Target '${tgt}' does not exist")
    endif()

    get_target_property(tgt_type ${tgt} TYPE)
    if(NOT tgt_type STREQUAL "EXECUTABLE"
       AND NOT tgt_type STREQUAL "STATIC_LIBRARY"
       AND NOT tgt_type STREQUAL "SHARED_LIBRARY"
       AND NOT tgt_type STREQUAL "MODULE_LIBRARY"
       AND NOT tgt_type STREQUAL "OBJECT_LIBRARY")
        message(FATAL_ERROR "register_clang_format_target(): '${tgt}' is not a compile target")
    endif()

    set_property(GLOBAL APPEND PROPERTY CLANG_FORMAT_TARGETS "${tgt}")
endfunction()

# Exclude specific files from clang-format.
function(exclude_clang_format_files)
    foreach(file IN LISTS ARGN)
        if(NOT IS_ABSOLUTE "${file}")
            get_filename_component(abs "${file}" ABSOLUTE BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
        else()
            set(abs "${file}")
        endif()

        set_property(GLOBAL APPEND PROPERTY CLANG_FORMAT_EXCLUDE_FILES "${abs}")
    endforeach()
endfunction()

# Finalise clang-format rules AFTER all targets/subdirectories are added.
function(cobalt_finalize_clang_format)
    # Find clang-format
    find_program(CLANG_FORMAT_EXE NAMES clang-format)
    if(NOT CLANG_FORMAT_EXE)
        message(STATUS "clang-format not found; clang-format targets will not be created")
        return()
    endif()

    # Collect targets that opted in
    get_property(all_targets GLOBAL PROPERTY CLANG_FORMAT_TARGETS)
    list(REMOVE_DUPLICATES all_targets)

    if(NOT all_targets)
        message(STATUS "No targets registered for clang-format")
        return()
    endif()

    # Which file extensions to format
    set(FORMAT_SUFFIXES .c .cc .cxx .cpp .h .hpp .hh .hxx .inl .pkg)
    set(ALL_FORMAT_SOURCES "")

    # Collect all sources from registered targets
    foreach(tgt IN LISTS all_targets)
        get_target_property(srcs "${tgt}" SOURCES)
        if(NOT srcs)
            continue()
        endif()

        # Directory where the target was defined (for resolving relative paths)
        get_target_property(tgt_source_dir "${tgt}" SOURCE_DIR)

        foreach(src IN LISTS srcs)
            # Skip generator expressions, etc.
            if(src MATCHES "^\\$<")
                continue()
            endif()

            get_filename_component(ext "${src}" EXT)
            list(FIND FORMAT_SUFFIXES "${ext}" idx)
            if(idx EQUAL -1)
                continue()
            endif()

            if(NOT IS_ABSOLUTE "${src}")
                get_filename_component(abs_src "${src}" ABSOLUTE BASE_DIR "${tgt_source_dir}")
            else()
                set(abs_src "${src}")
            endif()

            list(APPEND ALL_FORMAT_SOURCES "${abs_src}")
        endforeach()
    endforeach()

    list(REMOVE_DUPLICATES ALL_FORMAT_SOURCES)

    # Apply exclusions
    get_property(EXCLUDED_FILES GLOBAL PROPERTY CLANG_FORMAT_EXCLUDE_FILES)
    if(EXCLUDED_FILES)
        list(REMOVE_ITEM ALL_FORMAT_SOURCES ${EXCLUDED_FILES})
    endif()

    if(NOT ALL_FORMAT_SOURCES)
        message(STATUS "No source files selected for clang-format")
        return()
    endif()

    # Per-file stamp-based clang-format rules (incremental + parallel)
    set(CLANG_FORMAT_STAMP_DIR "${CMAKE_BINARY_DIR}/clang-format-stamps")
    file(MAKE_DIRECTORY "${CLANG_FORMAT_STAMP_DIR}")

    set(CLANG_FORMAT_CHECK_OUTPUTS "")
    set(CLANG_FORMAT_FIX_OUTPUTS "")

    foreach(src IN LISTS ALL_FORMAT_SOURCES)
        # Nice project-root-relative path for messages
        cmake_path(CONVERT "${src}" TO_CMAKE_PATH_LIST src_norm)
        cmake_path(RELATIVE_PATH src_norm
                   BASE_DIRECTORY "${CMAKE_SOURCE_DIR}"
                   OUTPUT_VARIABLE rel)

        # Stable ids for stamp filenames
        string(MD5 fmt_id "${src}")
        set(check_stamp "${CLANG_FORMAT_STAMP_DIR}/check_${fmt_id}.stamp")
        set(fix_stamp   "${CLANG_FORMAT_STAMP_DIR}/fix_${fmt_id}.stamp")

        # CHECK: run dry-run / Werror; only touch stamp on success
        add_custom_command(
            OUTPUT  "${check_stamp}"
#            COMMAND ${CMAKE_COMMAND} -E echo "[clang-format] check ${rel}"
            COMMAND "${CLANG_FORMAT_EXE}" --dry-run --Werror -style=file "${src}"
            COMMAND ${CMAKE_COMMAND} -E touch "${check_stamp}"
            DEPENDS "${src}" "${CMAKE_SOURCE_DIR}/.clang-format"
            COMMENT "clang-format check: ${rel}"
            VERBATIM
        )

        # FIX: apply formatting and mark stamp
        add_custom_command(
            OUTPUT  "${fix_stamp}"
#            COMMAND ${CMAKE_COMMAND} -E echo "[clang-format] fix   ${rel}"
            COMMAND "${CLANG_FORMAT_EXE}" -i -style=file "${src}"
            COMMAND ${CMAKE_COMMAND} -E touch "${fix_stamp}"
            DEPENDS "${src}" "${CMAKE_SOURCE_DIR}/.clang-format"
            COMMENT "clang-format fix: ${rel}"
            VERBATIM
        )

        list(APPEND CLANG_FORMAT_CHECK_OUTPUTS "${check_stamp}")
        list(APPEND CLANG_FORMAT_FIX_OUTPUTS   "${fix_stamp}")
    endforeach()

    if(COBALT_CLANG_FORMAT_ON_ALL)
        add_custom_target(clang-format-check ALL
            DEPENDS ${CLANG_FORMAT_CHECK_OUTPUTS}
            COMMENT "Running clang-format (check only) on selected sources"
        )
    else()
        add_custom_target(clang-format-check
            DEPENDS ${CLANG_FORMAT_CHECK_OUTPUTS}
            COMMENT "Running clang-format (check only) on selected sources"
        )
    endif()
    set_target_properties(clang-format-check PROPERTIES FOLDER "CustomTargets")

    add_custom_target(clang-format-fix
        DEPENDS ${CLANG_FORMAT_FIX_OUTPUTS}
        COMMENT "Running clang-format (apply fixes) on selected sources"
    )
    set_target_properties(clang-format-fix PROPERTIES FOLDER "CustomTargets")
endfunction()
