# Specify our source files
set(MathTypes_HEADERS
    "MathTypes.pkg"
    "BasicHalfFloat.h"
    "BasicMatrix.h"
    "BasicNormalizedInts.h"
    "BasicVector.h"
)

# Define our project targets
add_library(MathTypes_Lib INTERFACE)
add_library(Cobalt::MathTypes ALIAS MathTypes_Lib)
target_include_directories(MathTypes_Lib INTERFACE ${COBALT_INTERNAL_INCLUDE_DIR})

# Dummy targets to show in the IDE and register for clang-format
add_library(MathTypes STATIC ${MathTypes_HEADERS})
register_clang_format_and_tidy_target(MathTypes USAGE_TARGET Cobalt::MathTypes)
set_target_properties(MathTypes PROPERTIES LINKER_LANGUAGE CXX EXCLUDE_FROM_ALL TRUE)
set_target_properties(MathTypes PROPERTIES FOLDER "Libraries/Public")

# Transfer required files to their output locations
cobalt_install_sdk_headers(MathTypes ${MathTypes_HEADERS})

# Define our virtual project folder and file structure
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES
    BasicHalfFloat.h
    BasicMatrix.h
    BasicNormalizedInts.h
    BasicVector.h
)
