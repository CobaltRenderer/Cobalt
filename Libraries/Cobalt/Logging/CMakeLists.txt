# Specify our source files
set(Logging_HEADERS
    "Logging.pkg"
    "ILogTarget.h"
    "Logger.h"
    "Logger.inl"
    "LogManager.h"
    "LogManager.inl"
    "LogTargetCallback.h"
    "LogTargetCallback.inl"
    "LogTargetConsole.h"
    "LogTargetConsole.inl"
    "LogTargetStandardOut.h"
    "LogTargetStandardOut.inl"
    "LogTargetFile.h"
    "LogTargetFile.inl"
)

# Define our project targets
add_library(Logging_Lib INTERFACE)
add_library(Cobalt::Logging ALIAS Logging_Lib)
target_include_directories(Logging_Lib INTERFACE ${COBALT_INTERNAL_INCLUDE_DIR})

# Dummy targets to show in the IDE and register for clang-format
add_library(Logging STATIC ${Logging_HEADERS})
register_clang_format_and_tidy_target(Logging USAGE_TARGET Cobalt::Logging)
set_target_properties(Logging PROPERTIES LINKER_LANGUAGE CXX EXCLUDE_FROM_ALL TRUE)
set_target_properties(Logging PROPERTIES FOLDER "Libraries/Public")

# Specify our project dependencies
target_link_libraries(Logging_Lib INTERFACE
    Cobalt::LoggingInterface
)

# Transfer required files to their output locations
cobalt_install_sdk_headers(Logging ${Logging_HEADERS})

# Define our virtual project folder and file structure
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${Logging_HEADERS})
source_group("Log Targets" FILES
    "LogTargetConsole.h"
    "LogTargetConsole.inl"
    "LogTargetFile.h"
    "LogTargetFile.inl"
    "LogTargetCallback.h"
    "LogTargetCallback.inl"
    "LogTargetStandardOut.h"
    "LogTargetStandardOut.inl"
)
