include_guard(GLOBAL)

# Clang (and clang-tidy) compiler warning settings
set(COBALT_CLANG_WARNING_FLAGS
    -Wall
    -Wno-unknown-warning-option
    -Werror
    -Wno-c++98-compat
    -Wno-c++98-compat-pedantic
    -Wno-cast-function-type
    -Wno-covered-switch-default
    -Wno-ctad-maybe-unsupported
    -Wno-declaration-after-statement
    -Wno-delete-non-virtual-dtor
    -Wno-disabled-macro-expansion
    -Wno-documentation-unknown-command
    -Wno-double-promotion
    -Wno-exit-time-destructors
    -Wno-extra-semi
    -Wno-float-equal
    -Wno-global-constructors
    -Wno-implicit-fallthrough
    -Wno-missing-noreturn
    -Wno-non-virtual-dtor
    -Wno-old-style-cast
    -Wno-padded
    -Wno-reorder-ctor
    -Wno-reserved-identifier
    -Wno-reserved-macro-identifier
    -Wno-shadow
    -Wno-sign-conversion
    -Wno-suggest-override
    -Wno-switch
    -Wno-switch-default
    -Wno-switch-enum
    -Wno-unneeded-internal-declaration
    -Wno-unsafe-buffer-usage
    -Wno-unused-command-line-argument
    -Wno-unused-macros
    -Wno-unused-parameter
    -Wno-unused-variable
    -Wno-unused-private-field
    -Wno-zero-as-null-pointer-constant
)
if (WIN32)
    list(APPEND COBALT_CLANG_WARNING_FLAGS -Wno-language-extension-token)
endif()

# MSVC compiler warning settings
set(COBALT_MSVC_WARNING_FLAGS
    /Wall
    /WX
    /Wv:19.50.35718
    /wd4061
    /wd4062
    /wd4100
    /wd4127
    /wd4191
    /wd4250
    /wd4255
    /wd4265
    /wd4302
    /wd4324
    /wd4339
    /wd4342
    /wd4347
    /wd4350
    /wd4365
    /wd4371
    /wd4412
    /wd4428
    /wd4435
    /wd4464
    /wd4471
    /wd4514
    /wd4571
    /wd4574
    /wd4610
    /wd4619
    /wd4623
    /wd4625
    /wd4626
    /wd4628
    /wd4640
    /wd4641
    /wd4668
    /wd4682
    /wd4692
    /wd4710
    /wd4711
    /wd4738
    /wd4746
    /wd4774
    /wd4777
    /wd4820
    /wd4865
    /wd4868
    /wd4917
    /wd4946
    /wd4987
    /wd5026
    /wd5027
    /wd5038
    /wd5039
    /wd5045
    /wd5204
    /wd5220
    /wd5246
)

# GCC compiler warning settings (some may not be supported)
set(COBALT_GCC_ALL_WARNING_FLAGS
    -Wall
    -Wextra
    -Werror
    -Wno-delete-non-virtual-dtor
    -Wno-cast-function-type
    -Wno-switch-default
    -Wno-double-promotion
    -Wno-extra-semi
    -Wno-float-equal
    -Wno-implicit-fallthrough
    -Wno-non-virtual-dtor
    -Wno-old-style-cast
    -Wno-reorder
    -Wno-shadow
    -Wno-sign-conversion
    -Wno-suggest-override
    -Wno-switch
    -Wno-switch-enum
    -Wno-unused-macros
    -Wno-unused-parameter
    -Wno-unused-variable
    -Wno-zero-as-null-pointer-constant
)

# Helper functions for GCC to filter down a list of warning flags to what's supported
function(_cobalt_build_supported_cxx_compiler_flag_list out_var)
    include(CheckCXXCompilerFlag)

    set(_cobalt_supported "")
    foreach(_cobalt_flag IN LISTS ARGN)
        # Create a safe cache var name from the flag so we don't re-test endlessly.
        string(REGEX REPLACE "^[/-]" "" _cobalt_flag_id "${_cobalt_flag}")
        string(REGEX REPLACE "[^A-Za-z0-9_]" "_" _cobalt_flag_id "${_cobalt_flag_id}")
        set(_cobalt_var "COBALT_HAS_${_cobalt_flag_id}")

        check_cxx_compiler_flag("${_cobalt_flag}" ${_cobalt_var})
        if(${_cobalt_var})
            list(APPEND _cobalt_supported "${_cobalt_flag}")
        endif()
    endforeach()

    set(${out_var} "${_cobalt_supported}" PARENT_SCOPE)
endfunction()
function(_cobalt_build_supported_c_compiler_flag_list out_var)
    include(CheckCCompilerFlag)

    set(_cobalt_supported "")
    foreach(_cobalt_flag IN LISTS ARGN)
        # Create a safe cache var name
        string(REGEX REPLACE "^[/-]" "" _cobalt_flag_id "${_cobalt_flag}")
        string(REGEX REPLACE "[^A-Za-z0-9_]" "_" _cobalt_flag_id "${_cobalt_flag_id}")
        set(_cobalt_var "COBALT_HAS_${_cobalt_flag_id}")

        check_c_compiler_flag("${_cobalt_flag}" ${_cobalt_var})
        if(${_cobalt_var})
            list(APPEND _cobalt_supported "${_cobalt_flag}")
        endif()
    endforeach()

    set(${out_var} "${_cobalt_supported}" PARENT_SCOPE)
endfunction()

# Filter down the list of GCC warning settings to what's supported by the current GCC compiler
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    _cobalt_build_supported_cxx_compiler_flag_list(COBALT_GCC_CXX_WARNING_FLAGS ${COBALT_GCC_ALL_CXX_WARNING_FLAGS})
    _cobalt_build_supported_c_compiler_flag_list(COBALT_GCC_C_WARNING_FLAGS ${COBALT_GCC_ALL_C_WARNING_FLAGS})
endif()

# Helper function to apply relevant compiler flags to the specified target
function(cobalt_apply_compiler_warning_settings target)
    # Ensure that compiler warning settings can be set for the target
    if(NOT TARGET "${target}")
        message(FATAL_ERROR "cobalt_apply_compiler_warning_settings: '${target}' is not a CMake target")
    endif()
    get_target_property(_cobalt_tgt_type "${target}" TYPE)
    if(_cobalt_tgt_type STREQUAL "INTERFACE_LIBRARY")
        message(FATAL_ERROR "cobalt_apply_compiler_warning_settings: '${target}' is an interface")
    endif()

    # Apply compiler warning flags based on the type of compiler
    if(MSVC AND NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        message(STATUS "Cobalt: applying MSVC warning settings to '${target}'")
        target_compile_options("${target}" PRIVATE ${COBALT_MSVC_WARNING_FLAGS})
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "^(AppleClang|Clang)$")
        message(STATUS "Cobalt: applying Clang warning settings to '${target}'")
        target_compile_options("${target}" PRIVATE ${COBALT_CLANG_WARNING_FLAGS})
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        message(STATUS "Cobalt: applying GCC warning settings to '${target}'")
        target_compile_options("${target}" PRIVATE
            $<$<COMPILE_LANGUAGE:CXX>:${COBALT_GCC_CXX_WARNING_FLAGS}>
            $<$<COMPILE_LANGUAGE:C>:${COBALT_GCC_C_WARNING_FLAGS}>
        )
    endif()

    # Add _DEBUG macro for debug build targets
    target_compile_definitions("${target}" PRIVATE $<$<CONFIG:Debug>:_DEBUG>)

    # Don't allow unresolved external symbols on shared libraries
    if(UNIX AND NOT APPLE AND (_cobalt_tgt_type STREQUAL "SHARED_LIBRARY" OR _cobalt_tgt_type STREQUAL "MODULE_LIBRARY"))
        target_link_options("${target}" PRIVATE "LINKER:--no-undefined")
    endif()

endfunction()
