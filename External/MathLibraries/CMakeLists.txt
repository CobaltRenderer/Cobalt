# glm
FetchContent_Declare(glm
    GIT_REPOSITORY "https://github.com/g-truc/glm.git"
    GIT_TAG        "1.0.2"
    SOURCE_DIR     "${COBALT_EXTERNAL_ROOT}/Cache/glm"
)
FetchContent_Populate(glm)
add_library(glm INTERFACE)
target_include_directories(glm SYSTEM INTERFACE "${COBALT_EXTERNAL_ROOT}/Cache/glm")

# half
FetchContent_Declare(half
    URL        "https://sourceforge.net/projects/half/files/half/2.2.0/half-2.2.0.zip/download"
    SOURCE_DIR "${COBALT_EXTERNAL_ROOT}/Cache/half"
)
FetchContent_MakeAvailable(half)
add_library(half INTERFACE)
target_include_directories(half SYSTEM INTERFACE "${COBALT_EXTERNAL_ROOT}/Cache/half/include")
