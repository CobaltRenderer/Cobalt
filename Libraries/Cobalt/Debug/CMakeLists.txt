# Specify our source files
set(Debug_HEADERS
    "Debug.pkg"
    "Debug.h"
    "Debug.inl"
    "Warnings.h"
)

# Define our project targets
add_library(Debug_Lib INTERFACE)
add_library(Cobalt::Debug ALIAS Debug_Lib)
target_include_directories(Debug_Lib INTERFACE ${COBALT_INTERNAL_INCLUDE_DIR})

# Dummy targets to show in the IDE and register for clang-format
add_library(Debug STATIC ${Debug_HEADERS})
register_clang_format_and_tidy_target(Debug USAGE_TARGET Cobalt::Debug)
set_target_properties(Debug PROPERTIES LINKER_LANGUAGE CXX EXCLUDE_FROM_ALL TRUE)
set_target_properties(Debug PROPERTIES FOLDER "Libraries/Public")

# Transfer required files to their output locations
cobalt_install_sdk_headers(Debug ${Debug_HEADERS})

# Define our virtual project folder and file structure
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${Debug_HEADERS})
