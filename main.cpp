#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <stdio.h>
#include <logger.h>
#include "helper.h"

VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objType, uint64_t obj, size_t location, int32_t code, const char* layerPrefix, const char* msg, void* userData) {
    logerror("validation layer:{}", msg);
    
    return VK_FALSE;
}

int SetupValidationLayerCallback(VkInstance instance, PFN_vkDebugReportCallbackEXT _pfnCallback, VkDebugReportCallbackEXT *h)
{
    VkDebugReportCallbackCreateInfoEXT createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
    createInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;
    createInfo.pfnCallback = _pfnCallback;
    
    auto func = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugReportCallbackEXT");
    if (func != nullptr) {
        return func(instance, &createInfo, nullptr, h);
    }
    return VK_ERROR_EXTENSION_NOT_PRESENT;
}


int main(int argc, char **argv){
    logger_init_file_output("vulkan.log");
    
    
#ifdef __APPLE__
    const std::vector<const char*> validationLayers = { "MoltenVK" };
#else
    printf("not on mac\n");
    const std::vector<const char*> validationLayers = { "VK_LAYER_LUNARG_standard_validation" };
#endif
    if (!CheckInstanceLayerPropertiesSupport(validationLayers)) {
        logerror("checkInstanceLayerPropertiesSupport fail");
        return -1;
    }

    GetInstanceExtensionProperties();
    
    VkInstance instance = CreateInstance(validationLayers);
    
    std::unique_ptr<std::vector<VkPhysicalDevice>> devices = GetPhysicalDevices(instance);
    for (int i = 0; i < devices->size(); i++){
        VkPhysicalDevice device = devices->operator[](i);
        GetPhysicalDeviceProperties(device);
        GetPhysicalDeviceExtensionProperties(device);
        GetPhysicalDeviceQueueFamilyProperties(device);
        GetPhysicalDeviceMemoryProperties(device);
    }

    VkDebugReportCallbackEXT hDebugCallback;
    SetupValidationLayerCallback(instance, debugCallback, &hDebugCallback);
}
