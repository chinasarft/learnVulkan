#pragma once
#include "offscreen.h"

struct SwapChainImages{
    VkImage                       Handle;
    VkImageView                   View;
    VkFormat                      Format;
    VkExtent3D                    Extent3D;

    SwapChainImages() :
        Handle(VK_NULL_HANDLE),
        View(VK_NULL_HANDLE),
        Format(VK_FORMAT_UNDEFINED),
        Extent3D{ 0,0,0 } {
    }
};

struct SwapChainParameters {
    VkSwapchainKHR                Handle;
    VkFormat                      Format;
    //View Memory and Requirments is invalid
    std::vector<SwapChainImages>  Images;
    VkExtent2D                    Extent;

    SwapChainParameters() :
        Handle(VK_NULL_HANDLE),
        Format(VK_FORMAT_UNDEFINED),
        Images(),
        Extent() {
    }
};


class OnScreenVulkan : public OffScreenVulkan {
public:
    OnScreenVulkan(VkInstance _instance, VkSurfaceKHR _surface);
    // 会获取VK_QUEUE_GRAPHICS_BIT和present family index
    virtual bool SelectProperPhysicalDeviceAndQueueFamily(const std::vector<const char*> _enableExtensions, VkPhysicalDeviceType _deviceType = VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU);
    void CreateLogicalDevice( const std::vector<const char*> _enableLayer,
        const std::vector<const char*> _enableExtensions = {} );
    void CreateSwapChain();
    void ReCreateSwapChain();
    int GetSwapChainBackBufferSize();
    void Draw();
    void CreateRenderPass();
    
protected:
    void PrepareFrame(VkCommandBuffer command_buffer,
        SwapChainImages &image_parameters, VkFramebuffer &framebuffer);

private:
    void copyImageToSwapchain(ImageParameters &src, SwapChainImages&dst, VkCommandBuffer command_buffer);
    void createFramebuffer(VkFramebuffer &framebuffer, VkImageView image_view);
    void createImageViews();

private:
    VkSurfaceKHR m_hSurface;
    QueueParameters m_presentQueueParams;
    SwapChainParameters m_swapChainParams;
};
