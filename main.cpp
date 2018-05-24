#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <stdio.h>
#include <logger.h>
#include "helper.h"



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
}
