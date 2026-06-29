# macos-clang-arm64.cmake
set(CMAKE_SYSTEM_NAME Darwin)
set(CMAKE_SYSTEM_PROCESSOR arm64)

set(CMAKE_C_COMPILER /usr/bin/clang)
set(CMAKE_CXX_COMPILER /usr/bin/clang++)

# Ensure we build for Apple Silicon
set(CMAKE_OSX_ARCHITECTURES "arm64" CACHE STRING "" FORCE)
