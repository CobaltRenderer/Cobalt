# macos-clang-x64.cmake
set(CMAKE_SYSTEM_NAME Darwin)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

set(CMAKE_C_COMPILER /usr/bin/clang)
set(CMAKE_CXX_COMPILER /usr/bin/clang++)

# Ensure we build for Intel macs
set(CMAKE_OSX_ARCHITECTURES "x86_64" CACHE STRING "" FORCE)
