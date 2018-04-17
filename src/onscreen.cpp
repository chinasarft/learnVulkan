#include "onscreen.h"
#include <set>

OnScreenVulkan::OnScreenVulkan(VkInstance _instance, VkSurfaceKHR _surface):
    OffScreenVulkan(_instance),
    m_hSurface(_surface),
    m_presentQueueParams(),
    m_swapChainParams()
{

}

bool OnScreenVulkan::SelectProperPhysicalDeviceAndQueueFamily(const std::vector<const char*> _enableExtensions,
    VkPhysicalDeviceType _deviceType)
{
    auto physicalDevices = GetPhysicalDevices(m_hInstance);
    if (physicalDevices->size() <= 0)
        return false;

    VkPhysicalDevice selectedPhysicalDevice = VK_NULL_HANDLE;
    int selectedPresentFamilyIdx = -1;
    int selectedGraphicFamilyIdx = -1;

    bool found = false;
    for (int i = 0; i < physicalDevices->size(); i++) {
        VkPhysicalDevice physicalDevice = physicalDevices->operator[](i);

        SwapChainSupportDetails details = QuerySwapChainSupport(physicalDevice, m_hSurface);
        if (!(details.formats.size() > 0 && details.presentModes.size() > 0))
            continue;

        int presentFamilyIdx = CheckPhysicalDeviceSurfaceSupport(physicalDevice, m_hSurface);
        if (presentFamilyIdx < 0)
            continue;

        bool extensionsSupported = CheckPhsicalDeviceExtensionsSupport(physicalDevice, _enableExtensions);
        if (!extensionsSupported)
            continue;

        int graphicsFamilyIdx = CheckPhysicalDeviceQueueFamilyPropertiesSupport(physicalDevice, VK_QUEUE_GRAPHICS_BIT);
        if (graphicsFamilyIdx < 0)
            continue;

        auto prop = GetPhysicalDeviceProperties(physicalDevice);
        bool rightDeviceType = !!(prop->deviceType & _deviceType);

        selectedPhysicalDevice = physicalDevice;
        selectedGraphicFamilyIdx = graphicsFamilyIdx;
        selectedPresentFamilyIdx = presentFamilyIdx;
        found = true;
        if (rightDeviceType) {
            break;
        }
    }
    if (found) {
        m_presentQueueParams.FamilyIndex = selectedPresentFamilyIdx;
        m_graphicQueueParams.FamilyIndex = selectedGraphicFamilyIdx;
        m_hSelectedPhsicalDevice = selectedPhysicalDevice;
        GetPhysicalDeviceMemoryProperties(selectedPhysicalDevice);
    }
    return found;
}

void OnScreenVulkan::CreateLogicalDevice(const std::vector<const char*> _enableExtensions,
        const std::vector<const char*> _enableLayers)
{
    std::set<uint32_t> uniqueQueueFamilies = {
        m_graphicQueueParams.FamilyIndex,
        m_presentQueueParams.FamilyIndex 
    };

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;

    float queuePriority = 1.0f;
    for (uint32_t queueFamilyIdx : uniqueQueueFamilies) {
        VkDeviceQueueCreateInfo queueCreateInfo = {};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamilyIdx;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }

    VkPhysicalDeviceFeatures deviceFeatures = {};

    VkDeviceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.pQueueCreateInfos = queueCreateInfos.data();

    createInfo.pEnabledFeatures = &deviceFeatures;

    createInfo.enabledExtensionCount = static_cast<uint32_t>(_enableExtensions.size());
    createInfo.ppEnabledExtensionNames = _enableExtensions.data();

    if (_enableLayers.size() > 0) {
        createInfo.enabledLayerCount = static_cast<uint32_t>(_enableLayers.size());
        createInfo.ppEnabledLayerNames = _enableLayers.data();
    }
    else {
        createInfo.enabledLayerCount = 0;
    }

    if (vkCreateDevice(m_hSelectedPhsicalDevice, &createInfo, nullptr, &m_hLogicalDevice) != VK_SUCCESS) {
        throw std::runtime_error("failed to create logical device!");
    }

    vkGetDeviceQueue(m_hLogicalDevice, m_graphicQueueParams.FamilyIndex,
        0, &m_graphicQueueParams.Handle);
    vkGetDeviceQueue(m_hLogicalDevice, m_presentQueueParams.FamilyIndex,
        0, &m_presentQueueParams.Handle);
}

void OnScreenVulkan::CreateSwapChain() {
    SwapChainSupportDetails swapChainSupport = QuerySwapChainSupport(m_hSelectedPhsicalDevice, m_hSurface);

    VkSurfaceFormatKHR surfaceFormat = GetProperSwapSurfaceFormat(swapChainSupport.formats);
    VkPresentModeKHR presentMode = GetProperSwapPresentMode(swapChainSupport.presentModes);
    VkExtent2D extent = GetProperSwapChainExtent(swapChainSupport.capabilities);
    VkImageUsageFlags desireImageUsage = GetSwapSufaceImageUsageFlags(swapChainSupport.capabilities);
    if (static_cast<int>(desireImageUsage) == -1) {
        throw std::runtime_error("no proper image usage flag!");
    }
    VkSurfaceTransformFlagBitsKHR desiredTransform = GetSwapChainTransform(swapChainSupport.capabilities);
    VkSwapchainKHR oldSwapChain = m_swapChainParams.Handle;


    //minImageCount 值提供交换链正常运行所需的最少图像, 一般都是2
    //如果2+1就是三缓冲了
    uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
    if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
        imageCount = swapChainSupport.capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = m_hSurface;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = desireImageUsage;
#ifdef USE_VK_SHARING_MODE_CONCURRENT
    uint32_t queueFamilyIndices[] = {
        m_graphicQueueParams.FamilyIndex,
        m_presentQueueParams.FamilyIndex,
    };
    if (m_graphicQueueParams.FamilyIndex != m_presentQueueParams.FamilyIndex) {
        // if imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        // queueFamilyIndexCount and pQueueFamilyIndices will igonred
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    }
#else
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
#endif
    createInfo.preTransform = desiredTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = oldSwapChain;

    if (vkCreateSwapchainKHR(m_hLogicalDevice, &createInfo, nullptr, &m_swapChainParams.Handle) != VK_SUCCESS) {
        throw std::runtime_error("failed to create swap chain!");
    }

    if (oldSwapChain != VK_NULL_HANDLE) {
        vkDestroySwapchainKHR(m_hLogicalDevice, oldSwapChain, nullptr);
    }


    vkGetSwapchainImagesKHR(m_hLogicalDevice, m_swapChainParams.Handle, &imageCount, nullptr);
    m_swapChainParams.Images.resize(imageCount);
    std::vector<VkImage> images(imageCount);
    if (vkGetSwapchainImagesKHR(m_hLogicalDevice, m_swapChainParams.Handle, &imageCount, &images[0]) != VK_SUCCESS) {
        std::runtime_error("Could not get swap chain images!");
    }
    for (size_t i = 0; i < m_swapChainParams.Images.size(); ++i) {
        m_swapChainParams.Images[i].Handle = images[i];
        m_swapChainParams.Images[i].Format = surfaceFormat.format;
    }

    m_swapChainParams.Format = surfaceFormat.format;
    m_swapChainParams.Extent = extent;

    createImageViews();
}

void OnScreenVulkan::ReCreateSwapChain()
{
    if (m_hLogicalDevice != VK_NULL_HANDLE) {
        vkDeviceWaitIdle(m_hLogicalDevice);
    }

    for (size_t i = 0; i < m_swapChainParams.Images.size(); ++i) {
        if (m_swapChainParams.Images[i].View != VK_NULL_HANDLE) {
            vkDestroyImageView(m_hLogicalDevice, m_swapChainParams.Images[i].View,
                nullptr);
            m_swapChainParams.Images[i].View = VK_NULL_HANDLE;
        }
    }
    m_swapChainParams.Images.clear();
    CreateSwapChain();
}

int OnScreenVulkan::GetSwapChainBackBufferSize() {
    int s = (int)m_swapChainParams.Images.size();
    if (s <= 0) {
        std::runtime_error("should create swapchain first");
    }
    return s;
}

void OnScreenVulkan::createImageViews() {
    for (size_t i = 0; i < m_swapChainParams.Images.size(); i++) {
        VkImageViewCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.image = m_swapChainParams.Images[i].Handle;
        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.format = m_swapChainParams.Format;
        createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        createInfo.subresourceRange.baseMipLevel = 0;
        createInfo.subresourceRange.levelCount = 1;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount = 1;

        if (vkCreateImageView(m_hLogicalDevice, &createInfo, nullptr, &m_swapChainParams.Images[i].View) != VK_SUCCESS) {
            throw std::runtime_error("failed to create image views!");
        }
    }
}

void OnScreenVulkan::createFramebuffer(VkFramebuffer &framebuffer, VkImageView image_view) {
    if (framebuffer != VK_NULL_HANDLE) {
        vkDestroyFramebuffer(m_hLogicalDevice, framebuffer, nullptr);
        framebuffer = VK_NULL_HANDLE;
    }

    VkFramebufferCreateInfo framebuffer_create_info = {
        VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO, // VkStructureType                sType
        nullptr,                                   // const void                    *pNext
        0,                                         // VkFramebufferCreateFlags       flags
        m_renderPass,                              // VkRenderPass                   renderPass
        1,                                         // uint32_t                       attachmentCount
        &image_view,                               // const VkImageView             *pAttachments
        m_swapChainParams.Extent.width,            // uint32_t                       width
        m_swapChainParams.Extent.height,           // uint32_t                       height
        1                                          // uint32_t                       layers
    };

    if (vkCreateFramebuffer(m_hLogicalDevice, &framebuffer_create_info, nullptr, &framebuffer) != VK_SUCCESS) {
        std::runtime_error("Could not create a framebuffer!");
    }
    return;
}

void OnScreenVulkan::CreateRenderPass()
{
    this->OffScreenVulkan::CreateRenderPass(m_swapChainParams.Format);
}

void OnScreenVulkan::Draw() {
    static size_t           resource_index = 0;
    RenderingResourcesData &current_rendering_resource = m_renderingResources[resource_index];
    VkSwapchainKHR          swap_chain = m_swapChainParams.Handle;
    uint32_t                image_index;

    resource_index = (resource_index + 1) % m_renderingResources.size();

    if (vkWaitForFences(m_hLogicalDevice, 1, &current_rendering_resource.Fence, VK_FALSE, 1000000000) != VK_SUCCESS) {
        throw std::runtime_error("waiting for fence takes tool long!");
    }
    vkResetFences(m_hLogicalDevice, 1, &current_rendering_resource.Fence);

    VkResult result = vkAcquireNextImageKHR(m_hLogicalDevice, swap_chain, UINT64_MAX,
        current_rendering_resource.ImageAvailableSemaphore, VK_NULL_HANDLE, &image_index);
    switch (result) {
    case VK_SUCCESS:
        break;
    case VK_SUBOPTIMAL_KHR:
    case VK_ERROR_OUT_OF_DATE_KHR:
        if(result == VK_SUBOPTIMAL_KHR)
            std::cout << "maybe recreate swap chain:" << result<<std::endl;
        else
            std::cout << "should recreate swap chain:" << result<<std::endl;
        //return OnWindowSizeChanged();
        break;
    default:
        throw std::runtime_error("Problem occurred during swap chain image acquisition!");
    }

    PrepareFrame(current_rendering_resource.CommandBuffer,
        m_swapChainParams.Images[image_index], current_rendering_resource.Framebuffer);

    VkPipelineStageFlags wait_dst_stage_mask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    VkSubmitInfo submit_info = {
        VK_STRUCTURE_TYPE_SUBMIT_INFO,                          // VkStructureType              sType
        nullptr,                                                // const void                  *pNext
        1,                                                      // uint32_t                     waitSemaphoreCount
        &current_rendering_resource.ImageAvailableSemaphore,    // const VkSemaphore           *pWaitSemaphores
        &wait_dst_stage_mask,                                   // const VkPipelineStageFlags  *pWaitDstStageMask;
        1,                                                      // uint32_t                     commandBufferCount
        &current_rendering_resource.CommandBuffer,              // const VkCommandBuffer       *pCommandBuffers
        1,                                                      // uint32_t                     signalSemaphoreCount
        &current_rendering_resource.FinishedRenderingSemaphore  // const VkSemaphore           *pSignalSemaphores
    };

    if (vkQueueSubmit(m_graphicQueueParams.Handle, 1, &submit_info, current_rendering_resource.Fence) != VK_SUCCESS) {
        throw std::runtime_error("Could not submit to queue(draw)!");
    }

    VkPresentInfoKHR present_info = {
        VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,                     // VkStructureType              sType
        nullptr,                                                // const void                  *pNext
        1,                                                      // uint32_t                     waitSemaphoreCount
        &current_rendering_resource.FinishedRenderingSemaphore, // const VkSemaphore           *pWaitSemaphores
        1,                                                      // uint32_t                     swapchainCount
        &swap_chain,                                            // const VkSwapchainKHR        *pSwapchains
        &image_index,                                           // const uint32_t              *pImageIndices
        nullptr                                                 // VkResult                    *pResults
    };
    result = vkQueuePresentKHR(m_presentQueueParams.Handle, &present_info);

    switch (result) {
    case VK_SUCCESS:
        break;
    case VK_ERROR_OUT_OF_DATE_KHR:
    case VK_SUBOPTIMAL_KHR:
        if(result == VK_SUBOPTIMAL_KHR)
            std::cout << "maybe1 recreate swap chain:" << result<<std::endl;
        else
            std::cout << "should1 recreate swap chain:" << result<<std::endl;
        //return OnWindowSizeChanged();
    default:
        throw std::runtime_error("Problem occurred during image presentation!");
    }
    return;
}


void OnScreenVulkan::PrepareFrame(VkCommandBuffer command_buffer,
    SwapChainImages &image_parameters, VkFramebuffer &framebuffer) {

    VkExtent2D extent = { 1280, 720 };

    //CreateFramebuffer(framebuffer, m_renderTarget.View);
    if (framebuffer == VK_NULL_HANDLE) {

        VkFramebufferCreateInfo framebuffer_create_info = {
            VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO, // VkStructureType                sType
            nullptr,                                   // const void                    *pNext
            0,                                         // VkFramebufferCreateFlags       flags
            m_renderPass,                              // VkRenderPass                   renderPass
            1,                                         // uint32_t                       attachmentCount
            &m_renderTarget.Target.View,                      // const VkImageView             *pAttachments
            extent.width,            // uint32_t               width
            extent.height,           // uint32_t                 height
            1                                          // uint32_t                       layers
        };
        if (vkCreateFramebuffer(m_hLogicalDevice, &framebuffer_create_info, nullptr, &framebuffer) != VK_SUCCESS) {
            std::runtime_error("Could not create a framebuffer!");
        }
    }


    VkCommandBufferBeginInfo command_buffer_begin_info = {
        VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,        // VkStructureType                        sType
        nullptr,                                            // const void                            *pNext
        VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,        // VkCommandBufferUsageFlags              flags
        nullptr                                             // const VkCommandBufferInheritanceInfo  *pInheritanceInfo
    };

    vkBeginCommandBuffer(command_buffer, &command_buffer_begin_info);

    VkImageSubresourceRange image_subresource_range = {
        VK_IMAGE_ASPECT_COLOR_BIT,                          // VkImageAspectFlags                     aspectMask
        0,                                                  // uint32_t                               baseMipLevel
        1,                                                  // uint32_t                               levelCount
        0,                                                  // uint32_t                               baseArrayLayer
        1                                                   // uint32_t                               layerCount
    };

    uint32_t present_queue_family_index = (m_presentQueueParams.Handle != m_graphicQueueParams.Handle) ? m_presentQueueParams.FamilyIndex : VK_QUEUE_FAMILY_IGNORED;
    uint32_t graphics_queue_family_index = (m_presentQueueParams.Handle != m_graphicQueueParams.Handle) ? m_graphicQueueParams.FamilyIndex : VK_QUEUE_FAMILY_IGNORED;
    VkImageMemoryBarrier barrier_from_srcimage_to_draw = GetImageBeforeRenderMemoryBarrier(
            present_queue_family_index, graphics_queue_family_index, m_renderTarget.Target.Handle, image_subresource_range);
    vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, 0,
        nullptr, 0, nullptr, 1, &barrier_from_srcimage_to_draw);

    VkClearValue clear_value = {
        { {1.0f, 0.8f, 0.4f, 0.0f} },      // VkClearColorValue color
    };

    VkRenderPassBeginInfo render_pass_begin_info = {
        VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,    // VkStructureType    sType
        nullptr,                                     // const void         *pNext
        m_renderPass,                                // VkRenderPass       renderPass
        framebuffer,                                 // VkFramebuffer      framebuffer
        {                                            // VkRect2D           renderArea
            {                                        // VkOffset2D         offset
                0,                                   // int32_t            x
                0                                    // int32_t            y
            },
        extent,                    // VkExtent2D         extent;
        },
        1,                                           // uint32_t           clearValueCount
        &clear_value                                 // const VkClearValue *pClearValues
    };

    vkCmdBeginRenderPass(command_buffer, &render_pass_begin_info,
        VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
        m_graphicsPipeline);

    VkViewport viewport = {
        0.0f,                                               // float                                  x
        0.0f,                                               // float                                  y
        static_cast<float>(extent.width),    // float                                  width
        static_cast<float>(extent.height),   // float                                  height
        0.0f,                                               // float                                  minDepth
        1.0f                                                // float                                  maxDepth
    };

    VkRect2D scissor = {
        {                                                   // VkOffset2D                             offset
            0,                                                  // int32_t                                x
            0                                                   // int32_t                                y
        },
        {                                                   // VkExtent2D                             extent
            extent.width,                        // uint32_t                               width
            extent.height                        // uint32_t                               height
        }
    };

    vkCmdSetViewport(command_buffer, 0, 1, &viewport);
    vkCmdSetScissor(command_buffer, 0, 1, &scissor);

    VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(command_buffer, 0, 1, &m_vertexBuffer.Handle, &offset);

    vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 0, 1, &m_descriptorSetParams.Handle, 0, nullptr);

    vkCmdDraw(command_buffer, 4, 1, 0, 0);

    vkCmdEndRenderPass(command_buffer);

    VkImageMemoryBarrier barrier_from_draw_to_read = GetImageAfterRenderMemoryBarrier(
            present_queue_family_index, graphics_queue_family_index, m_renderTarget.Target.Handle, image_subresource_range);
    vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, nullptr,
        0, nullptr, 1, &barrier_from_draw_to_read);

    copyImageToSwapchain(m_renderTarget.Target, image_parameters, command_buffer);

    if (vkEndCommandBuffer(command_buffer) != VK_SUCCESS) {
        std::runtime_error("Could not record command buffer!");
    }
    return;
}

void OnScreenVulkan::copyImageToSwapchain(ImageParameters &src, SwapChainImages&dst, VkCommandBuffer command_buffer)
{

    uint32_t present_queue_family_index = (m_presentQueueParams.Handle != m_graphicQueueParams.Handle) ? m_presentQueueParams.FamilyIndex : VK_QUEUE_FAMILY_IGNORED;
    uint32_t graphics_queue_family_index = (m_presentQueueParams.Handle != m_graphicQueueParams.Handle) ? m_graphicQueueParams.FamilyIndex : VK_QUEUE_FAMILY_IGNORED;
    VkImageSubresourceRange image_subresource_range = {
        VK_IMAGE_ASPECT_COLOR_BIT,   // VkImageAspectFlags  aspectMask
        0,                           // uint32_t            baseMipLevel
        1,                           // uint32_t            levelCount
        0,                           // uint32_t            baseArrayLayer
        1                            // uint32_t            layerCount
    };
    VkImageMemoryBarrier srcBeforeBarrier = GetSrcImageBeforeCopyMemoryBarrier(
            present_queue_family_index, graphics_queue_family_index, src.Handle, image_subresource_range);
    VkImageMemoryBarrier dstBeforeBarrier = GetDstSwapChainImageBeforeCopyMemoryBarrier(
            present_queue_family_index, graphics_queue_family_index, dst.Handle, image_subresource_range);
    VkImageMemoryBarrier barrier_from_srcimage_to_draw[2] = { srcBeforeBarrier, dstBeforeBarrier };

    vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0,
        nullptr, 0, nullptr, 2, barrier_from_srcimage_to_draw);


    VkOffset3D srcBlitSize;
    srcBlitSize.x = 1280;
    srcBlitSize.y = 720;
    srcBlitSize.z = 1;
    VkOffset3D dstBlitSize;
    dstBlitSize.x = m_swapChainParams.Extent.width;
    dstBlitSize.y = m_swapChainParams.Extent.height;
    dstBlitSize.z = 1;
    VkImageBlit imageBlitRegion{};
    imageBlitRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    imageBlitRegion.srcSubresource.layerCount = 1;
    imageBlitRegion.srcOffsets[1] = srcBlitSize;
    imageBlitRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    imageBlitRegion.dstSubresource.layerCount = 1;
    imageBlitRegion.dstOffsets[1] = dstBlitSize;

    // Issue the blit command
    vkCmdBlitImage(
        command_buffer,
        src.Handle, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        dst.Handle, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1,
        &imageBlitRegion,
        VK_FILTER_NEAREST);

    VkImageMemoryBarrier srcAfterBarrier = GetSrcImageAfterCopyMemoryBarrier(
            present_queue_family_index, graphics_queue_family_index, src.Handle, image_subresource_range);
    VkImageMemoryBarrier dstAfterBarrier = GetDstSwapChainImageAfterCopyMemoryBarrier(
            present_queue_family_index, graphics_queue_family_index, dst.Handle, image_subresource_range);
    VkImageMemoryBarrier barrier_from_draw_to_srcimage[2] = { srcAfterBarrier, dstAfterBarrier };

    vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0,
        nullptr, 0, nullptr, 2, barrier_from_draw_to_srcimage);
    return;
}
