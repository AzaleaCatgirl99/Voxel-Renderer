cmake_minimum_required(VERSION 3.30.5)
project(Voxel-Renderer
        VERSION 0.1.0.0
        DESCRIPTION "An open source voxel renderer."
        HOMEPAGE_URL "https://github.com/AzaleaCatgirl99/Voxel-Renderer"
        LANGUAGES CXX
)

# Setting languages
enable_language(CXX)
set(CMAKE_CXX_STANDARD 23)

# Source files
file(GLOB_RECURSE CPP_FILES src/*.cpp)
set(SRC ${CPP_FILES})

# Main app executable
add_executable(Voxel-Renderer ${SRC})
include_directories(src)

# SDL3 for windowing
find_package(SDL3 REQUIRED CONFIG REQUIRED COMPONENTS SDL3-shared)
target_link_libraries(Voxel-Renderer PRIVATE SDL3::SDL3)

# Vulkan pipeline
find_package(Vulkan REQUIRED)
target_link_libraries(Voxel-Renderer PRIVATE Vulkan::Vulkan)

# GLM for linear algebra
add_subdirectory(lib/glm EXCLUDE_FROM_ALL)
target_link_libraries(Voxel-Renderer PRIVATE glm::glm-header-only)
