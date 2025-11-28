cmake_minimum_required(VERSION 3.30.5)
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

# Default settings for the program
set(VOXEL_RENDERER_VERBOSE_LOG OFF)
set(VOXEL_RENDERER_INFO_LOG OFF)
set(VOXEL_RENDERER_WARNING_LOG ON)
set(VOXEL_RENDERER_ERROR_LOG ON)
set(VOXEL_FORCE_USE_DEBUG_SETTINGS OFF)

# Custom config settings to enable
if (EXISTS config/config.cmake)
    include(config/config.cmake)
endif ()

# Custom macros used
if (VOXEL_RENDERER_VERBOSE_LOG)
    add_compile_definitions(INTERNAL_VXL_RENDERER_VERBOSE_LOG=1)
endif ()

if (VOXEL_RENDERER_INFO_LOG)
    add_compile_definitions(INTERNAL_VXL_RENDERER_INFO_LOG=1)
endif ()

if (VOXEL_RENDERER_WARNING_LOG)
    add_compile_definitions(INTERNAL_VXL_RENDERER_WARNING_LOG=1)
endif ()

if (VOXEL_RENDERER_ERROR_LOG)
    add_compile_definitions(INTERNAL_VXL_RENDERER_ERROR_LOG=1)
endif ()

if (VOXEL_FORCE_USE_DEBUG_SETTINGS)
    add_compile_definitions(INTERNAL_VXL_FORCE_USE_DEBUG_SETTINGS=1)
endif ()

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
