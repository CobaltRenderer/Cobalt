<img src="Assets/logos/ColorTransparentText.png" alt="Project Logo" width="300">

[![CI Build - Windows/Ubuntu/macOS](https://github.com/CobaltRenderer/Cobalt/actions/workflows/CIBuild.yml/badge.svg?branch=main&event=push)](https://github.com/CobaltRenderer/Cobalt/actions/workflows/CIBuild.yml)

The Cobalt Renderer is a generic, cross platform graphics library. Cobalt aims to make using modern 3D graphics hardware simple. It does this by giving you a clean
API to express **what** you want to draw (or compute), without getting bogged down in **how** to do that. Cobalt is designed to be an accessible, lightweight
alternative to directly using low-level graphics APIs such as Vulkan, Direct3D and OpenGL.

The Cobalt Renderer was originally developed in 2018-2020 as an internal library for a range of commercial software. As of 2026, this library is now available for
free as an open-source project under the permissive MIT license.

## Main Features
- Clear interface that unifies concepts and features across all available graphics APIs
- Native C++ library, with C bindings available for easy integration into other languages, such as Rust (coming soon)
- Plugin based architecture with implementations using Vulkan, Direct3D, OpenGL and Metal (coming soon) as the backing graphics API
- Header-only interface with no static libraries to link, and no compiler version/settings to match
- Strong cross-platform (Windows, Linux, macOS) and hardware support, backed by an extensive test suite
- Shader cross compilation, allowing you to write your shaders once for all platforms in common formats like HLSL, GLSL, SPIR-V and more
- Advanced optimizations which would be difficult to achieve at an application level

## Why does this exist?
From around 2005, 3D graphics hardware has in many ways gotten simpler, while conversely, graphics APIs have exploded in complexity. What took a hundred lines of
code to express in OpenGL in 1996 now takes a few thousand lines of code on any modern graphics API. At the same time, the games industry, the primary driver of
advances in 3D graphics hardware, has consolidated around a handful of large engines and middleware packages. Graphics APIs now primarily cater to the interests of
these engines, which want the lowest level, most direct access to the hardware possible. Modern graphics APIs require you to worry about issues like memory residency,
shared vs separate command queues, or differences between unified vs discrete video memory, which while great for micro-optimizing a game engine on console hardware
with known fixed system specs, makes it virtually impossible to just draw a few simple polygons to the screen in a standalone product, especially on PC hardware with
a wide range of graphics devices with differing capabilities. As graphics hardware and APIs continue to change and evolve, there's also a significant maintenance
burden keeping code that drives low-level graphics APIs current.

The Cobalt Renderer addresses these issues by remaining focused on the high-level of what you're actually trying to do, giving you a natural way of expressing that in
a way that isn't platform or API dependent. You have full control over your draw/compute operations, with full flexibility to define your shaders the way you want,
but all the "glue" of setting up the hardware, putting everything into the correct state, and moving data around or synchronizing changes between shader stages is
handled for you. The Cobalt Renderer understands what you want to do at a high enough level to ensure this all happens at the right time, in the right order, using
the most efficient techniques on each underlying API, to achieve the best possible performance. Additionally, shader cross-compilation is built in, allowing you to
write your shader code once in one format, such as HLSL or compiled SPIR-V, and run that code on any platform, with any underlying renderer API. This makes it possible
to put a polygon on the screen with a hundred lines of code again, while still allowing you to do incredibly demanding drawing tasks at speed and scale, with better
performance than you would be likely to achieve even if you did it all yourself by hand.

## What **ISN'T** the Cobalt Renderer?
1. **Not a full engine/framework.** The Cobalt Renderer just does rendering. It doesn't handle collision detection, physics, scripting, asset formats, audio, input handling,
or anything else of that nature. If you're looking for a solution which covers those areas, you should look at other packages which cater for that, such as a game
engine. The Cobalt Renderer could be used as a *component* within a game engine, but its focus is 100% on rendering/compute alone.
2. **Not limited to game development.** The Cobalt Renderer is not built for any one specific task. It is built to drive tasks on modern 3D graphics hardware. That could be
realtime gaming. It could also be event-driven CAD software, or cloud-based graphics rendering, or data analytics, or pure GPU compute. It doesn't assume you're doing
any kind of specific task. This makes it flexible and useful as a component in a wide variety of areas, including game development, but it's not built assuming
that's what you will be doing.

# Design Philosophy
- **Keep a clean and expressive interface.** The API should have clear naming with no superfluous functionality.
- **Distill the core functionality of underlying APIs.** Various characteristics from each API should be focused to their common elements.
- **Implement established technologies.** Only implement functionality and extensions with strong cross-vendor adoption.
- **Simplicity over squeezing performance.** A maintainable and clear architecture is more important than chasing a marginal improvement in performance.

# Getting started

### Binary releases
You can find the latest binary release of the Cobalt Renderer [here](https://github.com/CobaltRenderer/Cobalt/releases/latest). All historical open-source
releases are available under the [Releases](https://github.com/CobaltRenderer/Cobalt/releases) section. The API itself is header-only. You do not need to
link to any libraries in order to compile against the Cobalt Renderer. You can optionally choose to statically link against particular renderers, or you can
load the renderers themselves at runtime.

Note that due to the use of interfaces and marshalling in the Cobalt Renderer API, you are able to use binary releases even if you use a different C++ compiler,
or a different version of the compiler than was used to generate these releases. You are also able to use either debug or release versions of the renderer
plugins regardless of the configuration of your project.

### SDK documentation
You can find the latest SDK documentation for this project hosted on GitHub pages at [https://cobaltrenderer.github.io/Cobalt](https://cobaltrenderer.github.io/Cobalt).
Additionally, the SDK documentation is included within each SDK release. This documentation aims to provide comprehensive coverage of the API, as well as an
overview of the high-level concepts behind it.

### Examples
In addition to the information contained in the SDK documentation, the repository contains a growing set of code samples under the "Examples" directory. For
a basic, cross-platform example of how to do rendering, you can refer to [SDLExample](Examples/SDLExample/main.cpp), which is a good place to get
started.

The automated test framework under the "Tests" directory also contains short code snippets showing the usage of many features in the renderer API. You can refer
to these for practical examples of using the graphics API.

### Automated tests
The Cobalt Renderer contains a comprehensive automated test framework under the "Testing" directory. This testing suite covers the entire surface area of the
graphics API, and achieves over 90% code coverage on all renderers. These tests are run continuously when code changes are made, across all target operating
systems on a variety of hardware platforms. These tests serve as examples of how to use various API features, while also serving to prove the renderer works
as intended, and help ensure it stays working as changes are made and new features are added. These tests can also be run manually, and the test suite is
provided as a build artifact for each release.

# Building from source
The provided binary releases are the easiest way to use the Cobalt renderer in your application. If you want to build from source however, that's fairly
straightforward. Cobalt uses a modern CMake project, with presets and toolchain files to simplify configuration and building. The process is largely the same
on all platforms, and is basically as follows:
1. Install prerequisites
2. Clone the repository
3. Select a preset
4. Configure/Build/Install using CMake
5. Run/Debug/Use the installed build artifacts from the "Output" directory

## Prerequisites
There are very few prerequisites for building the Cobalt renderer, as most dependencies are retrieved and built automatically. The following packages are
assumed to be present before attempting a build however:
1. Git
2. CMake 3.21 or above
3. Python 3
4. C++ build tools for your platform

### Windows
For Windows, this is best satisfied by installing Visual Studio 2022 or above, and selecting Git, CMake, and optionally clang-cl during installation. You must
also select Python 3 or install it separately if it is not already present. No further dependencies are required.

### macOS
For macOS, you need to install Homebrew, which will also install the Xcode build tools for your system. This is best done by following the steps from the
official Homebrew site at [https://brew.sh](https://brew.sh). After that is complete, you can prepare your build environment by running the following commands
from the terminal:
```
brew update
brew install git ninja cmake clang-format
```
If you want to support clang-tidy on macOS, you should also run the following:
```
brew install llvm
```

### Linux
For Linux, numerous system libraries are required beyond the build tools themselves. This project tests on Ubuntu. For this platform, you can prepare your environment by running the following commands:
```
sudo apt update
# General build tools
sudo apt install -y git cmake ninja-build pkg-config
sudo apt install -y clang libc++-dev libc++abi-dev
# X11/XCB/Wayland
sudo apt install -y libxcb1 libxcb1-dev libx11-dev libx11-xcb-dev libwayland-dev libcairo-dev libxrandr-dev libxcursor-dev wayland-protocols
# OpenGL mesa libraries
sudo apt install -y mesa-utils mesa-common-dev libgl1-mesa-dev libglu1-mesa-dev
# SDL3 dependencies
sudo apt install -y build-essential make gnome-desktop-testing libasound2-dev libpulse-dev libaudio-dev libfribidi-dev libjack-dev libsndio-dev libx11-dev libxext-dev libxrandr-dev libxcursor-dev libxfixes-dev libxi-dev libxss-dev libxtst-dev libxkbcommon-dev libdrm-dev libgbm-dev libgl1-mesa-dev libgles2-mesa-dev libegl1-mesa-dev libdbus-1-dev libibus-1.0-dev libudev-dev libthai-dev
# SDK documentation generation
sudo apt install -y xsltproc
```

Other Linux distros may require a different set of packages.

## Presets
The Cobalt renderer has a list of predefined presets that cover all supported platforms. You can get a list of the presets from the shell using the following command:
```
cmake --list-presets
```
Here is a complete list of the current set of presets:
```
  "win-msvc-x64-debug"        - Win MSVC x64 Debug
  "win-msvc-x64-asan"         - Win MSVC x64 Asan
  "win-msvc-x64-release"      - Win MSVC x64 RelWithDebInfo
  "win-msvc-x86-debug"        - Win MSVC x86 Debug
  "win-msvc-x86-asan"         - Win MSVC x86 Asan
  "win-msvc-x86-release"      - Win MSVC x86 RelWithDebInfo
  "win-msvc-arm64-debug"      - Win MSVC ARM64 Debug
  "win-msvc-arm64-asan"       - Win MSVC ARM64 Asan
  "win-msvc-arm64-release"    - Win MSVC ARM64 RelWithDebInfo
  "win-clangcl-x64-debug"     - Win Clang-cl x64 Debug
  "win-clangcl-x64-asan"      - Win Clang-cl x64 Asan
  "win-clangcl-x64-release"   - Win Clang-cl x64 RelWithDebInfo
  "win-clangcl-x86-debug"     - Win Clang-cl x86 Debug
  "win-clangcl-x86-asan"      - Win Clang-cl x86 Asan
  "win-clangcl-x86-release"   - Win Clang-cl x86 RelWithDebInfo
  "win-clangcl-arm64-debug"   - Win Clang-cl ARM64 Debug
  "win-clangcl-arm64-asan"    - Win Clang-cl ARM64 Asan
  "win-clangcl-arm64-release" - Win Clang-cl ARM64 RelWithDebInfo
  "macos-clang-x64-debug"     - macOS Clang x64 Debug
  "macos-clang-x64-asan"      - macOS Clang x64 Asan
  "macos-clang-x64-release"   - macOS Clang x64 RelWithDebInfo
  "macos-clang-arm64-debug"   - macOS Clang ARM64 Debug
  "macos-clang-arm64-asan"    - macOS Clang ARM64 Asan
  "macos-clang-arm64-release" - macOS Clang ARM64 RelWithDebInfo
  "linux-gcc-x64-debug"       - Linux GCC x64 Debug
  "linux-gcc-x64-asan"        - Linux GCC x64 Asan
  "linux-gcc-x64-release"     - Linux GCC x64 RelWithDebInfo
  "linux-gcc-arm64-debug"     - Linux GCC ARM64 Debug
  "linux-gcc-arm64-asan"      - Linux GCC ARM64 Asan
  "linux-gcc-arm64-release"   - Linux GCC ARM64 RelWithDebInfo
  "linux-clang-x64-debug"     - Linux Clang x64 Debug
  "linux-clang-x64-asan"      - Linux Clang x64 Asan
  "linux-clang-x64-release"   - Linux Clang x64 RelWithDebInfo
  "linux-clang-arm64-debug"   - Linux Clang ARM64 Debug
  "linux-clang-arm64-asan"    - Linux Clang ARM64 Asan
  "linux-clang-arm64-release" - Linux Clang ARM64 RelWithDebInfo
  "wsl-gcc-x64-debug"         - WSL GCC x64 Debug
  "wsl-gcc-x64-asan"          - WSL GCC x64 Asan
  "wsl-gcc-x64-release"       - WSL GCC x64 RelWithDebInfo
  "wsl-gcc-arm64-debug"       - WSL GCC ARM64 Debug
  "wsl-gcc-arm64-asan"        - WSL GCC ARM64 Asan
  "wsl-gcc-arm64-release"     - WSL GCC ARM64 RelWithDebInfo
  "wsl-clang-x64-debug"       - WSL Clang x64 Debug
  "wsl-clang-x64-asan"        - WSL Clang x64 Asan
  "wsl-clang-x64-release"     - WSL Clang x64 RelWithDebInfo
  "wsl-clang-arm64-debug"     - WSL Clang ARM64 Debug
  "wsl-clang-arm64-asan"      - WSL Clang ARM64 Asan
  "wsl-clang-arm64-release"   - WSL Clang ARM64 RelWithDebInfo
```

## CMake options
For most purposes you can leave all CMake settings at their defaults, however the following custom CMake options can be changed from
their defaults to adjust the build configuration:
```
option(COBALT_BUILD_TESTS "Specifies whether the test projects will be compiled." ON)
option(COBALT_BUILD_UTILITIES "Specifies whether the utility projects will be compiled." ON)
option(COBALT_BUILD_EXAMPLES "Specifies whether the example projects will be compiled." ON)
option(COBALT_OPENGL_USE_EGL "Specifies whether the OpenGL renderers should use EGL for Linux (supporting Xlib, XCB, and Wayland), or GLX (Xlib only)." ON)
option(COBALT_BUILD_VULKAN_VALIDATION_LAYERS "Specifies whether the Vulkan validation layers will be compiled and output to the SDK." OFF)
option(COBALT_USE_SDL3 "Specifies whether SDL3 will be used when building Cobalt. Since SDL sits above the API, this only affects example projects." ON)
option(COBALT_USE_LOCAL_SDL3 "Specifies whether SDL3 will be found locally on the system rather than downloaded and built." OFF)
option(COBALT_CLANG_FORMAT_ON_ALL "Run clang-format (check) in the default build" ON)
option(COBALT_CLANG_TIDY_ON_ALL "Run clang-tidy (check) in the default build" OFF)
option(COBALT_RUN_MSVC_STATIC_ANALYSIS "Specifies whether Visual Studio static analysis is being run during the build process." OFF)
option(COBALT_INHERIT_SYSTEM_INSTALL_PATH "Inherit the system-default install path rather than setting it internally" OFF)
```

## Compiling in Visual Studio
1. Select "File -> Open -> Folder" and open the repository
2. Select your required preset from the combobox on the toolbar
3. Select "Build -> Install CobaltRenderer"

## Compiling in Visual Studio Code
1. Enter `>CMake: Select Configure Preset` into the command palette
2. Select your required preset from the list
3. Enter `>CMake: Install` into the command palette

## Compiling from the Shell/Terminal (cmake + Ninja)
1. Type `cmake --preset <PresetName>` where "\<PresetName\>" is the name of your required preset
2. Type `cmake --build --preset <PresetName> --target install`

## Additional useful targets
| Target                      | Description |
|-----------------------------|-------------|
| `clang-tidy-check`          | Runs `clang-tidy` on all files in the project. |
| `clang-tidy-fix`            | Runs `clang-tidy` on all files in the project and attempts to automatically apply recommended fixes for reported issues. |
| `clang-format-check`        | Runs `clang-format` on all files in the project. |
| `clang-format-fix`          | Runs `clang-format` on all files in the project and attempts to automatically apply recommended fixes for reported issues. |
| `msvc-code-analysis`        | Runs Visual Studio static code analysis on the codebase. |
| `documentation`             | Builds the SDK documentation. |
| `run-all-unit-tests`        | Runs all unit tests for all renderers on all available graphics devices and produces a report. |
| `run-all-performance-tests` | Runs all performance tests for all renderers on all available graphics devices and produces a report. |
| `run-code-coverage`         | Runs code coverage analysis for the unit test suite and produces a report. |

# Contributing
This project welcomes contributions and suggestions. The sections below give some guidance on how to make contributing changes a smoother process.

### Submission process
You can contribute changes in the usual manner, by forking the repo and submitting a Pull Request. The Cobalt Renderer uses squashed merges for all changes,
to keep a clean linear commit history. This means you don't need to follow any particular format or pattern for your branch naming or commit messages,
however please try and keep your history tidy, with short and professional commit messages, as this will help the review process to go smoothly. Accepted
PRs will have a squashed commit message crafted by the project maintainers, which follows the [Conventional Commits](https://www.conventionalcommits.org)
notation, that will be used to automatically generate release notes and increment the project version.

### Limit the scope of each request
Where possible, we recommend you limit your pull requests to addressing a single bug, or introducing a single feature. This will make the review process
easier, and increase the likelihood that your submission will be accepted quickly. Combining issues into a single pull request makes merging an "all or
nothing" process, which makes it hard if only some of your changes are acceptable for merge. Also note that while the use of AI and static analysis tools
to assist in development is perfectly acceptable, you must still ensure that you properly understand your changes, and have adequately tested and reviewed
your submission, with separate issues addressed as separate pull requests, as described above. This is both to ensure the codebase maintains a high
standard, as well as to be fair on reviewers. We take pride in the quality of our codebase, and you should only submit changes you are also proud of.

### Formatting and naming conventions
The code in this repository uses a consistent standard for code structure, formatting, and naming. Where possible, those conventions are defined in files
designed to be picked up by your IDE, such as ".editorconfig", ".clang-format", and ".clang-tidy". Not all the coding conventions used in this repository
are tool checked however, so please pay attention to the existing code in the repository, and use it as an additional guide on coding style, and in
particular commenting style, which is largely not tool enforced. You should aim to make your code additions consistent with the style of the existing
codebase. Any issues with naming, formatting, or general style will be raised during the review process, and need to be resolved before a pull request
will be accepted.

### Regression testing
All pull requests will be run through a set of automated tests. These tests ensure that your changes compile on all platforms, and don't break graphics
tests on any renderers. Additionally, a suite of static analysis tools including Clang Format, Clang Tidy, and the Visual Studio Analyzer will run to check
for any issues, based on the configured rule files contained within the repository. It is recommended you run these tests yourself prior to submitting a pull
request, to avoid surprise failures which may delay acceptance of your changes.

### Pull request review
Pull requests to this repo will be reviewed by the maintainers of this repository, for guidance. Selected external contributors may also participate in the
review process. The purpose of the review process is to maintain the quality and consistency of the codebase, and to help ensure your changes are clean and
robust. This is intended to be a positive, collaborative process, and all contributions are welcome.

# License
While originally developed in-house by Maptek, this work is now released as open source under the MIT license. Please see the [LICENSE.txt](LICENSE.txt)
file in this repository for full information.
