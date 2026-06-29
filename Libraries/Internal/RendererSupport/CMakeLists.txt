# Specify our source files
set(RendererSupport_HEADERS
    "KnownDynamicCast.h"
    "RenderMarkers.h"
    "UnicodeConversion.h"
    "VariantHasType.h"
    "VendorName.h"
)

# Define our project targets
add_library(RendererSupport_Lib INTERFACE)
add_library(Cobalt::RendererSupport ALIAS RendererSupport_Lib)
target_include_directories(RendererSupport_Lib INTERFACE ${COBALT_INTERNAL_INCLUDE_DIR})

# Dummy targets to show in the IDE and register for clang-format
add_library(RendererSupport STATIC ${RendererSupport_HEADERS})
register_clang_format_and_tidy_target(RendererSupport USAGE_TARGET Cobalt::RendererSupport)
set_target_properties(RendererSupport PROPERTIES LINKER_LANGUAGE CXX EXCLUDE_FROM_ALL TRUE)
set_target_properties(RendererSupport PROPERTIES FOLDER "Libraries/Private")

# Specify our project dependencies
target_link_libraries(RendererSupport_Lib INTERFACE
    Cobalt::Debug
    Cobalt::RendererInterface
)

# Define our virtual project folder and file structure
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${RendererSupport_HEADERS})
source_group("Type Traits" FILES
    "VariantHasType.h"
)
source_group("VendorName" FILES
    "VendorName.h"
)
