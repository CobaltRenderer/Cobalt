# This file imports third party libraries which are used by our project, wrapping them into cmake targets so that output
# files and required paths can be passed down to consumers.
if(POLICY CMP0135)
  cmake_policy(SET CMP0135 NEW)
endif()
include(FetchContent)
include(ExternalProject)

# Define a helper function to assist moving imported third party library targets in the virtual folder structure
function(set_project_virtual_folder tgt folder)
    if(TARGET "${tgt}")
        set_target_properties("${tgt}" PROPERTIES FOLDER "${folder}")
    endif()
endfunction()

# Load each required set of third party libraries
add_subdirectory(MathLibraries)
add_subdirectory(ShaderUtilities)
if (WIN32)
    add_subdirectory(Direct3D12Libraries)
endif()
add_subdirectory(OpenGLLibraries)
add_subdirectory(VulkanLibraries)
add_subdirectory(GeneralUtilityLibraries)
if (UNIX AND NOT APPLE)
    add_subdirectory(UnixUILibraries)
endif()
