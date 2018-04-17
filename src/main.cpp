#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>


#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <iostream>
#include "onscreen.h"
#include <fstream>
#include <chrono>

//#define PERF_TEST

const std::vector<Vertex> vertex_data = {
    {{-0.7f, -0.7f}, { -0.1f, -0.1f}},
    {{-0.7f, 0.7f}, {-0.1f, 1.1f}},
    {{0.7f, -0.7f}, { 1.1f, -0.1f}},
    {{0.7f, 0.7f}, { 1.1f, 1.1f}}
};

//---------------------------------
void glfw_err_cb(int code,const char * msg) {
    std::cout << "code:" << code << " msg:" << msg << std::endl;
}
VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objType, uint64_t obj, size_t location, int32_t code, const char* layerPrefix, const char* msg, void* userData) {
    std::cerr << "validation layer: " << msg << std::endl;

    return VK_FALSE;
}
void windowsizefun(GLFWwindow*window, int w, int h) {
    void * p = glfwGetWindowUserPointer(window);
    OnScreenVulkan *pVulkan = (OnScreenVulkan*)p;
    pVulkan->ReCreateSwapChain();
}

int main() {
    glfwSetErrorCallback(glfw_err_cb);
    int ret = glfwInit();
    if (ret == GLFW_FALSE) {
        std::cout << "glfwInit fail"<<ret;
        return -1;
    }


    //获取glfw窗口需要的vulkan扩展
    unsigned int glfwExtensionCount = 0;
    const char** glfwExtensions;
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
#ifdef DEBUG_INFO
    std::cout << "required extensions:" << std::endl;
    for (uint32_t i = 0; i < glfwExtensionCount; i++) {
        std::cout << "\t" << glfwExtensions[i] << std::endl;
    }
#endif

    //检查 glfw 要求的扩展是否满足
    if (!CheckInstanceExtensionPropertiesSupport(glfwExtensions, glfwExtensionCount)) {
        std::cout << "checkInstanceExtensionPropertiesSupport fail" << std::endl;
        return -1;
    }


    //检查要启用的layer是否支持
    const std::vector<const char*> validationLayers = { "VK_LAYER_LUNARG_standard_validation" };
    if (!CheckInstanceLayerPropertiesSupport(validationLayers)) {
        std::cout << "checkInstanceLayerPropertiesSupport fail" << std::endl;
        return -1;
    }

    std::vector<const char *> exts;
    for (uint32_t i = 0; i < glfwExtensionCount; i++) {
        exts.push_back(glfwExtensions[i]);
    }

    VkInstance instance = CreateInstance(exts, validationLayers);


    //创建glfw窗口 和 surface
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    GLFWwindow* window = glfwCreateWindow(800, 600, "Vulkan window", nullptr, nullptr);

    VkSurfaceKHR surface;
    if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS) {
        throw std::runtime_error("failed to create window surface!");
    }
    //---------------------------
    OnScreenVulkan vulkan(instance, surface);
    //设置validation layer的callback
    vulkan.SetupValidationLayerCallback(debugCallback);

    const std::vector<const char*> deviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
    if (!vulkan.SelectProperPhysicalDeviceAndQueueFamily(deviceExtensions)) {
        throw std::runtime_error("failed to select physical device!");
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
    stbi_uc* pixels = stbi_load("/Users/liuye/vulkan/gpumix/src/Data06/texture.png", &texWidth,
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
