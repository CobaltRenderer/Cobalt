# Replace long paths with hashed names. The DirectX-Graphics-Samples repo contains very long names with deep
# directory structures, and FetchContent_Populate can't exceed MAX_PATH on windows, even when support for longer
# names has been activated at the OS level. We work around it here to prevent clone failures.
set(CMAKE_OBJECT_PATH_MAX 250)

# D3DX12Residency
FetchContent_Declare(DXSamples
    GIT_REPOSITORY "https://github.com/microsoft/DirectX-Graphics-Samples.git"
    GIT_TAG        "1704d8883ea74a73d332516bb6d990a134abaebd"
    SOURCE_DIR     "${COBALT_EXTERNAL_ROOT}/Cache/DXSamples"
    PATCH_COMMAND  git apply "${CMAKE_CURRENT_SOURCE_DIR}/D3DX12Residency.patch"
    UPDATE_DISCONNECTED TRUE
)
FetchContent_Populate(DXSamples)
add_library(D3DX12Residency INTERFACE)
target_include_directories(D3DX12Residency SYSTEM INTERFACE "${COBALT_EXTERNAL_ROOT}/Cache/DXSamples/Libraries/D3DX12Residency")

# DirectX-Headers
FetchContent_Declare(DirectX-Headers
    GIT_REPOSITORY "https://github.com/microsoft/DirectX-Headers"
    GIT_TAG        "v1.606.3-r1"
    SOURCE_DIR     "${COBALT_EXTERNAL_ROOT}/Cache/DirectX-Headers"
)
FetchContent_MakeAvailable(DirectX-Headers)
set_project_virtual_folder(DirectX-Guids "External/DirectX-Headers")
add_library(Direct3D12Bundle INTERFACE)
target_link_libraries(Direct3D12Bundle INTERFACE
    DirectX-Headers
    DirectX-Guids
)

# D3D12MemoryAllocator
FetchContent_Declare(D3D12MemoryAllocator
    GIT_REPOSITORY "https://github.com/GPUOpen-LibrariesAndSDKs/D3D12MemoryAllocator.git"
    GIT_TAG        "eace86c42911c8812841043d8f34c7574acee7da"
    SOURCE_DIR     "${COBALT_EXTERNAL_ROOT}/Cache/D3D12MemoryAllocator"
)
FetchContent_Populate(D3D12MemoryAllocator)
add_library(D3D12MemoryAllocator INTERFACE)
target_include_directories(D3D12MemoryAllocator SYSTEM INTERFACE
    "${COBALT_EXTERNAL_ROOT}/Cache/D3D12MemoryAllocator/include"
    "${COBALT_EXTERNAL_ROOT}/Cache/D3D12MemoryAllocator/src"
)

# WinPixEventRuntime
add_subdirectory(PixEvents)

# DirectXShaderCompiler
FetchContent_Declare(DirectXShaderCompilerOfficial
    URL https://github.com/microsoft/DirectXShaderCompiler/releases/download/v1.9.2602/dxc_2026_02_20.zip
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    SOURCE_DIR     "${COBALT_EXTERNAL_ROOT}/Cache/DirectXShaderCompilerOfficial"
)
FetchContent_MakeAvailable(DirectXShaderCompilerOfficial)
add_library(DirectXShaderCompilerOfficial::dxcompiler UNKNOWN IMPORTED GLOBAL)
set_target_properties(DirectXShaderCompilerOfficial::dxcompiler PROPERTIES IMPORTED_LOCATION "${directxshadercompilerofficial_SOURCE_DIR}/bin/${COBALT_ARCH}/dxcompiler.dll")
add_library(DirectXShaderCompilerOfficial::dxil UNKNOWN IMPORTED GLOBAL)
set_target_properties(DirectXShaderCompilerOfficial::dxil PROPERTIES IMPORTED_LOCATION "${directxshadercompilerofficial_SOURCE_DIR}/bin/${COBALT_ARCH}/dxil.dll")
