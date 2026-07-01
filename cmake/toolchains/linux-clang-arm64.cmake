# linux-clang-arm64.cmake
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR aarch64)

set(CMAKE_C_COMPILER /usr/bin/clang)
set(CMAKE_CXX_COMPILER /usr/bin/clang++)

set(CMAKE_CXX_FLAGS_INIT "-stdlib=libc++")
set(CMAKE_CXX_LINK_FLAGS_INIT "-stdlib=libc++")
