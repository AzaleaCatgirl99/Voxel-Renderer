#include "renderer/detail/vulkan/VkResultHandler.h"

#include <string>

// Fixes missing macros for Apple devices.
#ifdef SDL_PLATFORM_APPLE
#define VK_ERROR_VALIDATION_FAILED VK_ERROR_VALIDATION_FAILED_EXT
#define VK_ERROR_NOT_PERMITTED VK_ERROR_NOT_PERMITTED_EXT
#endif

namespace detail {

Logger VkResultHandler::sLogger = Logger("VkResultHandler");

void VkResultHandler::CheckResult(const VkResult result, const char* error, std::optional<const char*> success) {
    if (result == VK_SUCCESS) {
        if (success.has_value())
            sLogger.Info(success.value());
        return;
    }

    if (result >= 0) {
        if (success.has_value())
            sLogger.Warning(success.value(), " BUT: ", ProcessResultDescription(result)); // Warn for other cases.
        return;
    }

    throw sLogger.RuntimeError(error, " ", ProcessResultDescription(result));
}

const char* VkResultHandler::GetResultDescription(const VkResult result) {
    // https://docs.vulkan.org/refpages/latest/refpages/source/VkResult.html
    switch (result) {
    case VK_SUCCESS:
        return "";
    case VK_NOT_READY:
        return "A fence or query has not yet completed.";
    case VK_TIMEOUT:
        return "A wait operation has not completed in the specified time.";
    case VK_EVENT_SET:
        return "An event is signaled.";
    case VK_EVENT_RESET:
        return "An event is unsignaled.";
    case VK_INCOMPLETE:
        return "A return array was too small for the result.";
    case VK_ERROR_OUT_OF_HOST_MEMORY:
        return "A host memory allocation has failed.";
    case VK_ERROR_OUT_OF_DEVICE_MEMORY:
        return "A device memory allocation has failed.";
    case VK_ERROR_INITIALIZATION_FAILED:
        return "Initialization of an object could not be completed for implementation-specific reasons.";
    case VK_ERROR_DEVICE_LOST:
        return "The logical or physical device has been lost. See Lost Device.";
    case VK_ERROR_MEMORY_MAP_FAILED:
        return "Mapping of a memory object has failed.";
    case VK_ERROR_LAYER_NOT_PRESENT:
        return "A requested layer is not present or could not be loaded.";
    case VK_ERROR_EXTENSION_NOT_PRESENT:
        return "A requested extension is not supported.";
    case VK_ERROR_FEATURE_NOT_PRESENT:
        return "A requested feature is not supported.";
    case VK_ERROR_INCOMPATIBLE_DRIVER:
        return "The requested version of Vulkan is not supported by the driver or is otherwise incompatible for implementation-specific reasons.";
    case VK_ERROR_TOO_MANY_OBJECTS:
        return "Too many objects of the type have already been created.";
    case VK_ERROR_FORMAT_NOT_SUPPORTED:
        return "A requested format is not supported on this device.";
    case VK_ERROR_FRAGMENTED_POOL:
        return "A pool allocation has failed due to fragmentation of the pool's memory.";
    case VK_ERROR_UNKNOWN:
        return "An unknown error has occurred; either the application has provided invalid input, or an implementation failure has occurred.";
    case VK_ERROR_OUT_OF_POOL_MEMORY:
        return "A pool memory allocation has failed.";
    case VK_ERROR_INVALID_EXTERNAL_HANDLE:
        return "An external handle is not a valid handle of the specified type.";
    case VK_ERROR_FRAGMENTATION:
        return "A descriptor pool creation has failed due to fragmentation.";
    case VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS:
        return "A buffer creation or memory allocation failed because the requested address is not available. A shader group handle assignment failed because the requested shader group handle information is no longer valid.";
    case VK_PIPELINE_COMPILE_REQUIRED:
        return "A requested pipeline creation would have required compilation, but the application requested compilation to not be performed.";
    case VK_ERROR_SURFACE_LOST_KHR:
        return "A surface is no longer available.";
    case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR:
        return "The requested window is already in use by Vulkan or another API in a manner which prevents it from being used again.";
    case VK_SUBOPTIMAL_KHR:
        return "A swapchain no longer matches the surface properties exactly, but can still be used to present to the surface successfully.";
    case VK_ERROR_OUT_OF_DATE_KHR:
        return "A surface has changed in such a way that it is no longer compatible with the swapchain, and further presentation requests using the swapchain will fail. Applications must query the new surface properties and recreate their swapchain if they wish to continue presenting to the surface.";
    case VK_ERROR_INCOMPATIBLE_DISPLAY_KHR:
        return "The display used by a swapchain does not use the same presentable image layout, or is incompatible in a way that prevents sharing an image.";
    case VK_ERROR_VALIDATION_FAILED:
        return "A command failed because invalid usage was detected by the implementation or a validation layer. This may result in the command not being dispatched to the ICD.";
    case VK_ERROR_INVALID_SHADER_NV:
        return "One or more shaders failed to compile or link. More details are reported back to the application via VK_EXT_debug_report if enabled.";
    case VK_ERROR_IMAGE_USAGE_NOT_SUPPORTED_KHR:
        return "The requested VkImageUsageFlags are not supported.";
    case VK_ERROR_VIDEO_PICTURE_LAYOUT_NOT_SUPPORTED_KHR:
        return "The requested video picture layout is not supported.";
    case VK_ERROR_VIDEO_PROFILE_OPERATION_NOT_SUPPORTED_KHR:
        return "A video profile operation specified via VkVideoProfileInfoKHR:videoCodecOperation is not supported.";
    case VK_ERROR_VIDEO_PROFILE_FORMAT_NOT_SUPPORTED_KHR:
        return "Format parameters in a requested VkVideoProfileInfoKHR chain are not supported.";
    case VK_ERROR_VIDEO_PROFILE_CODEC_NOT_SUPPORTED_KHR:
        return "Codec-specific parameters in a requested VkVideoProfileInfoKHR chain are not supported.";
    case VK_ERROR_VIDEO_STD_VERSION_NOT_SUPPORTED_KHR:
        return "The specified video Std header version is not supported.";
    case VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT:
        return "Invalid DRM format modifier plane layout.";
    case VK_ERROR_NOT_PERMITTED:
        return "The driver implementation has denied a request to acquire a priority above the default priority (VK_QUEUE_GLOBAL_PRIORITY_MEDIUM_EXT) because the application does not have sufficient privileges.";
    case VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT:
        return "An operation on a swapchain created with VK_FULL_SCREEN_EXCLUSIVE_APPLICATION_CONTROLLED_EXT failed as it did not have exclusive full-screen access. This may occur due to implementation-dependent reasons, outside of the application's control.";
    case VK_THREAD_IDLE_KHR:
        return "A deferred operation is not complete but there is currently no work for this thread to do at the time of this call.";
    case VK_THREAD_DONE_KHR:
        return "A deferred operation is not complete but there is no work remaining to assign to additional threads.";
    case VK_OPERATION_DEFERRED_KHR:
        return "A deferred operation was requested and at least some of the work was deferred.";
    case VK_OPERATION_NOT_DEFERRED_KHR:
        return "A deferred operation was requested and no operations were deferred.";
    case VK_ERROR_INVALID_VIDEO_STD_PARAMETERS_KHR:
        return "The specified Video Std parameters do not adhere to the syntactic or semantic requirements of the used video compression standard, or values derived from parameters according to the rules defined by the used video compression standard do not adhere to the capabilities of the video compression standard or the implementation.";
    case VK_ERROR_COMPRESSION_EXHAUSTED_EXT:
        return "An image creation failed because internal resources required for compression are exhausted. This must only be returned when fixed-rate compression is requested.";
    case VK_INCOMPATIBLE_SHADER_BINARY_EXT:
        return "The provided binary shader code is not compatible with this device.";
#ifndef SDL_PLATFORM_APPLE
    case VK_ERROR_NOT_ENOUGH_SPACE_KHR:
        return "The application did not provide enough space to return all the required data.";
    case VK_PIPELINE_BINARY_MISSING_KHR:
        return "The application attempted to create a pipeline binary by querying an internal cache, but the internal cache entry did not exist.";
#endif
    case VK_RESULT_MAX_ENUM:
        return "";
    }
}

std::string VkResultHandler::ProcessResultDescription(const VkResult result) {
    std::string description = GetResultDescription(result);
    if (description != "")
        return description;

    // If the flat_map does not hold the result, then output the code in integer form.
    return "Result code '" + std::to_string(result) + "'";
};

}
