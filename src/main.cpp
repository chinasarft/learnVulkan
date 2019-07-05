#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>


#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <iostream>
#include "onscreen.h"
#include <fstream>
#include <chrono>
#include "logger.h"

//#define PERF_TEST

const std::vector<Vertex> vertex_data = {
    {{-0.7f, -0.7f}, { -0.1f, -0.1f}},
    {{-0.7f, 0.7f}, {-0.1f, 1.1f}},
    {{0.7f, -0.7f}, { 1.1f, -0.1f}},
    {{0.7f, 0.7f}, { 1.1f, 1.1f}}
};

//---------------------------------
void glfw_err_cb(int code,const char * msg) {
    logerror("glfw:code:{} msg:{}", code, msg);
}
VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objType, uint64_t obj, size_t location, int32_t code, const char* layerPrefix, const char* msg, void* userData) {
    logerror("validation layer:{}", msg);

    return VK_FALSE;
}
void windowsizefun(GLFWwindow*window, int w, int h) {
    void * p = glfwGetWindowUserPointer(window);
    OnScreenVulkan *pVulkan = (OnScreenVulkan*)p;
    pVulkan->ReCreateSwapChain();
}

int testoffscreen(){

    const std::vector<const char*> validationLayers;
    if (!CheckInstanceLayerPropertiesSupport(validationLayers)) {
        logerror("checkInstanceLayerPropertiesSupport fail");
        return -1;
    }
    std::vector<const char *> exts;
    VkInstance instance = CreateInstance("offscreen", VK_MAKE_VERSION(0, 1, 0), exts, validationLayers);
    
    OffScreenVulkan vulkan(instance);
    //设置validation layer的callback
    vulkan.SetupValidationLayerCallback(debugCallback);
    
    const std::vector<const char*> deviceExtensions;
    if (!vulkan.SelectProperPhysicalDeviceAndQueueFamily(deviceExtensions)) {
        logerror("failed to select physical device!");
        return -11;
    }
    
    vulkan.CreateLogicalDevice(deviceExtensions, validationLayers);

    vulkan.CreateRenderingResources(2);
    
    int texWidth, texHeight, texChannels;
    stbi_uc* pixels = stbi_load(PIC_PATH"/texture.png", &texWidth,
                                &texHeight, &texChannels, STBI_rgb_alpha);
    int texSize = texWidth * texHeight * 4;
    vulkan.CreateTexture(texSize,
                         texWidth, texHeight, VK_FORMAT_R8G8B8A8_UNORM);

    vulkan.CreateStagingBuffer();
    
    vulkan.CreateDescriptorSetLayout();
    vulkan.CreateDescriptorPool();
    vulkan.AllocateDescriptorSet();
    vulkan.UpdateDescriptorSet();
    vulkan.CreateRenderPass(VK_FORMAT_R8G8B8A8_UNORM);//VK_FORMAT_R8G8B8A8_UNORM VK_FORMAT_B8G8R8A8_UNORM
    vulkan.CreatePipelineLayout();
    vulkan.CreatePipeline();
    vulkan.CreateVertexBuffer(sizeof(Vertex) * vertex_data.size());
    vulkan.UpdateVertexData((const char *)vertex_data.data(), sizeof(Vertex)*vertex_data.size());
    
    vulkan.CreateRenderTarget(1280, 720, VK_FORMAT_R8G8B8A8_UNORM);
    
    std::chrono::time_point<std::chrono::high_resolution_clock> start = std::chrono::high_resolution_clock::now();



    vulkan.CopyTextureData((char *)pixels, texSize, texWidth, texHeight);
    vulkan.Draw();
    
    
    uint8_t *map = (uint8_t *)malloc(1280 * 720 * 4);
    int len = vulkan.ReadPixels(map, 1280*720*4);
    std::ofstream f;
    char outFileName[128] = {0};
    snprintf(outFileName, sizeof(outFileName), "%dx%d_1280x720.rgb", texWidth, texHeight);
    f.open(outFileName, std::fstream::out | std::fstream::binary);
    f.write((const char *)map, len);
    f.flush();
    f.close();
    free(map);
    
    
    std::chrono::time_point<std::chrono::high_resolution_clock> end = std::chrono::high_resolution_clock::now();
    auto diff = end - start;
    int ms = diff.count()/1000000;
    std::cout << "use:" << ms << std::endl;

    
    return 0;
}

int main(int argc, char **argv) {
    logger_init_file_output("gpumix.log");
    //return testoffscreen();
    if(argc > 1){
        printf("just test offsetscreen\n");
        return testoffscreen();
    }
    
    glfwSetErrorCallback(glfw_err_cb);
    int ret = glfwInit();
    if (ret == GLFW_FALSE) {
        logerror("glfwInit fail:{}", ret);
        return -1;
    }


    //获取glfw窗口需要的vulkan扩展
    unsigned int glfwExtensionCount = 0;
    const char** glfwExtensions;
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
#ifdef DEBUG_INFO
    loginfo("required extensions:");
    for (uint32_t i = 0; i < glfwExtensionCount; i++) {
        loginfo("glfw required extension[{}]:{}",i, glfwExtensions[i]);
    }
#endif

    //检查 glfw 要求的扩展是否满足
    if (!CheckInstanceExtensionPropertiesSupport(glfwExtensions, glfwExtensionCount)) {
        logerror("checkInstanceExtensionPropertiesSupport fail");
        return -1;
    }


    //检查要启用的layer是否支持
    
    bool isSupportValidationLayer = true;
    // macos 通过moltenvk支持vulkan，反正就是不支持VK_LAYER_LUNARG_standard_validation这个
    std::vector<const char*> validationLayers = { "VK_LAYER_LUNARG_standard_validation" };
    if (!CheckInstanceLayerPropertiesSupport(validationLayers)) {
        isSupportValidationLayer = false;
        validationLayers.pop_back();
        loginfo("not support VK_LAYER_LUNARG_standard_validation");
    }

    std::vector<const char *> exts;
    for (uint32_t i = 0; i < glfwExtensionCount; i++) {
        exts.push_back(glfwExtensions[i]);
    }

    VkInstance instance = CreateInstance("test", VK_MAKE_VERSION(0, 1, 0), exts, validationLayers);


    //创建glfw窗口 和 surface
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    GLFWwindow* window = glfwCreateWindow(800, 600, "Vulkan window", nullptr, nullptr);

    VkSurfaceKHR surface;
    if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS) {
        logerror("failed to create window surface!");
        return -10;
    }
    //---------------------------
    OnScreenVulkan vulkan(instance, surface);
    //设置validation layer的callback
    vulkan.SetupValidationLayerCallback(debugCallback);

    const std::vector<const char*> deviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
    if (!vulkan.SelectProperPhysicalDeviceAndQueueFamily(deviceExtensions)) {
        logerror("failed to select physical device!");
        return -11;
    }

    // validataionLayers 在instance和device级别都要启动
#ifdef PERF_TEST
    vulkan.CreateLogicalDevice(deviceExtensions);
#else
    vulkan.CreateLogicalDevice(deviceExtensions, validationLayers);
#endif
    vulkan.CreateSwapChain();
    int renderResourceSize = vulkan.GetSwapChainBackBufferSize();
    vulkan.CreateRenderingResources(renderResourceSize);

    int texWidth, texHeight, texChannels;
    stbi_uc* pixels = stbi_load(PIC_PATH"/texture.png", &texWidth,
        &texHeight, &texChannels, STBI_rgb_alpha);
    int texSize = texWidth * texHeight * 4;
    vulkan.CreateTexture(texSize,
        texWidth, texHeight, VK_FORMAT_R8G8B8A8_UNORM);

    vulkan.CreateStagingBuffer();

    vulkan.CreateDescriptorSetLayout();
    vulkan.CreateDescriptorPool();
    vulkan.AllocateDescriptorSet();
    vulkan.UpdateDescriptorSet();
    vulkan.CreateRenderPass();
    vulkan.CreatePipelineLayout();
    vulkan.CreatePipeline();
    vulkan.CreateVertexBuffer(sizeof(Vertex) * vertex_data.size());
    vulkan.UpdateVertexData((const char *)vertex_data.data(), sizeof(Vertex)*vertex_data.size());

    //due to surface is VK_FORMAT_B8G8R8A8_UNORM
    vulkan.CreateRenderTarget(1280, 720, VK_FORMAT_B8G8R8A8_UNORM);

    glfwSetWindowUserPointer(window, &vulkan);
    glfwSetWindowSizeCallback(window, windowsizefun);
    

#ifdef PERF_TEST
    int count = 1;
    std::chrono::time_point<std::chrono::high_resolution_clock> start = std::chrono::high_resolution_clock::now();
#else
    int count = 0;
#endif
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        vulkan.CopyTextureData((char *)pixels, texSize, texWidth, texHeight);
        vulkan.Draw();
        count++;
#ifndef PERF_TEST
        if (count < 2) {
            uint8_t *map = (uint8_t *)malloc(1280 * 720 * 4);
            //ReadPixels(map, sizeof(map), m_renderingResources[resource_index].CommandBuffer);
            int len = vulkan.ReadPixels(map, 1280*720*4);
            std::ofstream f;
            f.open("v.rgb", std::fstream::out | std::fstream::binary);
            f.write((const char *)map, len);
            f.flush();
            f.close();
            free(map);
        }
#else
        uint8_t *pixelBuf = (uint8_t *)malloc(1280 * 720 * 4.5);
        if (count < 1001) {
            int len = vulkan.ReadPixels(pixelBuf, 1280*720*4);
        }
        if (count == 1001) {
            std::chrono::time_point<std::chrono::high_resolution_clock> end = std::chrono::high_resolution_clock::now();
            auto diff = end - start;
            int ms = diff.count()/1000000;
            std::cout << "use:" << ms << std::endl;
            std::cout << std::endl;
        }
#endif
    }

    glfwDestroyWindow(window);

    glfwTerminate();

    return 0;
}
// @TODO 验证image作为rendertarget成功性
/*
定义texture，包含imageparameter和stagingbuffer
*/
