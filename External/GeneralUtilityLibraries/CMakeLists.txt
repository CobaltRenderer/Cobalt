# zlib
set(SKIP_INSTALL_ALL     ON  CACHE BOOL "" FORCE)
set(ZLIB_BUILD_EXAMPLES  OFF CACHE BOOL "" FORCE)
FetchContent_Declare(zlibPackage
    GIT_REPOSITORY "https://github.com/madler/zlib.git"
    GIT_TAG "v1.3.1"
    SOURCE_DIR "${COBALT_EXTERNAL_ROOT}/Cache/zlib"
)
FetchContent_MakeAvailable(zlibPackage)
set_project_virtual_folder(zlib "External/zlib")
set_project_virtual_folder(zlibstatic "External/zlib")
FetchContent_GetProperties(zlibPackage
  SOURCE_DIR zlib_src
  BINARY_DIR zlib_bin
)
file(COPY_FILE "${zlib_src}/zlib.h" "${zlib_bin}/zlib.h" ONLY_IF_DIFFERENT)
target_include_directories(zlibstatic PUBLIC "${zlib_bin}")
# Make zlib discoverable as a package to keep libpng happy
set(ZLIB_INCLUDE_DIR  "${zlib_bin}"               CACHE STRING "" FORCE)
set(ZLIB_INCLUDE_DIRS "${zlib_bin}"               CACHE STRING "" FORCE)
set(ZLIB_LIBRARY      zlibstatic                  CACHE STRING "" FORCE)
set(ZLIB_LIBRARIES    zlibstatic                  CACHE STRING "" FORCE)
set(ZLIB_USE_STATIC_LIBS ON                       CACHE BOOL  ""  FORCE)
add_library(ZLIB::ZLIB ALIAS zlibstatic)

# libpng
set(SKIP_INSTALL_ALL     ON  CACHE BOOL "" FORCE)
set(PNG_SHARED OFF CACHE BOOL "Build shared libpng" FORCE)
set(PNG_STATIC ON  CACHE BOOL "Build static libpng" FORCE)
set(PNG_FRAMEWORK OFF CACHE BOOL "Build macos libpng framework" FORCE)
set(PNG_TESTS  OFF CACHE BOOL "Build libpng tests" FORCE)
set(PNG_TOOLS  OFF CACHE BOOL "Build libpng tools" FORCE)
set(PNG_ARM_NEON off CACHE STRING "Enable ARM NEON optimizations" FORCE)
FetchContent_Declare(libpngPackage
    GIT_REPOSITORY "https://github.com/pnggroup/libpng.git"
    GIT_TAG "v1.6.51"
    SOURCE_DIR "${COBALT_EXTERNAL_ROOT}/Cache/libpng"
)
FetchContent_MakeAvailable(libpngPackage)
set_project_virtual_folder(png_static "External/libpng")
set_project_virtual_folder(png_genfiles "External/libpng")

# GLI
FetchContent_Declare(gli
    GIT_REPOSITORY "https://github.com/g-truc/gli.git"
    GIT_TAG "ea5e33aeab9b5f0c90e3d33ea8b794bf9e1dc006" # 0.8.4
    SOURCE_DIR "${COBALT_EXTERNAL_ROOT}/Cache/gli"
)
FetchContent_Populate(gli)
add_library(gli INTERFACE)
target_include_directories(gli SYSTEM INTERFACE
    "${COBALT_EXTERNAL_ROOT}/Cache/gli/external"
    "${COBALT_EXTERNAL_ROOT}/Cache/gli"
)
set_project_virtual_folder(gli "External/gli")

# SDL3
if(COBALT_USE_SDL3)
    if(COBALT_USE_LOCAL_SDL3)
        find_package(SDL3 REQUIRED CONFIG)
    else()
        set(SDL_STATIC ON CACHE BOOL "" FORCE)
        set(SDL_SHARED OFF CACHE BOOL "" FORCE)
        set(SDL_TESTS OFF CACHE BOOL "" FORCE)
        set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)
        FetchContent_Declare(libsdl3Package
            GIT_REPOSITORY "https://github.com/libsdl-org/SDL.git"
            GIT_TAG "release-3.4.0"
            SOURCE_DIR "${COBALT_EXTERNAL_ROOT}/Cache/sdl3"
        )
        FetchContent_MakeAvailable(libsdl3Package)
        set_project_virtual_folder(SDL_uclibc "External/SDL3")
        set_project_virtual_folder(SDL3-static "External/SDL3")
    endif()
endif()
