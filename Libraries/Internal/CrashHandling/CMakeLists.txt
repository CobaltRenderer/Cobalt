# Specify our source files
set(CrashHandling_HEADERS
    "CrashHandling.pkg"
    "CrashHandler.h"
    "CrashHandler.inl"
)

# Define our project targets
add_library(CrashHandling_Lib INTERFACE)
add_library(Cobalt::CrashHandling ALIAS CrashHandling_Lib)
target_include_directories(CrashHandling_Lib INTERFACE ${COBALT_INTERNAL_INCLUDE_DIR})

# Dummy targets to show in the IDE and register for clang-format
add_library(CrashHandling STATIC ${CrashHandling_HEADERS})
register_clang_format_and_tidy_target(CrashHandling USAGE_TARGET Cobalt::CrashHandling)
set_target_properties(CrashHandling PROPERTIES LINKER_LANGUAGE CXX EXCLUDE_FROM_ALL TRUE)
set_target_properties(CrashHandling PROPERTIES FOLDER "Libraries/Private")

# Specify our project dependencies
if (WIN32)
    target_link_libraries(CrashHandling_Lib
        INTERFACE
            DbgHelp
    )
endif()

# Define our virtual project folder and file structure
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${CrashHandling_HEADERS})
