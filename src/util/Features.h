#pragma once

// This is a list of features to enable and disable, alongside some debugging utilities.

// True-false statements.
#define VXL_TRUE true
#define VXL_FALSE false

// Main debug macro.
#if !defined (NDEBUG) || defined (INTERNAL_VXL_FORCE_USE_DEBUG_SETTINGS)
#define VXL_DEBUG
#endif

// Debug settings.
#ifdef VXL_DEBUG

// Renderer logging settings.

#ifdef INTERNAL_VXL_RENDERER_VERBOSE_LOG
#define VXL_RENDERER_VERBOSE_LOG VXL_TRUE
#else
#define VXL_RENDERER_VERBOSE_LOG VXL_FALSE
#endif

#ifdef INTERNAL_VXL_RENDERER_INFO_LOG
#define VXL_RENDERER_INFO_LOG VXL_TRUE
#else
#define VXL_RENDERER_INFO_LOG VXL_FALSE
#endif

#ifdef INTERNAL_VXL_RENDERER_WARNING_LOG
#define VXL_RENDERER_WARNING_LOG VXL_TRUE
#else
#define VXL_RENDERER_WARNING_LOG VXL_FALSE
#endif

#ifdef INTERNAL_VXL_RENDERER_ERROR_LOG
#define VXL_RENDERER_ERROR_LOG VXL_TRUE
#else
#define VXL_RENDERER_ERROR_LOG VXL_FALSE
#endif

#endif
