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

add_compile_options(-march=native)

# Default settings for the program
set(VXL_VERBOSE_LOGGING ON)
set(VXL_INFO_LOGGING ON)
set(VXL_WARNING_LOGGING ON)
set(VXL_ERROR_LOGGING ON)
set(VXL_PROJECT_NAME "Voxel Renderer")

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

add_compile_definitions(VXL_PROJECT_NAME="Voxel Renderer")
# A simple macro for inlining with constexpr
add_compile_definitions("VXL_INLINE=inline constexpr")

# ========== Source Files & Executable ==========

# Main app source
file(GLOB_RECURSE CPP_FILES src/*.cpp)
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
add_executable(Voxel-Renderer ${SRC} ${IMGUI_SRC})
include_directories(src)
include_directories(lib/imgui)

# Custom definitions based on specific CMake settings
target_compile_definitions(Voxel-Renderer PUBLIC
    # Build type
    $<$<CONFIG:Debug>:VXL_DEBUG=1>
    $<$<CONFIG:RelWithDebInfo>:VXL_DEBUG=1>
    $<$<CONFIG:Release>:VXL_RELEASE=1>
    $<$<CONFIG:MinSizeRel>:VXL_RELEASE=1>

    # Debugging
    $<$<CONFIG:Debug>:VXL_RENDERSYSTEM_DEBUG=1>
    $<$<CONFIG:RelWithDebInfo>:VXL_RENDERSYSTEM_DEBUG=1>
)

# ========== Libraries ==========

# SDL3 for windowing
find_package(SDL3 REQUIRED CONFIG REQUIRED COMPONENTS SDL3-shared)
target_link_libraries(Voxel-Renderer PRIVATE SDL3::SDL3)

# Vulkan pipeline
find_package(Vulkan REQUIRED COMPONENTS glslc)
target_link_libraries(Voxel-Renderer PRIVATE Vulkan::Vulkan)

# GLM for linear algebra
add_subdirectory(lib/glm EXCLUDE_FROM_ALL)
target_link_libraries(Voxel-Renderer PRIVATE glm::glm-header-only)

# Highway for SIMD
add_subdirectory(lib/highway EXCLUDE_FROM_ALL)
target_link_libraries(Voxel-Renderer PRIVATE hwy)

# STB for imaging
target_include_directories(Voxel-Renderer PRIVATE lib/stb)

# ========== Assets ==========

# Deletes the assets dir in the build dir if it exists
if (EXISTS "${PROJECT_BINARY_DIR}/assets")
add_custom_command(TARGET Voxel-Renderer POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E remove_directory "${PROJECT_BINARY_DIR}/assets"
)
endif ()

# Moves the assets dir to the build dir at post-build
add_custom_command(TARGET Voxel-Renderer POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "${PROJECT_BINARY_DIR}/assets"
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/assets "${PROJECT_BINARY_DIR}/assets"
)

# Compile vertex shaders to Vulkan's binary format
file(GLOB_RECURSE VERT_SHADER_FILES ${PROJECT_BINARY_DIR}/assets/shaders/*.vert)
foreach (vert_file ${VERT_SHADER_FILES})
    set(VERT_COMPILED_FILE ${vert_file})
    string(REPLACE ".vert" "_vert.spv" VERT_COMPILED_FILE ${VERT_COMPILED_FILE})
    add_custom_command(TARGET Voxel-Renderer POST_BUILD
        COMMAND ${Vulkan_GLSLC_EXECUTABLE} ${vert_file} -o ${VERT_COMPILED_FILE}
    )
endforeach ()

# Compile fragment shaders to Vulkan's binary format
file(GLOB_RECURSE FRAG_SHADER_FILES ${PROJECT_BINARY_DIR}/assets/shaders/*.frag)
foreach (frag_file ${FRAG_SHADER_FILES})
    set(FRAG_COMPILED_FILE ${frag_file})
    string(REPLACE ".frag" "_frag.spv" FRAG_COMPILED_FILE ${FRAG_COMPILED_FILE})
    add_custom_command(TARGET Voxel-Renderer POST_BUILD
        COMMAND ${Vulkan_GLSLC_EXECUTABLE} ${frag_file} -o ${FRAG_COMPILED_FILE}
    )
endforeach ()
