# Specify our source files
set(LoggingInterface_HEADERS
    "LoggingInterface.pkg"
    "ILogger.h"
    "ILogger.inl"
    "NullLogger.h"
    "NullLogger.inl"
)

# Define our project targets
add_library(LoggingInterface_Lib INTERFACE)
add_library(Cobalt::LoggingInterface ALIAS LoggingInterface_Lib)
target_include_directories(LoggingInterface_Lib INTERFACE ${COBALT_INTERNAL_INCLUDE_DIR})

# Dummy targets to show in the IDE and register for clang-format
add_library(LoggingInterface STATIC ${LoggingInterface_HEADERS})
register_clang_format_and_tidy_target(LoggingInterface USAGE_TARGET Cobalt::LoggingInterface)
set_target_properties(LoggingInterface PROPERTIES LINKER_LANGUAGE CXX EXCLUDE_FROM_ALL TRUE)
set_target_properties(LoggingInterface PROPERTIES FOLDER "Libraries/Public")

# Transfer required files to their output locations
cobalt_install_sdk_headers(LoggingInterface ${LoggingInterface_HEADERS})

# Define our virtual project folder and file structure
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${LoggingInterface_HEADERS})
