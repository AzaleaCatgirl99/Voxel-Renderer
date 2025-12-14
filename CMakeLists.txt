cmake_minimum_required(VERSION 4.0)
project(Voxel-Renderer
        VERSION 0.1.0.0
        DESCRIPTION "An open source voxel renderer."
        HOMEPAGE_URL "https://github.com/AzaleaCatgirl99/Voxel-Renderer"
        LANGUAGES CXX
)

# ========== Configuration ==========

# Setting languages
enable_language(CXX)
set(CMAKE_CXX_STANDARD 23)

# Compile option needed for Highway SIMD
add_compile_options(-march=native)

# Main project name
set(VXL_TARGET_NAME Voxel-Renderer)

# Default settings for the program
set(VXL_VERBOSE_LOGGING ON)
set(VXL_INFO_LOGGING ON)
set(VXL_WARNING_LOGGING ON)
set(VXL_ERROR_LOGGING ON)
set(VXL_TEST OFF)

# Directories used by the program
set(VXL_TOOLS_DIR ${CMAKE_SOURCE_DIR}/tools)
set(VXL_ASSETS_DIR ${CMAKE_SOURCE_DIR}/assets)
set(VXL_SHADERS_DIR ${CMAKE_SOURCE_DIR}/assets/shaders)
set(VXL_SOURCE_DIR ${CMAKE_SOURCE_DIR}/src)
set(VXL_BUILD_ASSETS_DIR ${PROJECT_BINARY_DIR}/assets)
set(VXL_BUILD_SHADERS_DIR ${PROJECT_BINARY_DIR}/assets/shaders)

# Custom config settings to enable
if (EXISTS config/config.cmake)
    include(config/config.cmake)
endif ()

# Custom macros used
if (VXL_VERBOSE_LOGGING)
    add_compile_definitions(VXL_VERBOSE_LOGGING=1)
endif ()

if (VXL_INFO_LOGGING)
    add_compile_definitions(VXL_INFO_LOGGING=1)
endif ()

if (VXL_WARNING_LOGGING)
    add_compile_definitions(VXL_WARNING_LOGGING=1)
endif ()

if (VXL_ERROR_LOGGING)
    add_compile_definitions(VXL_ERROR_LOGGING=1)
endif ()

if (VXL_TEST)
    add_compile_definitions(VXL_TEST=1)
endif ()

add_compile_definitions(VXL_PROJECT_NAME="Voxel Renderer")
# A simple macro for inlining with constexpr
add_compile_definitions("VXL_INLINE=inline constexpr")

# ========== Source Files & Executable ==========

# Main app source
file(GLOB_RECURSE CPP_FILES ${VXL_SOURCE_DIR}/*.cpp)
set(SRC ${CPP_FILES})

# ImGUI for UI handling (this doesn't have a CMake config, so we have to add our own config)
set(IMGUI_SRC
    # Main required files
    lib/imgui/imgui.cpp
    lib/imgui/imgui_demo.cpp
    lib/imgui/imgui_draw.cpp
    lib/imgui/imgui_tables.cpp
    lib/imgui/imgui_widgets.cpp
    # Backend files
    lib/imgui/backends/imgui_impl_sdl3.cpp
    lib/imgui/backends/imgui_impl_vulkan.cpp
)

# Main app executable
add_executable(${VXL_TARGET_NAME} ${SRC} ${IMGUI_SRC})
include_directories(src)
include_directories(lib/imgui)

# Custom definitions based on specific CMake settings
target_compile_definitions(${VXL_TARGET_NAME} PRIVATE
    # Build type
    $<$<CONFIG:Debug>:VXL_DEBUG=1>
    $<$<CONFIG:RelWithDebInfo>:VXL_DEBUG=1>
    $<$<CONFIG:Release>:VXL_RELEASE=1>
    $<$<CONFIG:MinSizeRel>:VXL_RELEASE=1>
)

# ========== Libraries ==========

# SDL3 for windowing
find_package(SDL3 REQUIRED CONFIG REQUIRED COMPONENTS SDL3-shared)
target_link_libraries(${VXL_TARGET_NAME} PRIVATE SDL3::SDL3)

# Vulkan pipeline
find_package(Vulkan REQUIRED)
target_link_libraries(${VXL_TARGET_NAME} PRIVATE Vulkan::Vulkan)

# GLM for linear algebra
add_subdirectory(lib/glm EXCLUDE_FROM_ALL)
target_link_libraries(${VXL_TARGET_NAME} PRIVATE glm::glm-header-only)

# Highway for SIMD
add_subdirectory(lib/highway EXCLUDE_FROM_ALL)
target_link_libraries(${VXL_TARGET_NAME} PRIVATE hwy)

# STB for imaging
target_include_directories(${VXL_TARGET_NAME} PRIVATE lib/stb)

# ========== Assets ==========

# Copies the assets folder from the root project directory to the build directory
add_custom_target(CopyAssets
    COMMAND ${CMAKE_COMMAND} -P ${VXL_TOOLS_DIR}/assets.cmake
)
add_dependencies(${VXL_TARGET_NAME} CopyAssets)

# Compile shaders to Vulkan's binary format (SPIRV-V)
file(GLOB_RECURSE SHADER_FILES ${VXL_SHADERS_DIR}/*.slang)
foreach (SHADER ${SHADER_FILES})
    # Gets the file name needed for printing the comment on compilation
    file(RELATIVE_PATH SHADER_NAME ${VXL_SHADERS_DIR} ${SHADER})

    # Gets the file path for the compiled shader
    string(REPLACE ".slang" ".spv" COMPILED_SHADER ${SHADER_NAME})
    set(COMPILED_SHADER ${VXL_BUILD_SHADERS_DIR}/${COMPILED_SHADER})

    add_custom_command(TARGET ${VXL_TARGET_NAME} POST_BUILD
        COMMAND slangc ${SHADER} -target spirv -emit-spirv-via-glsl -emit-spirv-directly -o ${COMPILED_SHADER}
        COMMAND ${CMAKE_COMMAND} -E echo "-- Compiling Slang shader \'${SHADER_NAME}\'"
    )
endforeach ()
