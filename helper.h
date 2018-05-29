#pragma once
#include <vector>
#include <vulkan/vulkan.h>
#include <memory>
#include <iostream>

#define DEBUG_INFO
struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

struct QueueParameters {
    VkQueue                       Handle;
    uint32_t                      FamilyIndex;

    QueueParameters() :
        Handle(VK_NULL_HANDLE),
        FamilyIndex(-1) {
    }
};

//instance level
//  layer
std::unique_ptr<std::vector<VkLayerProperties>> GetInstanceLayerProperties();
bool CheckInstanceLayerPropertiesSupport(std::vector<const char*> _enableLayers);
//  extension
std::unique_ptr<std::vector<VkExtensionProperties>> GetInstanceExtensionProperties();
bool CheckInstanceExtensionPropertiesSupport(std::vector<const char*> _enableExtensions);
bool CheckInstanceExtensionPropertiesSupport(const char ** _enableExtensions, int _count);

VkInstance CreateInstance(const std::vector<const char*> enableLayers = {}, const std::vector<const char*> _enableExtensions = {});

//physical device info
std::unique_ptr<std::vector<VkPhysicalDevice>> GetPhysicalDevices(VkInstance _instance);
std::unique_ptr<VkPhysicalDeviceProperties> GetPhysicalDeviceProperties(VkPhysicalDevice _physicalDevice);
std::unique_ptr<std::vector<VkExtensionProperties>> GetPhysicalDeviceExtensionProperties(VkPhysicalDevice _physicalDevice);
std::unique_ptr<std::vector<VkQueueFamilyProperties>> GetPhysicalDeviceQueueFamilyProperties(
        VkPhysicalDevice _physicalDevice);
std::unique_ptr<VkPhysicalDeviceFeatures> GetPhysicalDeviceFeatures(VkPhysicalDevice _physicalDevice);
std::unique_ptr<VkPhysicalDeviceMemoryProperties> GetPhysicalDeviceMemoryProperties(VkPhysicalDevice _physicalDevice);
VkMemoryPropertyFlags GetBestMemoryPropertyFlags(VkMemoryPropertyFlags _must, 
        VkMemoryPropertyFlags _optional, const VkPhysicalDeviceMemoryProperties * _devicePorps);

// return queueFamilyIndex >= 0
// < 0 not support
int CheckPhysicalDeviceQueueFamilyPropertiesSupport(VkPhysicalDevice _physicalDevice, VkQueueFlags _propsFlag);
bool CheckPhsicalDeviceExtensionsSupport(VkPhysicalDevice _physicalDevice, const std::vector<const char*> _enableExtensions);

//shader module
VkShaderModule CreateShaderModule(VkDevice _device, const std::vector<char>& _code);
VkShaderModule CreateShaderModuleFromFile(VkDevice _device, const char * _fileName);

//on screen
int CheckPhysicalDeviceSurfaceSupport(VkPhysicalDevice _physicalDevice, VkSurfaceKHR _surface);
SwapChainSupportDetails QuerySwapChainSupport(VkPhysicalDevice _physicalDevice, VkSurfaceKHR _surface);
VkSurfaceFormatKHR GetProperSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
VkPresentModeKHR GetProperSwapPresentMode(const std::vector<VkPresentModeKHR> availablePresentModes);
VkExtent2D GetProperSwapChainExtent(VkSurfaceCapabilitiesKHR &surface_capabilities);
VkImageUsageFlags GetSwapSufaceImageUsageFlags(VkSurfaceCapabilitiesKHR &surface_capabilities);
VkSurfaceTransformFlagBitsKHR GetSwapChainTransform(VkSurfaceCapabilitiesKHR &surface_capabilities);


//create info
VkImageCreateInfo Get2DImageCreateInfo(uint32_t width, uint32_t height,
VkImageTiling _tiling, VkImageUsageFlags _usageFlags, VkFormat _format);
VkImageViewCreateInfo Get2DImageViewCreateInfo(VkImage _image, VkFormat _format);
VkSamplerCreateInfo GetSamplerCreateInfo();
VkCommandBufferBeginInfo GetCommandBufferOneTimeSubmitBeginInfo();

//barrier
VkImageMemoryBarrier GetDstSwapChainImageBeforeCopyMemoryBarrier(
uint32_t _presentQueue, uint32_t _graphicQueue, VkImage _image, VkImageSubresourceRange _range);
VkImageMemoryBarrier GetDstSwapChainImageAfterCopyMemoryBarrier(
        uint32_t _presentQueue, uint32_t _graphicQueue, VkImage _image, VkImageSubresourceRange _range);

VkImageMemoryBarrier GetSrcSwapChainImageBeforeCopyMemoryBarrier(
        uint32_t _presentQueue, uint32_t _graphicQueue, VkImage _image, VkImageSubresourceRange _range);
VkImageMemoryBarrier GetSrcSwapChainImageAfterCopyMemoryBarrier(
        uint32_t _presentQueue, uint32_t _graphicQueue, VkImage _image, VkImageSubresourceRange _range);

VkImageMemoryBarrier GetSwapChainImageBeforeRenderMemoryBarrier(
        uint32_t _presentQueue, uint32_t _graphicQueue, VkImage _image, VkImageSubresourceRange _range);
VkImageMemoryBarrier GetSwapChainImageAfterRenderMemoryBarrier(
        uint32_t _presentQueue, uint32_t _graphicQueue, VkImage _image, VkImageSubresourceRange _range);

VkImageMemoryBarrier GetSrcImageBeforeCopyMemoryBarrier(
        uint32_t _presentQueue, uint32_t _graphicQueue, VkImage _image, VkImageSubresourceRange _range);
VkImageMemoryBarrier GetSrcImageAfterCopyMemoryBarrier(
        uint32_t _presentQueue, uint32_t _graphicQueue, VkImage _image, VkImageSubresourceRange _range);

VkImageMemoryBarrier GetDstImageBeforeCopyMemoryBarrier(
        uint32_t _presentQueue, uint32_t _graphicQueue, VkImage _image, VkImageSubresourceRange _range);
VkImageMemoryBarrier GetDstImageAfterCopyMemoryBarrier(
        uint32_t _presentQueue, uint32_t _graphicQueue, VkImage _image, VkImageSubresourceRange _range);

VkImageMemoryBarrier GetImageBeforeRenderMemoryBarrier(
        uint32_t _presentQueue, uint32_t _graphicQueue, VkImage _image, VkImageSubresourceRange _range);
VkImageMemoryBarrier GetImageAfterRenderMemoryBarrier(
        uint32_t _presentQueue, uint32_t _graphicQueue, VkImage _image, VkImageSubresourceRange _range);


//tools
// not use now. because we do not use linear image
VkDeviceSize GetLinearImageRowPitch(VkDevice _device, VkImage _image);
