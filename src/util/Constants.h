#pragma once

// Various different rendering pipelines available.
enum eRenderPipeline {
    RENDER_PIPELINE_VULKAN
};

// Various different rendering swap intervals.
// OpenGL - https://wikis.khronos.org/opengl/Swap_Interval
// Vulkan - https://docs.vulkan.org/refpages/latest/refpages/source/VkPresentModeKHR.html
// ADAPTIVE_SYNC does not exist on Vulkan and TRIPLE_BUFFERING does not exist on OpenGL.
enum eRenderSwapInterval {
    RENDER_SWAP_INTERVAL_IMMEDIATE,
    RENDER_SWAP_INTERVAL_VSYNC,
    RENDER_SWAP_INTERVAL_ADAPTIVE_SYNC,
    RENDER_SWAP_INTERVAL_TRIPLE_BUFFERING
};
