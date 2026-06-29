include_guard(GLOBAL)

option(COBALT_RUN_MSVC_STATIC_ANALYSIS "Specifies whether Visual Studio static analysis is being run during the build process." OFF)
set(COBALT_MSVC_STATIC_ANALYSIS_RULESET "${PROJECT_SOURCE_DIR}/CobaltAnalysisRules.ruleset" CACHE PATH "MSVC code analysis ruleset")

# If static analysis has been enabled, add the necessary compiler flags now.
if(MSVC AND COBALT_RUN_MSVC_STATIC_ANALYSIS)
    add_compile_options(/analyze)
    add_compile_options("/analyze:ruleset${COBALT_MSVC_STATIC_ANALYSIS_RULESET}")
endif()

# Define our helper function to configure our msvc-code-analysis build target
function(cobalt_setup_visual_studio_static_analysis)
    # Define our arguments to use when invoking cmake
    set(cobalt_msvc_static_analysis_binary_dir "${CMAKE_BINARY_DIR}/msca")
    set(
        cobalt_msvc_static_analysis_configure_args
        -S "${PROJECT_SOURCE_DIR}"
        -B "${cobalt_msvc_static_analysis_binary_dir}"
        -G "${CMAKE_GENERATOR}"
        -DCOBALT_RUN_MSVC_STATIC_ANALYSIS=ON
        "-DCOBALT_MSVC_STATIC_ANALYSIS_RULESET:PATH=${COBALT_MSVC_STATIC_ANALYSIS_RULESET}"
    )
    if(CMAKE_GENERATOR_PLATFORM)
        list(APPEND cobalt_msvc_static_analysis_configure_args -A "${CMAKE_GENERATOR_PLATFORM}")
    endif()
    if(CMAKE_GENERATOR_TOOLSET)
        list(APPEND cobalt_msvc_static_analysis_configure_args -T "${CMAKE_GENERATOR_TOOLSET}")
    endif()
    if(CMAKE_TOOLCHAIN_FILE)
        list(APPEND cobalt_msvc_static_analysis_configure_args "-DCMAKE_TOOLCHAIN_FILE:FILEPATH=${CMAKE_TOOLCHAIN_FILE}")
    endif()
    get_property(cobalt_generator_is_multi_config GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
    if(cobalt_generator_is_multi_config)
        list(APPEND cobalt_msvc_static_analysis_configure_args "-DCMAKE_CONFIGURATION_TYPES:STRING=${CMAKE_CONFIGURATION_TYPES}")
        set(cobalt_msvc_static_analysis_build_args --build "${cobalt_msvc_static_analysis_binary_dir}" --config $<CONFIG>)
    else()
        list(APPEND cobalt_msvc_static_analysis_configure_args "-DCMAKE_BUILD_TYPE:STRING=${CMAKE_BUILD_TYPE}")
        set(cobalt_msvc_static_analysis_build_args --build "${cobalt_msvc_static_analysis_binary_dir}")
    endif()

    # Add our custom target to run static analysis
    add_custom_target(
        msvc-code-analysis
        COMMAND
            "${CMAKE_COMMAND}"
            ${cobalt_msvc_static_analysis_configure_args}
        COMMAND
            "${CMAKE_COMMAND}"
            ${cobalt_msvc_static_analysis_build_args}
        USES_TERMINAL
    )
    set_target_properties(msvc-code-analysis PROPERTIES FOLDER "CustomTargets")
endfunction()
