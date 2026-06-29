# These helper functions assist with additional file copying required during the compile and install phases

include_guard(GLOBAL)

# Adds an RPATH entry for a target, relative to the image directory:
#   add_rpath(MyTarget ".")          # directory containing the binary
#   add_rpath(MyTarget "Renderers")  # subdirectory of the image dir
#   add_rpath(MyTarget "../lib")     # parent-relative path
#
# Platform behaviour:
#   - Linux:  uses $ORIGIN
#   - macOS:  uses @loader_path
#   - Windows: no-op
function(add_rpath target rel_path)
    # No RPATH concept on Windows in the same way; do nothing.
    if(WIN32)
        return()
    endif()

    # Choose platform-specific "origin" token.
    if(APPLE)
        set(_base "@loader_path")
    else()
        # Escape $ so CMake doesn't interpret it.
        set(_base "\$ORIGIN")
    endif()

    # Compute the final RPATH entry.
    set(_entry "")

    # If empty or ".", use the image directory itself.
    if(rel_path STREQUAL "" OR rel_path STREQUAL ".")
        set(_entry "${_base}")
    else()
        # If it's absolute, just use it as-is (user really wants an absolute RPATH).
        if(IS_ABSOLUTE "${rel_path}")
            set(_entry "${rel_path}")
        else()
            # Strip a leading "./" if present.
            set(_rel "${rel_path}")
            string(REGEX REPLACE "^\\./" "" _rel "${_rel}")

            if(_rel STREQUAL "")
                set(_entry "${_base}")
            else()
                # Join base and relative path.
                set(_entry "${_base}/${_rel}")
            endif()
        endif()
    endif()

    # Append _entry to BUILD_RPATH and INSTALL_RPATH if not already present.
    foreach(_prop IN ITEMS BUILD_RPATH INSTALL_RPATH)
        get_target_property(_old_rpath "${target}" "${_prop}")
        if(NOT _old_rpath OR _old_rpath STREQUAL "NOTFOUND")
            set(_old_rpath "")
        endif()

        string(FIND "${_old_rpath}" "${_entry}" _idx)
        if(_idx EQUAL -1)
            if(_old_rpath STREQUAL "")
                set(_new_rpath "${_entry}")
            else()
                set(_new_rpath "${_old_rpath};${_entry}")
            endif()
            set_target_properties("${target}" PROPERTIES "${_prop}" "${_new_rpath}")
        endif()
    endforeach()
endfunction()



# Splits debug information from a built target into a separate file at install time.
#
#   cobalt_split_debug_info(MyTarget "bin")
#   cobalt_split_debug_info(MyLib    "lib")
#   cobalt_split_debug_info(MyTool   "${CMAKE_INSTALL_BINDIR}")  # absolute or relative
#
# Behaviour:
#   - Operates only on valid CMake targets (errors if the target does not exist).
#   - Runs during `install`, not at build time.
#   - Only active for the "RelWithDebInfo" configuration.
#
# Platform behaviour:
#   - Linux:
#       * Uses objcopy to extract debug info into "<binary>.debug"
#       * Strips debug symbols from the installed binary
#       * Adds a GNU debuglink so debuggers can find the .debug file
#
#   - macOS:
#       * Uses dsymutil to generate a "<binary>.dSYM" bundle
#       * Strips debug symbols from the installed binary
#
#   - Windows:
#       * No-op (debug info handled via PDB files separately)
#
# Install directory handling:
#   - If `install_dir` is absolute, it is used as-is.
#   - Otherwise it is treated as relative to `${COBALT_SDK_ROOT}`.
#   - The function assumes the target has already been installed/copied
#     into this directory via `install(TARGETS ...)`.
#
# Notes:
#   - This function should be called after installing a target to ensure
#     the binary exists at the expected location.
#   - Intended for executables and shared/module libraries.
#
function(cobalt_split_debug_info target install_dir)
  if(NOT TARGET "${target}")
    message(FATAL_ERROR "cobalt_split_debug_info: Target '${target}' does not exist")
  endif()

  if(WIN32)
    return()
  endif()

  # Construct full destination under SDK root (or use absolute as-is)
  if(IS_ABSOLUTE "${install_dir}")
    set(install_dir "${install_dir}")
  else()
    set(install_dir "${COBALT_SDK_ROOT}/${install_dir}")
  endif()

  if(APPLE)
    find_program(DSYMUTIL_EXECUTABLE dsymutil REQUIRED)
    find_program(STRIP_EXECUTABLE strip REQUIRED)

    install(CODE "
      if(CMAKE_INSTALL_CONFIG_NAME STREQUAL \"RelWithDebInfo\")
        set(binary \"${install_dir}/$<TARGET_FILE_NAME:${target}>\")
        message(STATUS \"Splitting out debug info for binary: \${binary}\")

        execute_process(
          COMMAND \"${DSYMUTIL_EXECUTABLE}\" \"\${binary}\" -o \"\${binary}.dSYM\"
          RESULT_VARIABLE rc
        )
        if(NOT rc EQUAL 0)
          message(FATAL_ERROR \"dsymutil failed for \${binary}\")
        endif()

        execute_process(
          COMMAND \"${STRIP_EXECUTABLE}\" -S \"\${binary}\"
          RESULT_VARIABLE rc
        )
        if(NOT rc EQUAL 0)
          message(FATAL_ERROR \"strip failed for \${binary}\")
        endif()
      endif()
    ")

  elseif(UNIX)
    find_program(OBJCOPY_EXECUTABLE NAMES objcopy llvm-objcopy REQUIRED)
    find_program(STRIP_EXECUTABLE strip REQUIRED)

    install(CODE "
      if(CMAKE_INSTALL_CONFIG_NAME STREQUAL \"RelWithDebInfo\")
        set(binary \"${install_dir}/$<TARGET_FILE_NAME:${target}>\")
        set(debug_file \"\${binary}.debug\")

        message(STATUS \"Splitting out debug info for binary: \${binary}\")

        execute_process(
          COMMAND \"${OBJCOPY_EXECUTABLE}\" --only-keep-debug \"\${binary}\" \"\${debug_file}\"
          RESULT_VARIABLE rc
        )
        if(NOT rc EQUAL 0)
          message(FATAL_ERROR \"objcopy --only-keep-debug failed for \${binary}\")
        endif()

        execute_process(
          COMMAND \"${STRIP_EXECUTABLE}\" --strip-debug \"\${binary}\"
          RESULT_VARIABLE rc
        )
        if(NOT rc EQUAL 0)
          message(FATAL_ERROR \"strip failed for \${binary}\")
        endif()

        get_filename_component(binary_dir \"\${binary}\" DIRECTORY)

        execute_process(
          COMMAND \"${OBJCOPY_EXECUTABLE}\" \"--add-gnu-debuglink=$<TARGET_FILE_NAME:${target}>.debug\" \"\${binary}\"
          WORKING_DIRECTORY \"\${binary_dir}\"
          RESULT_VARIABLE rc
        )
        if(NOT rc EQUAL 0)
          message(FATAL_ERROR \"objcopy --add-gnu-debuglink failed for \${binary}\")
        endif()
      endif()
    ")
  endif()
endfunction()


# Usage:
#   cobalt_configure_sdk_install(MyTarget)          # installs exe/dll + lib + pdb
#   cobalt_configure_sdk_install(MyTarget OMIT_LIB) # installs exe/dll + pdb, no lib
function(cobalt_configure_sdk_install target)
    set(options OMIT_LIB)
    cmake_parse_arguments(COBALT_INSTALL "${options}" "" "" ${ARGN})

    # Determine the target type for PDB installation (only linker-produced binaries)
    get_target_property(_cobalt_target_type "${target}" TYPE)

    if(COBALT_INSTALL_OMIT_LIB)
        # No ARCHIVE, so no .lib/.a
        install(TARGETS ${target}
            RUNTIME DESTINATION "${COBALT_SDK_BIN_DIR}"  # .exe / .dll
            LIBRARY DESTINATION "${COBALT_SDK_BIN_DIR}"  # .so / .dylib
        )
    else()
        # Include ARCHIVE - static/import libs go to Lib/x64
        install(TARGETS ${target}
            RUNTIME DESTINATION "${COBALT_SDK_BIN_DIR}"
            LIBRARY DESTINATION "${COBALT_SDK_BIN_DIR}"
            ARCHIVE DESTINATION "${COBALT_SDK_LIB_DIR}"
        )
    endif()
    if(_cobalt_target_type STREQUAL "EXECUTABLE"
        OR _cobalt_target_type STREQUAL "SHARED_LIBRARY"
        OR _cobalt_target_type STREQUAL "MODULE_LIBRARY")
        cobalt_split_debug_info("${target}" "${COBALT_SDK_BIN_DIR}")
    endif()

    # PDBs (MSVC): install next to the binaries.
    # Only valid for targets that produce linker-created artifacts
    # (executables, shared/libs, module libs). Skip static or header-only libraries.
    if(WIN32)
        if(_cobalt_target_type STREQUAL "EXECUTABLE"
           OR _cobalt_target_type STREQUAL "SHARED_LIBRARY"
           OR _cobalt_target_type STREQUAL "MODULE_LIBRARY")
            install(FILES
                "$<TARGET_PDB_FILE:${target}>"
                DESTINATION "${COBALT_SDK_BIN_DIR}"
                OPTIONAL
            )
        endif()
    endif()
endfunction()



# Return whether a target is a non-imported shared-library output we should stage.
function(_cobalt_target_is_copyable_shared_library target output_var)
    set(_is_copyable_shared_library FALSE)

    if(TARGET "${target}")
        get_target_property(_target_type "${target}" TYPE)
        get_target_property(_target_imported "${target}" IMPORTED)

        if(NOT _target_imported
           AND (_target_type STREQUAL "SHARED_LIBRARY" OR _target_type STREQUAL "MODULE_LIBRARY"))
            set(_is_copyable_shared_library TRUE)
        endif()
    endif()

    set(${output_var} "${_is_copyable_shared_library}" PARENT_SCOPE)
endfunction()


# Recursively collect shared-library targets linked by the given target.
function(_cobalt_collect_transitive_shared_library_targets target out_var)
    set(visited_targets ${ARGN})

    if(NOT TARGET "${target}")
        set(${out_var} "" PARENT_SCOPE)
        return()
    endif()

    list(FIND visited_targets "${target}" _visited_target_index)
    if(NOT _visited_target_index EQUAL -1)
        set(${out_var} "" PARENT_SCOPE)
        return()
    endif()
    list(APPEND visited_targets "${target}")

    set(collected_targets "")

    get_target_property(_linked_targets "${target}" LINK_LIBRARIES)

    foreach(dep IN LISTS _linked_targets)
        # Skip link keywords and generator expressions.
        if(dep STREQUAL "optimized" OR dep STREQUAL "debug" OR dep STREQUAL "general")
            continue()
        endif()
        if(dep MATCHES "^\\$<")
            continue()
        endif()

        if(TARGET "${dep}")
            get_target_property(_aliased_target "${dep}" ALIASED_TARGET)
            if(_aliased_target)
                set(_real_dep "${_aliased_target}")
            else()
                set(_real_dep "${dep}")
            endif()

            if(TARGET "${_real_dep}")
                get_target_property(_real_dep_type "${_real_dep}" TYPE)
                if(_real_dep_type STREQUAL "INTERFACE_LIBRARY")
                    continue()
                endif()

                _cobalt_target_is_copyable_shared_library("${_real_dep}" _is_copyable_shared_library)
                if(_is_copyable_shared_library)
                    list(APPEND collected_targets "${_real_dep}")
                endif()

                _cobalt_collect_transitive_shared_library_targets("${_real_dep}" _child_collected_targets ${visited_targets})
                list(APPEND collected_targets ${_child_collected_targets})
            endif()
        endif()
    endforeach()

    if(collected_targets)
        list(REMOVE_DUPLICATES collected_targets)
    endif()

    set(${out_var} "${collected_targets}" PARENT_SCOPE)
endfunction()


# Usage:
#   cobalt_install_with_deps(MyTarget "Bin/x64")
# Installs to:
#   ${COBALT_SDK_ROOT}/Bin/x64
#
function(cobalt_install_with_deps target rel_dest)
    if(NOT COBALT_SDK_ROOT)
        message(FATAL_ERROR "COBALT_SDK_ROOT must be set before calling cobalt_install_with_deps()")
    endif()

    if(NOT TARGET "${target}")
        message(FATAL_ERROR "cobalt_install_with_deps(): Target '${target}' does not exist")
    endif()

    # Determine target type (EXECUTABLE, SHARED_LIBRARY, INTERFACE_LIBRARY, etc.)
    get_target_property(_target_type "${target}" TYPE)

    # Header-only / non-binary targets have nothing to install; bail out early
    if(_target_type STREQUAL "INTERFACE_LIBRARY"
       OR _target_type STREQUAL "OBJECT_LIBRARY"
       OR _target_type STREQUAL "UTILITY")
        # Optional: message(VERBOSE "cobalt_install_with_deps(): '${target}' is ${_target_type}, nothing to install")
        return()
    endif()

    # Construct full destination under SDK root (or use absolute as-is)
    if(IS_ABSOLUTE "${rel_dest}")
        set(dest_dir "${rel_dest}")
    else()
        set(dest_dir "${COBALT_SDK_ROOT}/${rel_dest}")
    endif()

    # 1) Install the target itself (exe/dll/so/dylib), but no .lib/.a
    install(TARGETS ${target}
        RUNTIME DESTINATION "${dest_dir}"   # .exe / .dll (and .so on some platforms)
        LIBRARY DESTINATION "${dest_dir}"   # .so / .dylib
        # no ARCHIVE, so no .lib/.a
    )
    cobalt_split_debug_info("${target}" "${dest_dir}")

    # 2) Install the target's own PDB (if present)
    # Only valid for linker-produced binaries (not static libraries).
    if(WIN32)
        if(_target_type STREQUAL "EXECUTABLE"
           OR _target_type STREQUAL "SHARED_LIBRARY"
           OR _target_type STREQUAL "MODULE_LIBRARY")
            install(FILES
                "$<TARGET_PDB_FILE:${target}>"
                DESTINATION "${dest_dir}"
                OPTIONAL
            )
        endif()
    endif()

    # 3) Windows-only: runtime DLL dependencies + their PDBs if they are CMake targets
    if(WIN32)
        # 3a) Install all runtime DLLs this target depends on (CMake 3.21+)
        install(FILES
            $<TARGET_RUNTIME_DLLS:${target}>
            DESTINATION "${dest_dir}"
            OPTIONAL
        )

        # 3b) Try to install PDBs for linked CMake targets (static/imported/SHARED)
        get_target_property(_linked ${target} LINK_LIBRARIES)

        if(_linked)
            foreach(dep IN LISTS _linked)
                # Skip generator expressions and link keywords
                if(dep STREQUAL "optimized" OR dep STREQUAL "debug" OR dep STREQUAL "general")
                    continue()
                endif()
                if(dep MATCHES "^\\$<")
                    continue()
                endif()

                if(TARGET ${dep})
                    # Skip header-only / non-binary deps and static libraries (no linker PDB)
                    get_target_property(_dep_type "${dep}" TYPE)
                    if(_dep_type STREQUAL "INTERFACE_LIBRARY"
                       OR _dep_type STREQUAL "OBJECT_LIBRARY"
                       OR _dep_type STREQUAL "UTILITY"
                       OR _dep_type STREQUAL "STATIC_LIBRARY")
                        continue()
                    endif()

                    install(FILES
                        "$<TARGET_PDB_FILE:${dep}>"
                        DESTINATION "${dest_dir}"
                        OPTIONAL
                    )
                    cobalt_split_debug_info("${dep}" "${dest_dir}")
                endif()
            endforeach()
        endif()

    # 4) Linux/macOS: install transitive shared-library deps (self-contained distribution)
    else()
        _cobalt_collect_transitive_shared_library_targets("${target}" _cobalt_shared_library_targets)

        if(_cobalt_shared_library_targets)
            foreach(dep IN LISTS _cobalt_shared_library_targets)
                # Avoid re-installing the primary target as a dep of itself.
                if("${dep}" STREQUAL "${target}")
                    continue()
                endif()

                install(TARGETS ${dep}
                    RUNTIME DESTINATION "${dest_dir}"   # for macOS MODULE or helper tools
                    LIBRARY DESTINATION "${dest_dir}"   # .so / .dylib
                )
                cobalt_split_debug_info("${dep}" "${dest_dir}")
            endforeach()
        endif()
    endif()
endfunction()



function(cobalt_stage_deps target dest)
    if(NOT TARGET "${target}")
        message(FATAL_ERROR "cobalt_stage_deps(): Target '${target}' does not exist")
    endif()

    # Determine the target type (EXECUTABLE, STATIC_LIBRARY, SHARED_LIBRARY, INTERFACE_LIBRARY, UTILITY, etc.)
    get_target_property(_target_type "${target}" TYPE)

    # If this is a header-only / non-binary target, there is nothing to stage -> early out
    if(_target_type STREQUAL "INTERFACE_LIBRARY"
       OR _target_type STREQUAL "UTILITY"
       OR _target_type STREQUAL "OBJECT_LIBRARY")
        # Optional: be chatty if you like
        # message(VERBOSE "cobalt_stage_deps(): '${target}' is ${_target_type}, nothing to stage")
        return()
    endif()

    #
    # Resolve destination:
    # - If ABSOLUTE:     keep as-is
    # - If RELATIVE:     interpret relative to the target's output dir
    #
    if(IS_ABSOLUTE "${dest}")
        set(full_dest "${dest}")
    else()
        # remove accidental leading slashes
        string(REGEX REPLACE "^[/\\]+" "" dest_no_leading "${dest}")

        # Build-time expression: <target-dir>/<dest>
        set(full_dest "$<TARGET_FILE_DIR:${target}>/${dest_no_leading}")
    endif()


    #
    # 1) Copy the target's own binary
    #
    add_custom_command(TARGET "${target}" POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${full_dest}"
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            "$<TARGET_FILE:${target}>" "${full_dest}"
    )


    #
    # 2) Copy the target's PDB (if any)
    #
    # Only valid for linker-produced binaries (not static libraries).
    if(WIN32)
        if(_target_type STREQUAL "EXECUTABLE"
           OR _target_type STREQUAL "SHARED_LIBRARY"
           OR _target_type STREQUAL "MODULE_LIBRARY")
            add_custom_command(TARGET "${target}" POST_BUILD
                COMMAND "${CMAKE_COMMAND}" -E make_directory "${full_dest}"
                COMMAND "${CMAKE_COMMAND}" -E copy_if_different
                    "$<TARGET_PDB_FILE:${target}>" "${full_dest}"
                VERBATIM
            )
        endif()
    endif()


    #
    # 3) Windows-only: copy runtime DLL dependencies
    #
    if(WIN32)
        # $<TARGET_RUNTIME_DLLS:...> expands into all dependent DLL paths
        add_custom_command(TARGET "${target}" POST_BUILD
            COMMAND "${CMAKE_COMMAND}" -E make_directory "${full_dest}"
            COMMAND "${CMAKE_COMMAND}" -E copy_if_different
                $<TARGET_RUNTIME_DLLS:${target}> "${full_dest}"
            COMMAND_EXPAND_LISTS
        )
    endif()


    #
    # 4) Try to copy PDBs for linked CMake targets (but skip header-only / non-binary deps)
    #
    if(WIN32)
        get_target_property(_linked "${target}" LINK_LIBRARIES)

        if(_linked)
            foreach(dep IN LISTS _linked)
                # Skip link keywords and generator expressions
                if(dep STREQUAL "optimized" OR dep STREQUAL "debug" OR dep STREQUAL "general")
                    continue()
                endif()
                if(dep MATCHES "^\\$<")
                    continue()
                endif()
     
                if(TARGET "${dep}")
                    # Check the dependency's type; only binaries can have PDBs
                    get_target_property(_dep_type "${dep}" TYPE)
     
                    if(_dep_type STREQUAL "INTERFACE_LIBRARY"
                       OR _dep_type STREQUAL "UTILITY"
                       OR _dep_type STREQUAL "OBJECT_LIBRARY"
                       OR _dep_type STREQUAL "STATIC_LIBRARY")
                        # header-only / non-binary dep - nothing to copy
                        continue()
                    endif()
                    
                    add_custom_command(TARGET "${target}" POST_BUILD
                        COMMAND "${CMAKE_COMMAND}" -E make_directory "${full_dest}"
                        COMMAND "${CMAKE_COMMAND}" -E copy_if_different
                            "$<TARGET_PDB_FILE:${dep}>" "${full_dest}"
                    )
                endif()
            endforeach()
        endif()
    endif()

endfunction()


# Usage:
#   cobalt_install_sources("Include/Logging" ${Logging_HEADERS})
#   cobalt_install_sources("Data/Shaders" ${MyShaderFiles})
#
function(cobalt_install_sources rel_dest)
    if(NOT COBALT_SDK_ROOT)
        message(FATAL_ERROR "COBALT_SDK_ROOT must be set before calling cobalt_install_sources()")
    endif()

    # All arguments after rel_dest are the files we want to install
    set(files ${ARGN})

    if(NOT files)
        # Nothing to install, just exit quietly
        return()
    endif()

    # Destination under the SDK root
    set(dest_dir "${COBALT_SDK_ROOT}/${rel_dest}")

    install(
        FILES ${files}
        DESTINATION "${dest_dir}"
    )
endfunction()

# Usage:
#   cobalt_install_generated_bin("SomeDir/"      "Utilities/AutomatedTests/Renderers")
#   cobalt_install_generated_bin("SomeDir/file"  "Utilities/AutomatedTests/Renderers")
#   cobalt_install_generated_bin("SomeDir/*"     "Utilities/AutomatedTests/Renderers")
#
# All src_spec values are treated as relative to ${CMAKE_BINARY_DIR} unless absolute.
# Destination is always under ${COBALT_SDK_ROOT}.
#
function(cobalt_install_generated_bin src_spec rel_dest)
    if(NOT COBALT_SDK_ROOT)
        message(FATAL_ERROR "COBALT_SDK_ROOT must be set before calling cobalt_install_generated_bin()")
    endif()

    if(IS_ABSOLUTE "${rel_dest}")
        set(dest_dir "${rel_dest}")
    else()
        set(dest_dir "${COBALT_SDK_ROOT}/${rel_dest}")
    endif()

    # Does it contain a wildcard?
    set(has_wildcard FALSE)
    if(src_spec MATCHES ".*[\\*\\?].*")
        set(has_wildcard TRUE)
    endif()

    # Does it end with a slash?
    set(ends_with_slash FALSE)
    if(src_spec MATCHES ".+/$")
        set(ends_with_slash TRUE)
    endif()

    # --- Case 1: wildcard pattern (e.g. "SomeDir/*.dll") ---
    if(has_wildcard)
        # Split into directory + pattern
        # e.g. "SomeDir/*.dll" -> base="SomeDir", pattern="*.dll"
        string(REGEX MATCH "^(.*)/(.*)$" _match "${src_spec}")
        if(_match)
            set(base_rel "${CMAKE_MATCH_1}")
            set(pattern  "${CMAKE_MATCH_2}")
        else()
            # No slash, e.g. "*.dll" -> treat as pattern in CMAKE_BINARY_DIR
            set(base_rel ".")
            set(pattern  "${src_spec}")
        endif()

        # Make base dir absolute
        if(IS_ABSOLUTE "${base_rel}")
            set(src_dir_full "${base_rel}")
        else()
            set(src_dir_full "${CMAKE_BINARY_DIR}/${base_rel}")
        endif()

        install(
            DIRECTORY "${src_dir_full}/"
            DESTINATION "${dest_dir}"
            FILES_MATCHING
                PATTERN "${pattern}"
        )

    # --- Case 2: ends with "/" - all files under that directory (non-recursive) ---
    elseif(ends_with_slash)
        # Strip the trailing slash for the actual path
        string(REGEX REPLACE "/$" "" base_rel "${src_spec}")

        if(IS_ABSOLUTE "${base_rel}")
            set(src_dir_full "${base_rel}")
        else()
            set(src_dir_full "${CMAKE_BINARY_DIR}/${base_rel}")
        endif()

        install(
            DIRECTORY "${src_dir_full}/"
            DESTINATION "${dest_dir}"
            FILES_MATCHING
                PATTERN "*"
                PATTERN "*/" EXCLUDE    # exclude subdirectories - only top-level files
        )

    # --- Case 3: single file ---
    else()
        if(src_spec MATCHES "\\$<[^>]+>")
            set(file_full "${src_spec}")
        elseif(IS_ABSOLUTE "${src_spec}")
            set(file_full "${src_spec}")
        else()
            set(file_full "${CMAKE_BINARY_DIR}/${src_spec}")
        endif()

        install(
            FILES "${file_full}"
            DESTINATION "${dest_dir}"
        )
    endif()
endfunction()


function(cobalt_install_sdk_headers target)
    # ARGN contains all arguments after <target>
    set(headers ${ARGN})

    if(headers)
        install(
            FILES ${headers}
            DESTINATION "${COBALT_SDK_INCLUDE_DIR}/${target}"
        )
    endif()
endfunction()

function(cobalt_stage_generated_bin target src_spec rel_dest)
    if(NOT COBALT_SDK_ROOT)
        message(FATAL_ERROR
            "COBALT_SDK_ROOT must be set before calling cobalt_stage_generated_bin()")
    endif()

    if(NOT TARGET "${target}")
        message(FATAL_ERROR
            "cobalt_stage_generated_bin(): target '${target}' does not exist")
    endif()

    # Resolve destination directory
    if(IS_ABSOLUTE "${rel_dest}")
        set(dest_dir "${rel_dest}")
    else()
        set(dest_dir "$<TARGET_FILE_DIR:${target}>/${rel_dest}")
    endif()

    # Detect wildcard
    set(has_wildcard FALSE)
    if(src_spec MATCHES ".*[\\*\\?].*")
        set(has_wildcard TRUE)
    endif()

    # Detect trailing slash
    set(ends_with_slash FALSE)
    if(src_spec MATCHES ".+/$")
        set(ends_with_slash TRUE)
    endif()

    # ------------------------------------------------------------------------------
    #  Recursive glob & copy script (preserves directory structure)
    # ------------------------------------------------------------------------------
    set(_script_recursive "${CMAKE_CURRENT_BINARY_DIR}/cobalt_stage_glob_recursive.cmake")
    if(NOT EXISTS "${_script_recursive}")
        file(WRITE "${_script_recursive}" "
if(NOT DEFINED src_dir_full)
    message(FATAL_ERROR \"src_dir_full not set\")
endif()
if(NOT DEFINED dest_dir)
    message(FATAL_ERROR \"dest_dir not set\")
endif()
if(NOT DEFINED pattern)
    set(pattern \"*\")
endif()

file(MAKE_DIRECTORY \"\${dest_dir}\")

file(GLOB_RECURSE _files LIST_DIRECTORIES FALSE \"\${src_dir_full}/\${pattern}\")

foreach(f IN LISTS _files)
    file(RELATIVE_PATH _rel \"\${src_dir_full}\" \"\${f}\")
    get_filename_component(_rel_dir \"\${_rel}\" DIRECTORY)

    set(_dest \"\${dest_dir}\")
    if(NOT _rel_dir STREQUAL \".\" AND NOT _rel_dir STREQUAL \"\")
        set(_dest \"\${dest_dir}/\${_rel_dir}\")
    endif()

    file(MAKE_DIRECTORY \"\${_dest}\")
    file(COPY \"\${f}\" DESTINATION \"\${_dest}\")
endforeach()
")
    endif()

    # ------------------------------------------------------------------------------
    #  Determine source directory + pattern
    # ------------------------------------------------------------------------------

    if(has_wildcard)
        # Pattern case: SomeDir/*.dll OR *.dll
        string(REGEX MATCH "^(.*)/(.*)$" _match "${src_spec}")
        if(_match)
            set(base_rel "${CMAKE_MATCH_1}")
            set(pattern  "${CMAKE_MATCH_2}")
        else()
            set(base_rel ".")
            set(pattern "${src_spec}")
        endif()

    elseif(ends_with_slash)
        # Directory case: SomeDir/
        string(REGEX REPLACE "/$" "" base_rel "${src_spec}")
        set(pattern "*")

    else()
        # Single file case — non-recursive
        if(src_spec MATCHES "\\$<[^>]+>")
            set(file_full "${src_spec}")
        elseif(IS_ABSOLUTE "${src_spec}")
            set(file_full "${src_spec}")
        else()
            set(file_full "${CMAKE_BINARY_DIR}/${src_spec}")
        endif()

        add_custom_command(TARGET "${target}" POST_BUILD
            COMMAND "${CMAKE_COMMAND}" -E make_directory "${dest_dir}"
            COMMAND "${CMAKE_COMMAND}" -E copy_if_different
                    "${file_full}" "${dest_dir}"
#            COMMENT "Staging file ${file_full} - ${dest_dir}"
        )
        return()
    endif()

    # ------------------------------------------------------------------------------
    #  Resolve source directory for wildcard or directory case
    # ------------------------------------------------------------------------------

    if(IS_ABSOLUTE "${base_rel}")
        set(src_dir_full "${base_rel}")
    else()
        set(src_dir_full "${CMAKE_BINARY_DIR}/${base_rel}")
    endif()

    # ------------------------------------------------------------------------------
    #  Attach recursive copy command to target
    # ------------------------------------------------------------------------------

    add_custom_command(TARGET "${target}" POST_BUILD
        COMMAND "${CMAKE_COMMAND}"
                "-Dsrc_dir_full=${src_dir_full}"
                "-Ddest_dir=${dest_dir}"
                "-Dpattern=${pattern}"
                -P "${_script_recursive}"
#        COMMENT "Staging recursively from ${src_dir_full} (pattern '${pattern}') - ${dest_dir}"
    )
endfunction()

# make_paths_relative(<out-var> <base-dir> <list-of-absolute-paths>)
#
# Example:
#   make_paths_relative(REL_LIST "${CMAKE_SOURCE_DIR}" "${ABS_LIST}")
#   message("Relative: ${REL_LIST}")
#
function(make_paths_relative out_var base_dir)
    set(_result)

    foreach(_path IN LISTS ARGN)
        # Compute relative path
        file(RELATIVE_PATH _rel "${base_dir}" "${_path}")

        # Append to result list
        list(APPEND _result "${_rel}")
    endforeach()

    # Return to caller
    set(${out_var} "${_result}" PARENT_SCOPE)
endfunction()
