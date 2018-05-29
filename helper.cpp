#include "helper.h"
#include <set>
#include <fstream>
#include <logger.h>

/*
 * desc: 返回支持的layer
 * ps: macOS上目前还不支持layer
 */
std::unique_ptr<std::vector<VkLayerProperties>> GetInstanceLayerProperties()
{
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
    
    std::unique_ptr<std::vector<VkLayerProperties>> props =
    std::make_unique<std::vector<VkLayerProperties>>(layerCount);
    
    vkEnumerateInstanceLayerProperties(&layerCount, props->data());
    
    loginfo("available instance layer properties:");
    for (uint32_t i = 0; i < layerCount; i++) {
        loginfo("\t{}", props->operator[](i).layerName);
    }
    
    return props;
}

/**
 * 检查某些layer是否支持
 * @param _enableLayers init like that std::vector<const char*> _enableLayers = { "VK_LAYER_LUNARG_standard_validation" };
 **/
bool CheckInstanceLayerPropertiesSupport(std::vector<const char*> _enableLayers)
{
    std::shared_ptr<std::vector<VkLayerProperties>> 
        availableLayers = GetInstanceLayerProperties();

    for (const char* layerName : _enableLayers) {
        bool layerFound = false;

        for (int i = 0; i < availableLayers->size(); i++) {
            if (strcmp(layerName, availableLayers->operator[](i).layerName) == 0) {
                layerFound = true;
                break;
            }
        }

        if (!layerFound) {
            return false;
        }
    }

    return true;
}

/**
 * desc: 获取instance级别支持的扩展
 **/
std::unique_ptr<std::vector<VkExtensionProperties>> GetInstanceExtensionProperties()
{
    uint32_t extensionCount = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
    std::unique_ptr<std::vector<VkExtensionProperties>> props =
    std::make_unique<std::vector<VkExtensionProperties>>(extensionCount);
    
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, props->data());
    
    loginfo("available instance extension properties:");
    for (uint32_t i = 0; i < extensionCount; i++) {
        loginfo("\t{}", props->operator[](i).extensionName);
    }
    
    return props;
}

/*
 * desc: 检查instance支持的扩展，比如VK_KHR_swapchain就是一个扩展(vulkan自身就支持做离屏渲染)
 */
bool CheckInstanceExtensionPropertiesSupport(std::vector<const char*> _enableExtensions)
{
    std::unique_ptr<std::vector<VkExtensionProperties>>
        availableExtensions = GetInstanceExtensionProperties();

    for (const char* extensionName : _enableExtensions) {
        bool extensionFound = false;

        for (int i = 0; i < availableExtensions->size(); i++) {
            if (strcmp(extensionName, availableExtensions->operator[](i).extensionName) == 0) {
                extensionFound = true;
                break;
            }
        }

        if (!extensionFound) {
            return false;
        }
    }

    return true;
}

/**
 * desc: just c style array to c++ vector
 **/
bool CheckInstanceExtensionPropertiesSupport(const char ** _enableExtensions, int _count)
{
    std::vector<const char *> exts;
    for (int i = 0; i < _count; i++) {
        exts.push_back(_enableExtensions[i]);
    }
    return CheckInstanceExtensionPropertiesSupport(exts);
}

/**
 * 创建instance
 **/
VkInstance CreateInstance(const std::vector<const char*> _enableLayers, const std::vector<const char*> _enableExtensions)
{
    VkApplicationInfo appInfo = {};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Hello Triangle";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;
    
    VkInstanceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    
    createInfo.enabledExtensionCount = static_cast<uint32_t>(_enableExtensions.size());
    createInfo.ppEnabledExtensionNames = _enableExtensions.data();
    
    if (_enableLayers.size() > 0) {
        createInfo.enabledLayerCount = static_cast<uint32_t>(_enableLayers.size());
        createInfo.ppEnabledLayerNames = _enableLayers.data();
    }
    else {
        createInfo.enabledLayerCount = 0;
    }
    
    VkInstance instance = VK_NULL_HANDLE;
    if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
        throw std::runtime_error("failed to create instance!");
    }
    return instance;
}

/**
 * 获取物理设别，通常电脑只有集成显卡和独立显卡，最多两个设备
 **/
std::unique_ptr<std::vector<VkPhysicalDevice>> GetPhysicalDevices(VkInstance _instance)
{
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(_instance, &deviceCount, nullptr);
    
    if (deviceCount == 0) {
        throw std::runtime_error("failed to find GPUs with Vulkan support!");
    }
    
    std::unique_ptr<std::vector<VkPhysicalDevice>>
    physicalDevices = std::make_unique<std::vector<VkPhysicalDevice>>(deviceCount);
    
    vkEnumeratePhysicalDevices(_instance, &deviceCount, physicalDevices->data());
    
    //设备信息通过其它函数传入VkPhysicalDevice获取，所以这里只打印数量
    loginfo("available physical devices count:{}", deviceCount);
    
    return physicalDevices;
}

std::unique_ptr<VkPhysicalDeviceProperties> GetPhysicalDeviceProperties(VkPhysicalDevice _physicalDevice)
{
    //VkPhysicalDeviceProperties
    //  deviceType 字段集成或者独立显
    //  apiVersion 支持的vulkan的最高版本
    //  VkPhysicalDeviceLimits limits 显卡的物理限制，如
    //     1. limits.discreteQueuePriorities 和队列优先级有关
    std::unique_ptr<VkPhysicalDeviceProperties>
    props = std::make_unique<VkPhysicalDeviceProperties>();
    vkGetPhysicalDeviceProperties(_physicalDevice, props.get());
    
    loginfo("device name:{}", props->deviceName);
    
    return props;
}

/**
 * extension properties: char extensionName[VK_MAX_EXTENSION_NAME_SIZE];uint32_t specVersion;
 * 扩展名，如VK_KHR_swapchain
 **/
std::unique_ptr<std::vector<VkExtensionProperties>> GetPhysicalDeviceExtensionProperties(VkPhysicalDevice _physicalDevice)
{
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(_physicalDevice, nullptr, &extensionCount, nullptr);
    
    std::unique_ptr<std::vector<VkExtensionProperties>> props =
    std::make_unique<std::vector<VkExtensionProperties>>(extensionCount);
    
    vkEnumerateDeviceExtensionProperties(_physicalDevice, nullptr, &extensionCount, props->data());
    
    loginfo("available physical device extension properties:");
    for (uint32_t i = 0; i < extensionCount; i++) {
        loginfo("\t{}, specVersion:{}", props->operator[](i).extensionName, props->operator[](i).specVersion);
    }
    
    return props;
}

std::unique_ptr<std::vector<VkQueueFamilyProperties>> GetPhysicalDeviceQueueFamilyProperties(
    VkPhysicalDevice _physicalDevice)
{
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(_physicalDevice, &queueFamilyCount, nullptr);

    //VkQueueFamilyProperties
    //  queueFlags 常见包括图形功能 计算功能 传输操作(例如复制缓冲区和映像内容)
    std::unique_ptr<std::vector<VkQueueFamilyProperties>> props =
        std::make_unique<std::vector<VkQueueFamilyProperties>>(queueFamilyCount);

    vkGetPhysicalDeviceQueueFamilyProperties(_physicalDevice, &queueFamilyCount, props->data());
    
    loginfo("available queue:");
    for (uint32_t i = 0; i < queueFamilyCount; i++) {
        loginfo("\t{}queueCount:{}", i, props->operator[](i).queueCount);
        auto flag = props->operator[](i).queueFlags;
        if (flag & VK_QUEUE_GRAPHICS_BIT)
            loginfo("\t\tVK_QUEUE_GRAPHICS_BIT");
        if (flag & VK_QUEUE_COMPUTE_BIT)
            loginfo("\t\tVK_QUEUE_COMPUTE_BIT");
        if (flag & VK_QUEUE_TRANSFER_BIT)
            loginfo("\t\tVK_QUEUE_TRANSFER_BIT");
        if (flag & VK_QUEUE_SPARSE_BINDING_BIT)
            loginfo("\t\tVK_QUEUE_SPARSE_BINDING_BIT");
        if (flag & VK_QUEUE_PROTECTED_BIT)
            loginfo("\t\tVK_QUEUE_PROTECTED_BIT");
        if (flag == VK_QUEUE_FLAG_BITS_MAX_ENUM)
            loginfo("\t\tVK_QUEUE_FLAG_BITS_MAX_ENUM");
    }

    return props;
}


std::unique_ptr<VkPhysicalDeviceFeatures> GetPhysicalDeviceFeatures(VkPhysicalDevice _physicalDevice)
{
    std::unique_ptr<VkPhysicalDeviceFeatures>
        features = std::make_unique<VkPhysicalDeviceFeatures>();
    vkGetPhysicalDeviceFeatures(_physicalDevice, features.get());

    return features;
}

std::unique_ptr<VkPhysicalDeviceMemoryProperties> GetPhysicalDeviceMemoryProperties(VkPhysicalDevice _physicalDevice)
{
    //VkPhysicalDeviceMemoryProperties
    //  memoryTypes(VkMemoryType)
    //    propertyFlags 内存类型 只能设备(gpu)可见 主机可见等
    std::unique_ptr<VkPhysicalDeviceMemoryProperties>
        props = std::make_unique<VkPhysicalDeviceMemoryProperties>();
    vkGetPhysicalDeviceMemoryProperties(_physicalDevice, props.get());

    for (uint32_t i = 0; i < props->memoryHeapCount; i++) {
        loginfo("\tmemoryHeapCount Idx:{} size:{}", i, props->memoryHeaps[i].size);
        auto flag = props->memoryHeaps[i].flags;
        if (flag & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT)
            loginfo("\t\tVK_MEMORY_HEAP_DEVICE_LOCAL_BIT");
        if (flag & VK_MEMORY_HEAP_MULTI_INSTANCE_BIT)
            loginfo("\t\tVK_MEMORY_PROPERTY_HOST_VISIBLE_BIT==VK_MEMORY_HEAP_MULTI_INSTANCE_BIT_KHR");
        if (flag == VK_MEMORY_HEAP_FLAG_BITS_MAX_ENUM)
            loginfo("\t\tVK_MEMORY_HEAP_FLAG_BITS_MAX_ENUM");
    }
    for (uint32_t i = 0; i < props->memoryTypeCount; i++) {
        loginfo("\tmemoryType Idx:{}, memoryHeap Idx:{}", i, props->memoryTypes[i].heapIndex);
        auto flag = props->memoryTypes[i].propertyFlags;
        if (flag & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)// = 0x00000001,
            loginfo("\t\tVK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT");
        if (flag & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)// = 0x00000002,
            loginfo("\t\tVK_MEMORY_PROPERTY_HOST_VISIBLE_BIT");
        if (flag & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)// = 0x00000004,
            loginfo("\t\tVK_MEMORY_PROPERTY_HOST_COHERENT_BIT");
        if (flag & VK_MEMORY_PROPERTY_HOST_CACHED_BIT)// = 0x00000008,
            loginfo("\t\tVK_MEMORY_PROPERTY_HOST_CACHED_BIT");
        if (flag & VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT)// = 0x00000010,
            loginfo("\t\tVK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT");
        if (flag == VK_MEMORY_PROPERTY_FLAG_BITS_MAX_ENUM)//= 0x7FFFFFF
            loginfo("\t\tVK_MEMORY_PROPERTY_FLAG_BITS_MAX_ENUM");
    }

    return props;
}


int CheckPhysicalDeviceQueueFamilyPropertiesSupport(VkPhysicalDevice _physicalDevice, VkQueueFlags _propsFlag)
{
    auto props = GetPhysicalDeviceQueueFamilyProperties(_physicalDevice);
    for (int i = 0; i < props->size(); i++) {
        if (props->operator[](i).queueCount > 0 && props->operator[](i).queueFlags & _propsFlag) {
            return i;
        }
    }
    return -1;
}

VkMemoryPropertyFlags GetBestMemoryPropertyFlags(VkMemoryPropertyFlags _must,
    VkMemoryPropertyFlags _optional, const VkPhysicalDeviceMemoryProperties * _devicePorps)
{
    VkMemoryPropertyFlags bestFlags = 0;
    for (uint32_t i = 0; i < _devicePorps->memoryTypeCount; i++) {
        int m = _devicePorps->memoryTypes[i].propertyFlags & _must;
        int o = _devicePorps->memoryTypes[i].propertyFlags & _optional;
        if (m > 0 && o > 0)
            return _must | _optional;
        if (m > 0 && bestFlags == 0)
            bestFlags = _devicePorps->memoryTypes[i].propertyFlags;
    }
    if (bestFlags > 0)
        return _must;
    return 0;
}

/**
 * extension properties：检查扩展是否支持，如VK_KHR_swapchain
 **/
bool CheckPhsicalDeviceExtensionsSupport(VkPhysicalDevice _physicalDevice, const std::vector<const char*> _enableExtensions)
{
    auto exts = GetPhysicalDeviceExtensionProperties(_physicalDevice);

    std::set<std::string> requiredExtensions(_enableExtensions.begin(), _enableExtensions.end());

    for (int i = 0; i < exts->size(); i++) {
        requiredExtensions.erase(exts->operator[](i).extensionName);
    }

    return requiredExtensions.empty();
}

VkShaderModule CreateShaderModule(VkDevice _device, const std::vector<char>& _code)
{
    VkShaderModuleCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = _code.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(_code.data());

    VkShaderModule shaderModule;
    if (vkCreateShaderModule(_device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
        throw std::runtime_error("failed to create shader module!");
    }

    return shaderModule;
}

VkShaderModule CreateShaderModuleFromFile(VkDevice _device, const char * _fileName)
{
    std::ifstream file(_fileName, std::ios::ate | std::ios::binary);

    if (!file.is_open()) {
        throw std::runtime_error("failed to open file!");
    }

    size_t fileSize = (size_t)file.tellg();
    std::vector<char> buffer(fileSize);

    file.seekg(0);
    file.read(buffer.data(), fileSize);

    file.close();

    return CreateShaderModule(_device, buffer);
}

int CheckPhysicalDeviceSurfaceSupport(VkPhysicalDevice _physicalDevice, VkSurfaceKHR _surface)
{
    auto props = GetPhysicalDeviceQueueFamilyProperties(_physicalDevice);
    for (int i = 0; i < props->size(); i++) {
        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(_physicalDevice, i, _surface, &presentSupport);

        if (props->operator[](i).queueCount > 0 && presentSupport) {
            return i;
        }
    }
    return -1;
}

SwapChainSupportDetails QuerySwapChainSupport(VkPhysicalDevice _physicalDevice, VkSurfaceKHR _surface)
{
    SwapChainSupportDetails details;

    //获取的功能包含有关交换链支持范围（限制）的重要信息
    //即图像的最大数和最小数(就是backbuffer的支持数量)、图像的最小尺寸和最大尺寸，
    //或支持的转换格式（有些平台可能要求在演示图像之前首先进行图像转换）
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(_physicalDevice, _surface, &details.capabilities);

    //支持的颜色空间
    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(_physicalDevice, _surface, &formatCount, nullptr);

    if (formatCount != 0) {
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(_physicalDevice, _surface, &formatCount, details.formats.data());
    }

    //支持的演示模式 如 VK_PRESENT_MODE_FIFO_KHR 等
    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(_physicalDevice, _surface, &presentModeCount, nullptr);

    if (presentModeCount != 0) {
        details.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(_physicalDevice, _surface, &presentModeCount, details.presentModes.data());
    }

    return details;
}

VkSurfaceFormatKHR GetProperSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
{

    // If the list contains only one entry with undefined format
    // it means that there are no preferred surface formats and any can be chosen
    if (availableFormats.size() == 1 && availableFormats[0].format == VK_FORMAT_UNDEFINED) {
        return{ VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
    }

    // Check if list contains most widely used R8 G8 B8 A8 format
    // with nonlinear color spac
    for (const auto& availableFormat : availableFormats) {
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return availableFormat;
        }
    }

    return availableFormats[0];
}

VkPresentModeKHR GetProperSwapPresentMode(const std::vector<VkPresentModeKHR> availablePresentModes)
{
    // FIFO present mode is always available
    //VkPresentModeKHR backupMode = VK_PRESENT_MODE_FIFO_KHR;
    VkPresentModeKHR backupMode = VK_PRESENT_MODE_IMMEDIATE_KHR;

    for (const auto& bestMode : availablePresentModes) {
        // MAILBOX is the lowest latency V-Sync enabled mode
        // (something like triple-buffering) so use it if available
        if (bestMode == VK_PRESENT_MODE_MAILBOX_KHR) {
            return bestMode;
        }
    }

    return backupMode;
}

// Most of the cases we define size of the swap_chain images equal to current window's size
VkExtent2D GetProperSwapChainExtent(VkSurfaceCapabilitiesKHR &surfaceCapabilities) {
    // Special value of surface extent is width == height == -1
    // If this is so we define the size by ourselves but it must fit within defined confines
    if (surfaceCapabilities.currentExtent.width == -1) {
        VkExtent2D swapchainExtent = { 640, 480 };
        if (swapchainExtent.width < surfaceCapabilities.minImageExtent.width) {
            swapchainExtent.width = surfaceCapabilities.minImageExtent.width;
        }
        if (swapchainExtent.height < surfaceCapabilities.minImageExtent.height) {
            swapchainExtent.height = surfaceCapabilities.minImageExtent.height;
        }
        if (swapchainExtent.width > surfaceCapabilities.maxImageExtent.width) {
            swapchainExtent.width = surfaceCapabilities.maxImageExtent.width;
        }
        if (swapchainExtent.height > surfaceCapabilities.maxImageExtent.height) {
            swapchainExtent.height = surfaceCapabilities.maxImageExtent.height;
        }
        return swapchainExtent;
    }

    // Most of the cases we define size of the swap_chain images equal to current window's size
    return surfaceCapabilities.currentExtent;
}

VkImageUsageFlags GetSwapSufaceImageUsageFlags(VkSurfaceCapabilitiesKHR &surfaceCapabilities) {
    // Color attachment flag must always be supported
    // We can define other usage flags but we always need to check if they are supported
    if (surfaceCapabilities.supportedUsageFlags & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT
        //swapchain 的image不一定一直是rendertarget， 为了从别的rendertarget(image)
        //copy数据，所以需要这个标志DST_BIT
     && surfaceCapabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_DST_BIT) {
        return VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    }
    loginfo("VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT image usage is not supported by the swap chain!\nSupported swap chain's image usages include:\n"
            "{}{}{}{}{}{}{}{}",
            (surfaceCapabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_SRC_BIT ? "    VK_IMAGE_USAGE_TRANSFER_SRC\n" : ""),
            (surfaceCapabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_DST_BIT ? "    VK_IMAGE_USAGE_TRANSFER_DST\n" : ""),
            (surfaceCapabilities.supportedUsageFlags & VK_IMAGE_USAGE_SAMPLED_BIT ? "    VK_IMAGE_USAGE_SAMPLED\n" : ""),
            (surfaceCapabilities.supportedUsageFlags & VK_IMAGE_USAGE_STORAGE_BIT ? "    VK_IMAGE_USAGE_STORAGE\n" : ""),
            (surfaceCapabilities.supportedUsageFlags & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT ? "    VK_IMAGE_USAGE_COLOR_ATTACHMENT\n" : ""),
            (surfaceCapabilities.supportedUsageFlags & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT ? "    VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT\n" : ""),
            (surfaceCapabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT ? "    VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT\n" : ""),
            (surfaceCapabilities.supportedUsageFlags & VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT ? "    VK_IMAGE_USAGE_INPUT_ATTACHMENT" : ""));

    return static_cast<VkImageUsageFlags>(-1);
}

VkSurfaceTransformFlagBitsKHR GetSwapChainTransform(VkSurfaceCapabilitiesKHR &surface_capabilities) {
    // Sometimes images must be transformed before they are presented (i.e. due to device's orienation
    // being other than default orientation)
    // If the specified transform is other than current transform, presentation engine will transform image
    // during presentation operation; this operation may hit performance on some platforms
    // Here we don't want any transformations to occur so if the identity transform is supported use it
    // otherwise just use the same transform as current transform
    if (surface_capabilities.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR) {
        return VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    }
    else {
        return surface_capabilities.currentTransform;
    }
}

VkImageCreateInfo Get2DImageCreateInfo(uint32_t width, uint32_t height,
        VkImageTiling _tiling, VkImageUsageFlags _usageFlags, VkFormat _format)
{
    VkImageCreateInfo image_create_info = {
        VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO, // VkStructureType     sType;
        nullptr,                             // const void          *pNext
        0,                                   // VkImageCreateFlags  flags
        VK_IMAGE_TYPE_2D,                    // VkImageType         imageType
        _format,                             // VkFormat            format
        {                                    // VkExtent3D          extent
            width,                           // uint32_t            width
            height,                          // uint32_t            height
            1                                // uint32_t            depth
        },
        1,                                   // uint32_t            mipLevels
        1,                                   // uint32_t            arrayLayers
        VK_SAMPLE_COUNT_1_BIT,               // VkSampleCountFlagBits samples
        _tiling,                             // VkImageTiling         tiling
        _usageFlags,                         // VkImageUsageFlags     usage
        VK_SHARING_MODE_EXCLUSIVE,           // VkSharingMode         sharingMode
        0,                                   // uint32_t              queueFamilyIndexCount
        nullptr,                             // const uint32_t*       pQueueFamilyIndices
        VK_IMAGE_LAYOUT_UNDEFINED            // VkImageLayout         initialLayout
    };
    return image_create_info;
}

VkImageViewCreateInfo Get2DImageViewCreateInfo(VkImage _image, VkFormat _format)
{
    VkImageViewCreateInfo image_view_create_info = {
        VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,  // VkStructureType            sType
        nullptr,                                   // const void                *pNext
        0,                                         // VkImageViewCreateFlags     flags
        _image                  ,                   // VkImage                    image
        VK_IMAGE_VIEW_TYPE_2D,                     // VkImageViewType            viewType
        _format,                                  // VkFormat                   format
        {                                          // VkComponentMapping         components
            VK_COMPONENT_SWIZZLE_IDENTITY,         // VkComponentSwizzle         r
            VK_COMPONENT_SWIZZLE_IDENTITY,         // VkComponentSwizzle         g
            VK_COMPONENT_SWIZZLE_IDENTITY,         // VkComponentSwizzle         b
            VK_COMPONENT_SWIZZLE_IDENTITY          // VkComponentSwizzle         a
        },
        {                                          // VkImageSubresourceRange    subresourceRange
            VK_IMAGE_ASPECT_COLOR_BIT,             // VkImageAspectFlags         aspectMask
            0,                                     // uint32_t                   baseMipLevel
            1,                                     // uint32_t                   levelCount
            0,                                     // uint32_t                   baseArrayLayer
            1                                      // uint32_t                   layerCount
        }
    };
    return image_view_create_info;
}

VkSamplerCreateInfo GetSamplerCreateInfo()
{
    VkSamplerCreateInfo sampler_create_info = {
        VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,                // VkStructureType            sType
        nullptr,                                              // const void*                pNext
        0,                                                    // VkSamplerCreateFlags       flags
        VK_FILTER_LINEAR,                                     // VkFilter                   magFilter
        VK_FILTER_LINEAR,                                     // VkFilter                   minFilter
        VK_SAMPLER_MIPMAP_MODE_NEAREST,                       // VkSamplerMipmapMode        mipmapMode
        VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,                // VkSamplerAddressMode       addressModeU
        VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,                // VkSamplerAddressMode       addressModeV
        VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,                // VkSamplerAddressMode       addressModeW
        0.0f,                                                 // float                      mipLodBias
        VK_FALSE,                                             // VkBool32                   anisotropyEnable
        1.0f,                                                 // float                      maxAnisotropy
        VK_FALSE,                                             // VkBool32                   compareEnable
        VK_COMPARE_OP_ALWAYS,                                 // VkCompareOp                compareOp
        0.0f,                                                 // float                      minLod
        0.0f,                                                 // float                      maxLod
        VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK,              // VkBorderColor              borderColor
        VK_FALSE                                              // VkBool32                   unnormalizedCoordinates
    };
    return sampler_create_info;
}

VkCommandBufferBeginInfo GetCommandBufferOneTimeSubmitBeginInfo() {

    VkCommandBufferBeginInfo command_buffer_begin_info = {
        VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,        // VkStructureType                        sType
        nullptr,                                            // const void                            *pNext
        VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,        // VkCommandBufferUsageFlags              flags
        nullptr                                             // const VkCommandBufferInheritanceInfo  *pInheritanceInfo
    };
    return command_buffer_begin_info;
}

VkImageMemoryBarrier GetDstSwapChainImageBeforeCopyMemoryBarrier(
    uint32_t _presentQueue, uint32_t _graphicQueue, VkImage _image, VkImageSubresourceRange _range)
{
    VkImageMemoryBarrier barrier = {
        VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,    // VkStructureType       sType
        NULL,                                      // const void            *pNext
        0,                                         // VkAccessFlags         srcAccessMask
        VK_ACCESS_TRANSFER_WRITE_BIT,              // VkAccessFlags         dstAccessMask
        //对于交换链的图像，这里一定是VK_IMAGE_LAYOUT_UNDEFINED
        //因为交换链创建无法指定初始布局
        VK_IMAGE_LAYOUT_UNDEFINED,                // VkImageLayout         oldLayout
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,     // VkImageLayout         newLayout
        _presentQueue,                            // uint32_t              srcQueueFamilyIndex
        _graphicQueue,                            // uint32_t              dstQueueFamilyIndex
        _image,                                   // VkImage               image
        _range                                    // VkImageSubresourceRange subresourceRange
    };
    return barrier;
}

VkImageMemoryBarrier GetSrcSwapChainImageBeforeCopyMemoryBarrier(
    uint32_t _presentQueue, uint32_t _graphicQueue, VkImage _image, VkImageSubresourceRange _range)
{
    VkImageMemoryBarrier barrier = {
        VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,    // VkStructureType       sType
        NULL,                                      // const void            *pNext
        0,                                         // VkAccessFlags         srcAccessMask
        VK_ACCESS_TRANSFER_READ_BIT,              // VkAccessFlags         dstAccessMask
        //copy swapchain时候，肯定已经转化过了
        VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,          // VkImageLayout         oldLayout
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,     // VkImageLayout         newLayout
        _presentQueue,                            // uint32_t              srcQueueFamilyIndex
        _graphicQueue,                            // uint32_t              dstQueueFamilyIndex
        _image,                                   // VkImage               image
        _range                                    // VkImageSubresourceRange subresourceRange
    };
    return barrier;
}

VkImageMemoryBarrier GetDstSwapChainImageAfterCopyMemoryBarrier(
    uint32_t _presentQueue, uint32_t _graphicQueue, VkImage _image, VkImageSubresourceRange _range)
{
    VkImageMemoryBarrier barrier = {
        VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,   // VkStructureType       sType
        nullptr,                                  // const void            *pNext
        VK_ACCESS_TRANSFER_WRITE_BIT,             // VkAccessFlags         srcAccessMask
        VK_ACCESS_MEMORY_READ_BIT,                // VkAccessFlags         dstAccessMask
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,     // VkImageLayout         oldLayout
        VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,          // VkImageLayout         newLayout
        _presentQueue,                            // uint32_t              srcQueueFamilyIndex
        _graphicQueue,                            // uint32_t              dstQueueFamilyIndex
        _image,                                   // VkImage               image
        _range                                    // VkImageSubresourceRange subresourceRange
    };
    return barrier;
}

VkImageMemoryBarrier GetSrcSwapChainImageAfterCopyMemoryBarrier(
    uint32_t _presentQueue, uint32_t _graphicQueue, VkImage _image, VkImageSubresourceRange _range)
{
    VkImageMemoryBarrier barrier = {
        VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,   // VkStructureType       sType
        nullptr,                                  // const void            *pNext
        VK_ACCESS_TRANSFER_READ_BIT,             // VkAccessFlags         srcAccessMask
        VK_ACCESS_MEMORY_READ_BIT,                // VkAccessFlags         dstAccessMask
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,     // VkImageLayout         oldLayout
        VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,          // VkImageLayout         newLayout
        _presentQueue,                            // uint32_t              srcQueueFamilyIndex
        _graphicQueue,                            // uint32_t              dstQueueFamilyIndex
        _image,                                   // VkImage               image
        _range                                    // VkImageSubresourceRange subresourceRange
    };
    return barrier;
}

VkImageMemoryBarrier GetSwapChainImageBeforeRenderMemoryBarrier(
    uint32_t _presentQueue, uint32_t _graphicQueue, VkImage _image, VkImageSubresourceRange _range)
{
    VkImageMemoryBarrier barrier = {
        VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,             // VkStructureType                        sType
        nullptr,                                            // const void                            *pNext
        VK_ACCESS_MEMORY_READ_BIT,                          // VkAccessFlags                          srcAccessMask
        VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,               // VkAccessFlags                          dstAccessMask
        VK_IMAGE_LAYOUT_UNDEFINED,                          // VkImageLayout                          oldLayout
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,           // VkImageLayout                          newLayout
        _presentQueue,                            // uint32_t              srcQueueFamilyIndex
        _graphicQueue,                            // uint32_t              dstQueueFamilyIndex
        _image,                                   // VkImage               image
        _range                                    // VkImageSubresourceRange subresourceRange
    };
    return barrier;
}

VkImageMemoryBarrier GetSwapChainImageAfterRenderMemoryBarrier(
    uint32_t _presentQueue, uint32_t _graphicQueue, VkImage _image, VkImageSubresourceRange _range)
{
    VkImageMemoryBarrier barrier = {
        VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,             // VkStructureType                        sType
        nullptr,                                            // const void                            *pNext
        VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,               // VkAccessFlags                          srcAccessMask
        VK_ACCESS_MEMORY_READ_BIT,                          // VkAccessFlags                          dstAccessMask
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,           // VkImageLayout                          oldLayout
        VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,                    // VkImageLayout                          newLayout
        _presentQueue,                            // uint32_t              srcQueueFamilyIndex
        _graphicQueue,                            // uint32_t              dstQueueFamilyIndex
        _image,                                   // VkImage               image
        _range                                    // VkImageSubresourceRange subresourceRange
    };
    return barrier;
}

VkImageMemoryBarrier GetSrcImageBeforeCopyMemoryBarrier(
    uint32_t _presentQueue, uint32_t _graphicQueue, VkImage _image, VkImageSubresourceRange _range)
{
    VkImageMemoryBarrier barrier = {
        VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,    // VkStructureType       sType
        nullptr,                                   // const void            *pNext
        VK_ACCESS_MEMORY_READ_BIT,                 // VkAccessFlags         srcAccessMask
        VK_ACCESS_TRANSFER_READ_BIT,      // VkAccessFlags         dstAccessMask
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,                 // VkImageLayout         oldLayout
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,  // VkImageLayout         newLayout
        _presentQueue,                            // uint32_t              srcQueueFamilyIndex
        _graphicQueue,                            // uint32_t              dstQueueFamilyIndex
        _image,                                   // VkImage               image
        _range                                    // VkImageSubresourceRange subresourceRange
    };
    return barrier;
}

VkImageMemoryBarrier GetSrcImageAfterCopyMemoryBarrier(
    uint32_t _presentQueue, uint32_t _graphicQueue, VkImage _image, VkImageSubresourceRange _range)
{
    VkImageMemoryBarrier barrier = {
        VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,    // VkStructureType       sType
        nullptr,                                   // const void            *pNext
        VK_ACCESS_TRANSFER_READ_BIT,               // VkAccessFlags         srcAccessMask
        VK_ACCESS_MEMORY_READ_BIT,                 // VkAccessFlags         dstAccessMask
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,      // VkImageLayout         oldLayout
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,                 // VkImageLayout         newLayout
        _presentQueue,                            // uint32_t              srcQueueFamilyIndex
        _graphicQueue,                            // uint32_t              dstQueueFamilyIndex
        _image,                                   // VkImage               image
        _range                                    // VkImageSubresourceRange subresourceRange
    };
    return barrier;
}

VkImageMemoryBarrier GetDstImageBeforeCopyMemoryBarrier(
    uint32_t _presentQueue, uint32_t _graphicQueue, VkImage _image, VkImageSubresourceRange _range)
{
    VkImageMemoryBarrier barrier = {
        VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,   // VkStructureType       sType
        nullptr,                                  // const void            *pNext
        VK_ACCESS_MEMORY_READ_BIT,                // VkAccessFlags         srcAccessMask
        VK_ACCESS_TRANSFER_WRITE_BIT,             // VkAccessFlags         dstAccessMask
        //image一开始layout不确定,如果oldlayout不对validation layerhi报错
        VK_IMAGE_LAYOUT_UNDEFINED ,               // VkImageLayout         oldLayout
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,     // VkImageLayout         newLayout
        _presentQueue,                            // uint32_t              srcQueueFamilyIndex
        _graphicQueue,                            // uint32_t              dstQueueFamilyIndex
        _image,                                   // VkImage               image
        _range                                    // VkImageSubresourceRange subresourceRange
    };
    return barrier;
}

VkImageMemoryBarrier GetDstImageAfterCopyMemoryBarrier(
    uint32_t _presentQueue, uint32_t _graphicQueue, VkImage _image, VkImageSubresourceRange _range)
{
    VkImageMemoryBarrier barrier = {
        VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,   // VkStructureType       sType
        nullptr,                                  // const void            *pNext
        VK_ACCESS_TRANSFER_WRITE_BIT,             // VkAccessFlags         srcAccessMask
        VK_ACCESS_MEMORY_READ_BIT,                // VkAccessFlags         dstAccessMask
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,     // VkImageLayout         oldLayout
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, // VkImageLayout         newLayout
        _presentQueue,                            // uint32_t              srcQueueFamilyIndex
        _graphicQueue,                            // uint32_t              dstQueueFamilyIndex
        _image,                                   // VkImage               image
        _range                                    // VkImageSubresourceRange subresourceRange
    };
    return barrier;
}

VkImageMemoryBarrier GetImageBeforeRenderMemoryBarrier(
    uint32_t _presentQueue, uint32_t _graphicQueue, VkImage _image, VkImageSubresourceRange _range)
{

    VkImageMemoryBarrier barrier = {
        VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,    // VkStructureType       sType
        nullptr,                                   // const void            *pNext
        VK_ACCESS_MEMORY_READ_BIT,                 // VkAccessFlags         srcAccessMask
        VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,      // VkAccessFlags         dstAccessMask
        VK_IMAGE_LAYOUT_UNDEFINED,                 // VkImageLayout         oldLayout
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,  // VkImageLayout         newLayout
        _presentQueue,                            // uint32_t              srcQueueFamilyIndex
        _graphicQueue,                            // uint32_t              dstQueueFamilyIndex
        _image,                                   // VkImage               image
        _range                                    // VkImageSubresourceRange subresourceRange
    };
    return barrier;
}

VkImageMemoryBarrier GetImageAfterRenderMemoryBarrier(
    uint32_t _presentQueue, uint32_t _graphicQueue, VkImage _image, VkImageSubresourceRange _range)
{
    VkImageMemoryBarrier barrier = {
        VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,     // VkStructureType    sType
        nullptr,                                    // const void         *pNext
        VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,       // VkAccessFlags      srcAccessMask
        VK_ACCESS_MEMORY_READ_BIT,                  // VkAccessFlags      dstAccessMask
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,   // VkImageLayout      oldLayout
        //好像转换为其它的layout都要报错了，但是也没有必要转为其它的格式了
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,   // VkImageLayout         newLayout
        _presentQueue,                            // uint32_t              srcQueueFamilyIndex
        _graphicQueue,                            // uint32_t              dstQueueFamilyIndex
        _image,                                   // VkImage               image
        _range                                    // VkImageSubresourceRange subresourceRange
    };
    return barrier;
}

VkDeviceSize GetLinearImageRowPitch(VkDevice _device, VkImage _image) {
    VkImageSubresource subRes = {};
    subRes.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    subRes.mipLevel = 0;
    subRes.arrayLayer = 0;

    VkSubresourceLayout subResLayout = { 0 };
    vkGetImageSubresourceLayout(_device, _image, &subRes, &subResLayout);
    return subResLayout.rowPitch;
}
