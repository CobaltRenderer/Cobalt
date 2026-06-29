include_guard(GLOBAL)

# Enable IPO for release builds if supported by the current toolchain
include(CheckIPOSupported)
check_ipo_supported(RESULT cobalt_ipo_supported LANGUAGES C CXX)
if(cobalt_ipo_supported)
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELWITHDEBINFO ON)
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE ON)
    message(STATUS "IPO/LTO enabled")
endif()

# Re-enable dead code elimination and COMDAT folding on MSVC under RelWithDebInfo
if(MSVC)
    add_link_options(
        $<$<CONFIG:RelWithDebInfo>:/OPT:REF>
        $<$<CONFIG:RelWithDebInfo>:/OPT:ICF>
        $<$<CONFIG:RelWithDebInfo>:/INCREMENTAL:NO>
    )
endif()
