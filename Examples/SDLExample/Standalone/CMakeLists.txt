cmake_minimum_required(VERSION 3.21)

project(CobaltStandaloneSDLExample LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(COBALT_RENDERER_SDK_ROOT "" CACHE PATH "Path to an installed Cobalt Renderer SDK root")
if(COBALT_RENDERER_SDK_ROOT)
    list(PREPEND CMAKE_PREFIX_PATH "${COBALT_RENDERER_SDK_ROOT}")
endif()

find_package(CobaltRenderer CONFIG REQUIRED)
find_package(SDL3 CONFIG REQUIRED)

add_executable(SDLExample
    ../main.cpp
)

target_link_libraries(SDLExample
    PRIVATE
        Cobalt::Headers
        SDL3::SDL3
)

if(APPLE)
    enable_language(OBJCXX)
    set_source_files_properties(main.cpp PROPERTIES LANGUAGE OBJCXX)
    target_link_libraries(SDLExample
        PRIVATE
            "-framework Cocoa"
            "-framework QuartzCore"
            "-framework Metal"
    )
elseif(UNIX)
    find_package(X11 REQUIRED)
    target_include_directories(SDLExample PRIVATE ${X11_INCLUDE_DIR})
    target_link_libraries(SDLExample PRIVATE ${X11_LIBRARIES})

    find_library(X11_XCB_LIBRARY NAMES X11-xcb REQUIRED)
    find_path(WAYLAND_CLIENT_INCLUDE_DIR NAMES wayland-client.h REQUIRED)
    find_library(WAYLAND_CLIENT_LIBRARY NAMES wayland-client REQUIRED)
    target_include_directories(SDLExample PRIVATE "${WAYLAND_CLIENT_INCLUDE_DIR}")
    target_link_libraries(SDLExample PRIVATE "${X11_XCB_LIBRARY}" "${WAYLAND_CLIENT_LIBRARY}")
endif()

cobalt_copy_runtime_binaries(SDLExample)

install(TARGETS SDLExample
    RUNTIME DESTINATION "."
    BUNDLE DESTINATION "."
)
cobalt_install_runtime_binaries(DESTINATION ".")
