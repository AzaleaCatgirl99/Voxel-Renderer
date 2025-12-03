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

// Various different rendering states.
enum eRenderState {
    RENDER_STATE_VIEWPORT,
    RENDER_STATE_SCISSOR,
    RENDER_STATE_LINE_WIDTH,
    RENDER_STATE_BLEND,
    RENDER_STATE_CULL_MODE
};

// Various available vertex modes to use in the renderer.
// RENDER_VERTEX_MODE_POINT_LIST - points from vertices.
// RENDER_VERTEX_MODE_LINE_LIST - line from every 2 vertices without reuse.
// RENDER_VERTEX_MODE_LINE_STRIP - the end vertex of every line is used as start vertex for the next line.
// RENDER_VERTEX_MODE_TRIANGLE_LIST - triangle from every 3 vertices without reuse.
// RENDER_VERTEX_MODE_TRIANGLE_STRIP - the second and third vertex of every triangle are used as first two vertices of the next triangle.
enum eRenderVertexMode {
    RENDER_VERTEX_MODE_POINT_LIST,
    RENDER_VERTEX_MODE_LINE_LIST,
    RENDER_VERTEX_MODE_LINE_STRIP,
    RENDER_VERTEX_MODE_TRIANGLE_LIST,
    RENDER_VERTEX_MODE_TRIANGLE_STRIP
};

// Various available polygon modes to use in the renderer.
// RENDER_POLYGON_MODE_FILL - fill the area of the polygon with fragments.
// RENDER_POLYGON_MODE_LINE - polygon edges are drawn as lines.
// RENDER_POLYGON_MODE_POINT - polygon vertices are drawn as points.
enum eRenderPolygonMode {
    RENDER_POLYGON_MODE_FILL,
    RENDER_POLYGON_MODE_LINE,
    RENDER_POLYGON_MODE_POINT
};

// Various blending factors for rendering.
enum eRenderBlendFactor {
    RENDER_BLEND_FACTOR_ZERO,
    RENDER_BLEND_FACTOR_ONE,
    RENDER_BLEND_FACTOR_SRC_COLOR,
    RENDER_BLEND_FACTOR_ONE_MINUS_SRC_COLOR,
    RENDER_BLEND_FACTOR_DST_COLOR,
    RENDER_BLEND_FACTOR_ONE_MINUS_DST_COLOR,
    RENDER_BLEND_FACTOR_SRC_ALPHA,
    RENDER_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
    RENDER_BLEND_FACTOR_DST_ALPHA,
    RENDER_BLEND_FACTOR_ONE_MINUS_DST_ALPHA,
    RENDER_BLEND_FACTOR_CONST_COLOR,
    RENDER_BLEND_FACTOR_ONE_MINUS_CONST_COLOR,
    RENDER_BLEND_FACTOR_CONST_ALPHA,
    RENDER_BLEND_FACTOR_ONE_MINUS_CONST_ALPHA,
    RENDER_BLEND_FACTOR_SRC_ALPHA_SATURATE,
    RENDER_BLEND_FACTOR_SRC1_COLOR,
    RENDER_BLEND_FACTOR_ONE_MINUS_SRC1_COLOR,
    RENDER_BLEND_FACTOR_SRC1_ALPHA,
    RENDER_BLEND_FACTOR_ONE_MINUS_SRC1_ALPHA
};

// Various cull modes for rendering.
enum eRenderCullMode {
    RENDER_CULL_MODE_NONE,
    RENDER_CULL_MODE_FRONT,
    RENDER_CULL_MODE_BACK,
    RENDER_CULL_MODE_FRONT_AND_BACK
};

enum eRenderUniformType {
    RENDER_UNIFORM_TYPE_UNIFORM,
    RENDER_UNIFORM_TYPE_TEXEL
};

enum eRenderShaderStage {
    RENDER_SHADER_STAGE_VERTEX = 0x00000001,
    RENDER_SHADER_STAGE_FRAGMENT = 0x00000010,
    RENDER_SHADER_STAGE_ALL = 0x0000001F
};
