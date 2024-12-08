# Handle library creation here.

set(ENGINE_RUNTIME_SOURCE_DIR ${ENGINE_SOURCE_DIR}/Runtime)
set(ENGINE_RUNTIME Runtime)

# Do some OS checks, and setup accordingly.
set(APPLE_PLATFORM OFF)
set(LINUX OFF)
set(WINDOWS OFF)

# On macOS, we always set Metal support to on.
set(METAL OFF)

# Make our APIs into options
option(DX12 "DirectX12 (Windows only)" OFF)
option(DX11 "DirectX11 (Windows only)" OFF)
option(EXAMPLES "The Forge examples" OFF)
option(VULKAN "Vulkan" OFF)
option(DYNAMIC_LIB "Dynamic Library" OFF)

set(ASSIMP OFF)
set(OZZ OFF)

if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    message("Apple platform detected. Generating macOS and iOS targets.")
    set(APPLE_PLATFORM ON)
endif()

if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    message("Linux detected. Generating Linux targets.")
    set(LINUX ON)
    set(VULKAN ON)
endif()

if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    message("Windows detected. Generating Windows targets.")
    set(WINDOWS ON)
endif()

# Setup some sane API defaults.
set(API_SELECTED ON)

if(${VULKAN} MATCHES OFF)
    if(${DX11} MATCHES OFF)
        if(${DX12} MATCHES OFF)
            if(${METAL} MATCHES OFF)
                set(API_SELECTED OFF)
            endif()
        endif()
    endif()
endif()

# Apple platforms should always default to Metal.
if(${APPLE_PLATFORM} MATCHES ON)
    if(${METAL} MATCHES OFF)
        set(METAL ON)
    endif()

    if(${VULKAN} MATCHES ON)
        set(VULKAN OFF)
    endif()

    if(${DX11} MATCHES ON)
        set(DX11 OFF)
    endif()

    if(${DX12} MATCHES ON)
        set(DX12 OFF)
    endif()

    set(API_SELECTED ON)
endif()

if(${API_SELECTED} MATCHES OFF)
    if(${APPLE_PLATFORM} MATCHES ON)
        set(METAL ON)
    endif()

    if(${LINUX} MATCHES ON)
        set(VULKAN ON)
    endif()

    if(${WINDOWS} MATCHES ON)
        set(DX12 ON)

        # Needed due to how the profiler in forge force-links to vulkan as well.
        set(VULKAN ON)
    endif()
endif()

#set(DX12 OFF)
set(DX11 OFF)

if(${DX12} MATCHES ON)
    add_compile_definitions(ENABLE_DX12)
endif()

if(${DX11} MATCHES ON)
    add_compile_definitions(ENABLE_DX11)
endif()

if(${VULKAN} MATCHES ON)
    add_compile_definitions(ENABLE_VULKAN)
endif()


message("\n")

include(Platform)
include(Core)
include(RHI)


source_group(TREE ${CORE_INTERFACE_DIR} PREFIX "Header Files" FILES ${CORE_INTERFACE_FILES})
source_group(TREE ${CORE_SOURCE_DIR} PREFIX "Source Files\\Core" FILES ${CORE_INCLUDE_FILES} ${CORE_SOURCE_FILES})

source_group(TREE ${PLATFORM_INTERFACE_DIR} PREFIX "Header Files" FILES ${PLATFORM_INTERFACE_FILES})
source_group(TREE ${PLATFORM_SOURCE_DIR} PREFIX "Source Files\\Platform" FILES ${PLATFORM_INCLUDE_FILES} ${PLATFORM_SOURCE_FILES})

source_group(TREE ${RHI_INTERFACE_DIR} PREFIX "Header Files" FILES ${RHI_INTERFACE_FILES})
source_group(TREE ${RHI_SOURCE_DIR} PREFIX "Source Files\\RHI" FILES ${RHI_INCLUDE_FILES} ${RHI_SOURCE_FILES})


set(RUNTIME_INTERFACE_FILES
    ${CORE_INTERFACE_FILES}
    ${PLATFORM_INTERFACE_FILES}
    ${RHI_INTERFACE_FILES}

)

set(RUNTIME_SOURCE_FILES
    ${CORE_INCLUDE_FILES}
    ${CORE_SOURCE_FILES}
    ${PLATFORM_INCLUDE_FILES}
    ${PLATFORM_SOURCE_FILES}
    ${RHI_INCLUDE_FILES}
    ${RHI_SOURCE_FILES}
)

if(${DYNAMIC_LIB} MATCHES OFF)
    add_library(${ENGINE_RUNTIME} STATIC
        ${RUNTIME_SOURCE_FILES}
        ${RUNTIME_INTERFACE_FILES}
    )

else()
    add_library(${ENGINE_RUNTIME} SHARED
        ${RUNTIME_SOURCE_FILES}
        ${RUNTIME_INTERFACE_FILES}
    )
endif()

target_include_directories(${ENGINE_RUNTIME} PUBLIC
    ${ENGINE_SOURCE_DIR}
    ${ENGINE_RUNTIME_SOURCE_DIR}/Platform/Public
    ${ENGINE_RUNTIME_SOURCE_DIR}/Core/Public
    ${ENGINE_RUNTIME_SOURCE_DIR}/RHI/Public
    ${ENGINE_RUNTIME_SOURCE_DIR}/Renderer/Public
    ${ENGINE_RUNTIME_SOURCE_DIR}/Resource/Public
    ${ENGINE_RUNTIME_SOURCE_DIR}/Application/Public
    ${ENGINE_RUNTIME_SOURCE_DIR}/Scripting/Public
    ${THIRD_PARTY_INCLUDES}
)

# https://cmake.org/cmake/help/latest/command/target_precompile_headers.html
target_precompile_headers(${ENGINE_RUNTIME} INTERFACE
    #$<$<COMPILE_LANGUAGE:CXX>:${RUNTIME_INTERFACE_FILES}>
    ${RUNTIME_INTERFACE_FILES}>
)

if(Vulkan_FOUND MATCHES TRUE)
    target_include_directories(${ENGINE_RUNTIME} PUBLIC ${Vulkan_INCLUDE_DIRS})
endif()

target_link_libraries(${ENGINE_RUNTIME} PUBLIC ${RHI_LIBRARIES} ${THIRD_PARTY_DEPS})

target_link_directories(${ENGINE_RUNTIME} PUBLIC ${RHI_LIBRARY_PATHS})

target_compile_definitions(${ENGINE_RUNTIME} PUBLIC ${RHI_DEFINES})

# unity build
set_target_properties(${ENGINE_RUNTIME} PROPERTIES UNITY_BUILD ON)

target_compile_features(${ENGINE_RUNTIME} PRIVATE cxx_std_20)

if (${APPLE_PLATFORM} MATCHES ON)
    set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -std=c++17 -stdlib=libc++ -x objective-c++")
    target_compile_options(${ENGINE_RUNTIME} PRIVATE "-fobjc-arc")
endif()

# Add compiler-specific flags
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_compile_options(${ENGINE_RUNTIME} PRIVATE -fno-rtti -fno-exceptions)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    target_compile_options(${ENGINE_RUNTIME} PRIVATE /GR- /EHs-c-)
endif()


