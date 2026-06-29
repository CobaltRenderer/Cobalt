# CMake/DocToolchain.cmake
#
# Sets up a Python 3 + xsltproc toolchain that can be used by your
# documentation builder script, in a mostly self-contained way.
#
# Usage from top-level CMakeLists.txt:
#
#   list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/CMake")
#   include(DocToolchain)
#
#   cobalt_setup_doc_toolchain(DOC_PYTHON DOC_XSLTPROC)
#
#   add_custom_target(docs
#       COMMAND "${DOC_PYTHON}" "${CMAKE_SOURCE_DIR}/Tools/build_docs.py"
#               --source-root "${CMAKE_SOURCE_DIR}/Documentation"
#               --output-root "${CMAKE_BINARY_DIR}/Output/SDK/Documentation"
#               --xslt-tool "${DOC_XSLTPROC}"
#       WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
#       COMMENT "Building HTML documentation"
#   )
#

cmake_minimum_required(VERSION 3.18)  # for cmake -E tar xvf on zip

include_guard(GLOBAL)

# Helper: download + unpack an archive (zip/tar.*) into a directory exactly once.
function(_cobalt_download_and_unpack url out_dir)
    set(download_dir "${CMAKE_BINARY_DIR}/_downloads")
    file(MAKE_DIRECTORY "${download_dir}")

    get_filename_component(archive_name "${url}" NAME)
    set(archive_path "${download_dir}/${archive_name}")

    if(NOT EXISTS "${archive_path}")
        message(STATUS "Downloading ${url} to ${archive_path}")
        file(DOWNLOAD
            "${url}" "${archive_path}"
            SHOW_PROGRESS
            STATUS dl_status
        )
        list(GET dl_status 0 dl_code)
        if(NOT dl_code EQUAL 0)
            list(GET dl_status 1 dl_msg)
            message(FATAL_ERROR "Download failed for ${url}: ${dl_msg}")
        endif()
    endif()

    # Only unpack once
    if(NOT EXISTS "${out_dir}")
        message(STATUS "Unpacking ${archive_path} to ${out_dir}")
        file(MAKE_DIRECTORY "${out_dir}")
        execute_process(
            COMMAND "${CMAKE_COMMAND}" -E tar xvf "${archive_path}"
            WORKING_DIRECTORY "${out_dir}"
            RESULT_VARIABLE tar_rv
        )
        if(NOT tar_rv EQUAL 0)
            message(FATAL_ERROR "Failed to unpack ${archive_path} (exit code ${tar_rv})")
        endif()
    endif()
endfunction()

# Main entry point: detect / download / configure Python + xsltproc
function(cobalt_setup_doc_toolchain out_python out_xsltproc)
    if(WIN32)
        # ------------------------------------------------------------------
        # Windows: download embeddable Python + xsltproc
        # ------------------------------------------------------------------
        # These will be set and then returned to the caller via PARENT_SCOPE.
        set(_DOC_PYTHON "")
        set(_DOC_XSLTPROC "")

        # Version can be adjusted as needed.
        set(COBALT_PYTHON_VERSION "3.12.0" CACHE STRING
            "Embeddable Python version to download on Windows")

        # 64-bit embeddable package URL
        set(COBALT_PYTHON_EMBED_URL
            "https://www.python.org/ftp/python/${COBALT_PYTHON_VERSION}/python-${COBALT_PYTHON_VERSION}-embed-amd64.zip"
            CACHE STRING
            "URL of the embeddable Python3 ZIP for Windows (64-bit)")

        set(python_root "${CMAKE_BINARY_DIR}/_toolchain/python-${COBALT_PYTHON_VERSION}-embed-amd64")
        _cobalt_download_and_unpack("${COBALT_PYTHON_EMBED_URL}" "${python_root}")

        # The embeddable ZIP unpacks python.exe and DLLs directly into that dir
        set(_DOC_PYTHON "${python_root}/python.exe")
        if(NOT EXISTS "${_DOC_PYTHON}")
            message(FATAL_ERROR "python.exe not found in ${python_root} (check COBALT_PYTHON_EMBED_URL)")
        endif()

        # xsltproc: you need a bundle that contains xsltproc.exe and its DLLs.
        # Default here uses a small GitHub repo that packages Igor Zlatkovic's
        # binaries. You may swap this URL for your own hosted zip.
        set(COBALT_XSLTPROC_URL
            "https://github.com/mdecourse/xsltproc-win/raw/master/xsltproc-win.zip"
            CACHE STRING
            "URL of a ZIP containing xsltproc.exe and required DLLs for Windows")

        set(xslt_root "${CMAKE_BINARY_DIR}/_toolchain/xsltproc")
        _cobalt_download_and_unpack("${COBALT_XSLTPROC_URL}" "${xslt_root}")

        # The xsltproc-win.zip archive (from that repo) unpacks xsltproc.exe into the root
        set(_DOC_XSLTPROC "${xslt_root}/xsltproc/xsltproc.exe")
        if(NOT EXISTS "${_DOC_XSLTPROC}")
            message(FATAL_ERROR
                "xsltproc.exe not found in ${xslt_root}. "
                "Adjust COBALT_XSLTPROC_URL to point to a ZIP that contains xsltproc.exe.")
        endif()

    else()  # UNIX-like: Linux/macOS
        # ------------------------------------------------------------------
        # UNIX: rely on system python3 + xsltproc (much more robust than
        # trying to download random prebuilt binaries for every distro).
        # ------------------------------------------------------------------
        find_program(_DOC_PYTHON NAMES python3 python)
        if(NOT _DOC_PYTHON)
            message(WARNING
                "python3 not found on this system. Documentation generation will not be available.")
            set(_DOC_PYTHON "")
            set(_DOC_XSLTPROC "")
            return()
        endif()

        find_program(_DOC_XSLTPROC NAMES xsltproc)
        if(NOT _DOC_XSLTPROC)
            message(WARNING
                "xsltproc not found on this system. Documentation generation will not be available.")
            set(_DOC_PYTHON "")
            set(_DOC_XSLTPROC "")
            return()
        endif()
    endif()

    # Return to caller
    set(${out_python}  "${_DOC_PYTHON}"    PARENT_SCOPE)
    set(${out_xsltproc} "${_DOC_XSLTPROC}" PARENT_SCOPE)

    message(STATUS "Doc toolchain: Python  = ${_DOC_PYTHON}")
    message(STATUS "Doc toolchain: xsltproc = ${_DOC_XSLTPROC}")
endfunction()
