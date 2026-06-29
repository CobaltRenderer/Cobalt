# OpenGLRegistry
FetchContent_Declare(OpenGLRegistry
    GIT_REPOSITORY "https://github.com/KhronosGroup/OpenGL-Registry.git"
    GIT_TAG        "8f9b0ca1a8eafa2cebde3ff982629e7913d2817f"
    SOURCE_DIR     "${COBALT_EXTERNAL_ROOT}/Cache/OpenGL-Registry"
)
FetchContent_Populate(OpenGLRegistry)
add_library(OpenGLRegistry INTERFACE)
target_include_directories(OpenGLRegistry SYSTEM INTERFACE "${COBALT_EXTERNAL_ROOT}/Cache/OpenGL-Registry/api")
