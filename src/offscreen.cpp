#include "offscreen.h"
#include <set>

// TODO m_renderingResources size
OffScreenVulkan::OffScreenVulkan(VkInstance _instance) :
    m_hInstance(_instance),
    m_hDebugCallback(VK_NULL_HANDLE),
    m_hValidationCallback(VK_NULL_HANDLE),
    m_graphicQueueParams(),
    m_hSelectedPhsicalDevice(VK_NULL_HANDLE),
    m_hLogicalDevice(VK_NULL_HANDLE),
    m_hCommandPool(VK_NULL_HANDLE)
{}

int OffScreenVulkan::SetupValidationLayerCallback(PFN_vkDebugReportCallbackEXT _pfnCallback)
{
    VkDebugReportCallbackCreateInfoEXT createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
    createInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;
    createInfo.pfnCallback = _pfnCallback;

    auto func = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(m_hInstance, "vkCreateDebugReportCallbackEXT");
    if (func != nullptr) {
        return func(m_hInstance, &createInfo, nullptr, &m_hDebugCallback);
    }
    return VK_ERROR_EXTENSION_NOT_PRESENT;
}


bool OffScreenVulkan::SelectProperPhysicalDeviceAndQueueFamily(const std::vector<const char*> _enableExtensions,
    VkPhysicalDeviceType _deviceType)
{
    auto physicalDevices = GetPhysicalDevices(m_hInstance);
    if (physicalDevices->size() <= 0)
        return false;

    VkPhysicalDevice selectedPhysicalDevice = VK_NULL_HANDLE;
    int selectedGraphicFamilyIdx = -1;

    for (int i = 0; i < physicalDevices->size(); i++) {
        VkPhysicalDevice physicalDevice = physicalDevices->operator[](i);

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
        if (rightDeviceType)
            break;

        return true;
    }
    m_graphicQueueParams.FamilyIndex = selectedGraphicFamilyIdx;
    m_hSelectedPhsicalDevice = selectedPhysicalDevice;
    return false;
}

void OffScreenVulkan::DetermineBestMemoryPropertyFlag()
{
    if (m_hSelectedPhsicalDevice == VK_NULL_HANDLE)
        return;
    
    std::unique_ptr<VkPhysicalDeviceMemoryProperties> memProps =
        GetPhysicalDeviceMemoryProperties(m_hSelectedPhsicalDevice);

    //TODO flag
    //m_memoryPropertyFlag
}

void OffScreenVulkan::CreateLogicalDevice( const std::vector<const char*> _enableLayers,
    const std::vector<const char*> _enableExtensions)
{
    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;

    float queuePriority = 1.0f;
    VkDeviceQueueCreateInfo queueCreateInfo = {};
    queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.queueFamilyIndex = m_graphicQueueParams.FamilyIndex;
    queueCreateInfo.queueCount = 1;
    queueCreateInfo.pQueuePriorities = &queuePriority;
    queueCreateInfos.push_back(queueCreateInfo);

    VkPhysicalDeviceFeatures deviceFeatures = {};

    VkDeviceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.pQueueCreateInfos = queueCreateInfos.data();

    createInfo.pEnabledFeatures = &deviceFeatures;

    std::vector<const char*> mustExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
    if (_enableExtensions.size() == 0) {
        createInfo.enabledExtensionCount = static_cast<uint32_t>(mustExtensions.size());
        createInfo.ppEnabledExtensionNames = mustExtensions.data();
    }
    else {
        createInfo.enabledExtensionCount = static_cast<uint32_t>(_enableExtensions.size());
        createInfo.ppEnabledExtensionNames = _enableExtensions.data();
    }

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
}

void OffScreenVulkan::AllocateCommandBuffers(VkCommandPool pool, uint32_t count, VkCommandBuffer *command_buffers) {
    VkCommandBufferAllocateInfo command_buffer_allocate_info = {
        VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,   // VkStructureType                sType
        nullptr,                                          // const void                    *pNext
        pool,                                             // VkCommandPool                  commandPool
        VK_COMMAND_BUFFER_LEVEL_PRIMARY,                  // VkCommandBufferLevel           level
        count                                             // uint32_t                       bufferCount
    };

    if (vkAllocateCommandBuffers(m_hLogicalDevice, &command_buffer_allocate_info, command_buffers) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate command buffer!");
    }
}

void OffScreenVulkan::createCommandPool(uint32_t _queueIndex) {
    VkCommandPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = _queueIndex;
    //VK_COMMAND_POOL_CREATE_TRANSIENT_BIT: 提示命令缓冲区非常频繁的重新记录新命令(可能会改变内存分配行为)
    //VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT : 允许命令缓冲区单独重新记录，没有这个标志，所有的命令缓冲区都必须一起重置
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT | VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;

    if (vkCreateCommandPool(m_hLogicalDevice, &poolInfo, nullptr, &m_hCommandPool) != VK_SUCCESS) {
        throw std::runtime_error("failed to create graphics command pool!");
    }
}

void OffScreenVulkan::createCommandBuffers() {
    for (size_t i = 0; i < m_renderingResources.size(); ++i) {
        AllocateCommandBuffers(m_hCommandPool, 1, &m_renderingResources[i].CommandBuffer);
    }
}

void OffScreenVulkan::createSemaphores() {
    VkSemaphoreCreateInfo semaphoreInfo = {};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    semaphoreInfo.pNext = nullptr;
    semaphoreInfo.flags = 0;

    for (size_t i = 0; i < m_renderingResources.size(); ++i) {
        if ((vkCreateSemaphore(m_hLogicalDevice, &semaphoreInfo, nullptr, &m_renderingResources[i].ImageAvailableSemaphore) != VK_SUCCESS) ||
            (vkCreateSemaphore(m_hLogicalDevice, &semaphoreInfo, nullptr, &m_renderingResources[i].FinishedRenderingSemaphore) != VK_SUCCESS)) {
            throw std::runtime_error("failed to create semaphores!");
        }
    }
}


void OffScreenVulkan::CreateRenderingResources(int _resourceCount) {
    if (m_renderingResources.size() > 0)
        return;
    m_renderingResources.resize(_resourceCount);
    // TODO presend and graphic in different queue index?
    createCommandPool(m_graphicQueueParams.FamilyIndex);
    createCommandBuffers();
    createSemaphores();
    createFences();
}

void OffScreenVulkan::createFences() {
    VkFenceCreateInfo fenceCreateInfo = {};
    fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceCreateInfo.pNext = nullptr;
    fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (size_t i = 0; i < m_renderingResources.size(); ++i) {
        if (vkCreateFence(m_hLogicalDevice, &fenceCreateInfo, nullptr, &m_renderingResources[i].Fence) != VK_SUCCESS) {
            throw std::runtime_error("Could not create a fence!");
        }
    }
}

void OffScreenVulkan::CreateStagingBuffer() {
    m_stagingBuffer.Size = 1920*1080*4;
    createBuffer(VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, m_stagingBuffer);
    AllocateCommandBuffers(m_hCommandPool, 1, &m_stagingBuffer.CommandBuffer);

    VkFenceCreateInfo fenceCreateInfo = {};
    fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceCreateInfo.pNext = nullptr;

    if (vkCreateFence(m_hLogicalDevice, &fenceCreateInfo, nullptr, &m_stagingBuffer.Fence) != VK_SUCCESS) {
        throw std::runtime_error("Could not create a fence!");
    }

    // Prepare command buffer to copy data from staging buffer to image 
    VkCommandBufferBeginInfo command_buffer_begin_info = {};
    command_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    vkBeginCommandBuffer(m_stagingBuffer.CommandBuffer, &command_buffer_begin_info);

    VkImageSubresourceRange image_subresource_range = {
        VK_IMAGE_ASPECT_COLOR_BIT,     // VkImageAspectFlags aspectMask
        0,                             // uint32_t           baseMipLevel
        1,                             // uint32_t           levelCount
        0,                             // uint32_t           baseArrayLayer
        1                              // uint32_t           layerCount
    };

    VkImageMemoryBarrier image_memory_barrier_from_undefined_to_transfer_dst = {
        VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,  // VkStructureType      sType
        nullptr,                                 // const void           *pNext
        0,                                       // VkAccessFlags        srcAccessMask
        VK_ACCESS_TRANSFER_WRITE_BIT,            // VkAccessFlags        dstAccessMask
        VK_IMAGE_LAYOUT_UNDEFINED,               // VkImageLayout        oldLayout
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,    // VkImageLayout        newLayout
        VK_QUEUE_FAMILY_IGNORED,                 // uint32_t             srcQueueFamilyIndex
        VK_QUEUE_FAMILY_IGNORED,                 // uint32_t             dstQueueFamilyIndex
        m_image.Handle,                          // VkImage              image
        image_subresource_range                  // VkImageSubresourceRange subresourceRange
    };
    vkCmdPipelineBarrier(m_stagingBuffer.CommandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &image_memory_barrier_from_undefined_to_transfer_dst);

    VkBufferImageCopy buffer_image_copy_info = {
        0,                                                  // VkDeviceSize                           bufferOffset
        0,                                                  // uint32_t                               bufferRowLength
        0,                                                  // uint32_t                               bufferImageHeight
        {                                                   // VkImageSubresourceLayers               imageSubresource
            VK_IMAGE_ASPECT_COLOR_BIT,                          // VkImageAspectFlags                     aspectMask
            0,                                                  // uint32_t                               mipLevel
            0,                                                  // uint32_t                               baseArrayLayer
            1                                                   // uint32_t                               layerCount
        },
        {                                                   // VkOffset3D                             imageOffset
            0,                                                  // int32_t                                x
            0,                                                  // int32_t                                y
            0                                                   // int32_t                                z
        },
        {                                                   // VkExtent3D                             imageExtent
            m_image.Extent3D.width,                         // uint32_t                               width
            m_image.Extent3D.height,                        // uint32_t                               height
            1                                                   // uint32_t                               depth
        }
    };
    vkCmdCopyBufferToImage(m_stagingBuffer.CommandBuffer, m_stagingBuffer.Handle, m_image.Handle, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &buffer_image_copy_info);

    VkImageMemoryBarrier image_memory_barrier_from_transfer_to_shader_read = {
        VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,     // VkStructureType      sType
        nullptr,                                    // const void           *pNext
        VK_ACCESS_TRANSFER_WRITE_BIT,               // VkAccessFlags        srcAccessMask
        VK_ACCESS_SHADER_READ_BIT,                  // VkAccessFlags        dstAccessMask
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,       // VkImageLayout        oldLayout
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,   // VkImageLayout        newLayout
        VK_QUEUE_FAMILY_IGNORED,                    // uint32_t             srcQueueFamilyIndex
        VK_QUEUE_FAMILY_IGNORED,                    // uint32_t             dstQueueFamilyIndex
        m_image.Handle,                             // VkImage              image
        image_subresource_range                     // VkImageSubresourceRange subresourceRange
    };
    vkCmdPipelineBarrier(m_stagingBuffer.CommandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &image_memory_barrier_from_transfer_to_shader_read);

    vkEndCommandBuffer(m_stagingBuffer.CommandBuffer);
}

void OffScreenVulkan::createBuffer(VkBufferUsageFlags usage,
    VkMemoryPropertyFlagBits memoryProperty, BufferParameters &buffer) {

    VkBufferCreateInfo buffer_create_info = {};
    buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_create_info.pNext = nullptr;
    buffer_create_info.flags = 0;
    buffer_create_info.size = buffer.Size;
    buffer_create_info.usage = usage;
    buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    buffer_create_info.queueFamilyIndexCount = 0;
    buffer_create_info.pQueueFamilyIndices = nullptr;

    if (vkCreateBuffer(m_hLogicalDevice, &buffer_create_info, nullptr, &buffer.Handle) != VK_SUCCESS) {
        throw std::runtime_error("Could not create buffer!");
    }

    allocateBufferMemory(buffer.Handle, memoryProperty, &buffer.Memory);

    if (vkBindBufferMemory(m_hLogicalDevice, buffer.Handle, buffer.Memory, 0) != VK_SUCCESS) {
        throw std::runtime_error("Could not bind memory to a buffer!");
    }
}

void OffScreenVulkan::allocateBufferMemory(VkBuffer buffer, VkMemoryPropertyFlagBits property, VkDeviceMemory *memory) {
    VkMemoryRequirements buffer_memory_requirements;
    vkGetBufferMemoryRequirements(m_hLogicalDevice, buffer, &buffer_memory_requirements);

    VkPhysicalDeviceMemoryProperties memory_properties;
    vkGetPhysicalDeviceMemoryProperties(m_hSelectedPhsicalDevice, &memory_properties);

    for (uint32_t i = 0; i < memory_properties.memoryTypeCount; ++i) {
        if ((buffer_memory_requirements.memoryTypeBits & (1 << i)) &&
            (memory_properties.memoryTypes[i].propertyFlags & property)) {

            VkMemoryAllocateInfo memory_allocate_info = {
                VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,     // VkStructureType                        sType
                nullptr,                                    // const void                            *pNext
                buffer_memory_requirements.size,            // VkDeviceSize                           allocationSize
                i                                           // uint32_t                               memoryTypeIndex
            };

            if (vkAllocateMemory(m_hLogicalDevice, &memory_allocate_info, nullptr, memory) != VK_SUCCESS) {
                throw std::runtime_error("Could not allock memory!");
            }
        }
    }
}

void OffScreenVulkan::CreateImage(uint32_t width, uint32_t height, VkImage *image, VkFormat _format)
{
    VkImageCreateInfo image_create_info = Get2DImageCreateInfo(width, height,
        VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        _format);

    if (vkCreateImage(m_hLogicalDevice, &image_create_info, nullptr, image) != VK_SUCCESS) {
        throw std::runtime_error("Could create image!");
    }
}
void OffScreenVulkan::allocateImageMemory(ImageParameters &image, VkMemoryPropertyFlagBits property)
{
    VkMemoryRequirements image_memory_requirements;
    vkGetImageMemoryRequirements(m_hLogicalDevice, image.Handle, &image_memory_requirements);

    VkPhysicalDeviceMemoryProperties memory_properties;
    vkGetPhysicalDeviceMemoryProperties(m_hSelectedPhsicalDevice, &memory_properties);

    for (uint32_t i = 0; i < memory_properties.memoryTypeCount; ++i) {
        if ((image_memory_requirements.memoryTypeBits & (1 << i)) &&
            (memory_properties.memoryTypes[i].propertyFlags & property)) {

            VkMemoryAllocateInfo memory_allocate_info = {
                VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,     // VkStructureType                        sType
                nullptr,                                    // const void                            *pNext
                image_memory_requirements.size,             // VkDeviceSize                           allocationSize
                i                                           // uint32_t                               memoryTypeIndex
            };

            if (vkAllocateMemory(m_hLogicalDevice, &memory_allocate_info, nullptr, &image.Memory) != VK_SUCCESS) {
                throw std::runtime_error("Could allocate image memory!");
            }
        }
    }
    image.Requirements = image_memory_requirements;
    return ;
}

void OffScreenVulkan::createImageView(ImageParameters &image_parameters) {
    VkImageViewCreateInfo image_view_create_info = Get2DImageViewCreateInfo(
        image_parameters.Handle, image_parameters.Format);

    if (vkCreateImageView(m_hLogicalDevice, &image_view_create_info, nullptr, &image_parameters.View) != VK_SUCCESS) {
        throw std::runtime_error("Could create image view!");
    }
}

void OffScreenVulkan::createSampler(VkSampler *sampler) {
    VkSamplerCreateInfo sampler_create_info = GetSamplerCreateInfo();

    if(vkCreateSampler(m_hLogicalDevice, &sampler_create_info, nullptr, sampler) != VK_SUCCESS){
        throw std::runtime_error("Could create image view!");
    }
}

void OffScreenVulkan::CreateTexture(uint32_t data_size, int _width,
    int _height, VkFormat _format) {

    CreateImage(_width, _height, &m_image.Handle, _format);

    allocateImageMemory(m_image, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    if (vkBindImageMemory(m_hLogicalDevice, m_image.Handle, m_image.Memory, 0) != VK_SUCCESS) {
        throw std::runtime_error("Could not bind memory to an image!");
    }
    m_image.Format = _format;
    m_image.Extent3D = { (uint32_t)_width, (uint32_t)_height, 1 };
    createImageView(m_image);
    createSampler(&m_image.Sampler);
}

void OffScreenVulkan::CopyTextureData(char *texture_data, uint32_t data_size, uint32_t width, uint32_t height) {
    // Prepare data in staging buffer

    void *staging_buffer_memory_pointer;
    if (vkMapMemory(m_hLogicalDevice, m_stagingBuffer.Memory, 0, data_size, 0, &staging_buffer_memory_pointer) != VK_SUCCESS) {
        throw std::runtime_error("Could not map memory of staging buffer!");
    }

    memcpy(staging_buffer_memory_pointer, texture_data, data_size);

    VkMappedMemoryRange flush_range = {
        VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,  // VkStructureType                        sType
        nullptr,                                // const void                            *pNext
        m_stagingBuffer.Memory,                 // VkDeviceMemory                         memory
        0,                                      // VkDeviceSize                           offset
        data_size                               // VkDeviceSize                           size
    };
    vkFlushMappedMemoryRanges(m_hLogicalDevice, 1, &flush_range);

    vkUnmapMemory(m_hLogicalDevice, m_stagingBuffer.Memory);


    // Submit command buffer and copy data from staging buffer to a vertex buffer
    VkSubmitInfo submit_info = {
        VK_STRUCTURE_TYPE_SUBMIT_INFO,                      // VkStructureType                        sType
        nullptr,                                            // const void                            *pNext
        0,                                                  // uint32_t                               waitSemaphoreCount
        nullptr,                                            // const VkSemaphore                     *pWaitSemaphores
        nullptr,                                            // const VkPipelineStageFlags            *pWaitDstStageMask;
        1,                                                  // uint32_t                               commandBufferCount
        &m_stagingBuffer.CommandBuffer,                     // const VkCommandBuffer                 *pCommandBuffers
        0,                                                  // uint32_t                               signalSemaphoreCount
        nullptr                                             // const VkSemaphore                     *pSignalSemaphores
    };

    if (vkQueueSubmit(m_graphicQueueParams.Handle, 1, &submit_info, m_stagingBuffer.Fence) != VK_SUCCESS) {
        throw std::runtime_error("Could not submit to queue!");
    }

    vkDeviceWaitIdle(m_hLogicalDevice);
    if (vkWaitForFences(m_hLogicalDevice, 1, &m_stagingBuffer.Fence, VK_FALSE, 10000000) != VK_SUCCESS) {
        throw std::runtime_error("waiting for fence takes tool long!");
    }
    vkResetFences(m_hLogicalDevice, 1, &m_stagingBuffer.Fence);

    return;
}

void OffScreenVulkan::CreateDescriptorSetLayout() {
    VkDescriptorSetLayoutBinding layout_binding = {
        0,                                            // uint32_t          binding
        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,    // VkDescriptorType  descriptorType
        1,                                            // uint32_t          descriptorCount
        VK_SHADER_STAGE_FRAGMENT_BIT,                 // VkShaderStageFlags stageFlags
        nullptr                                       // const VkSampler    *pImmutableSamplers
    };

    VkDescriptorSetLayoutCreateInfo descriptor_set_layout_create_info = {
        VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,  // VkStructureType                      sType
        nullptr,                                              // const void                          *pNext
        0,                                                    // VkDescriptorSetLayoutCreateFlags     flags
        1,                                                    // uint32_t                             bindingCount
        &layout_binding                                       // const VkDescriptorSetLayoutBinding  *pBindings
    };

    if (vkCreateDescriptorSetLayout(m_hLogicalDevice, &descriptor_set_layout_create_info, nullptr, &m_descriptorSetParams.Layout) != VK_SUCCESS) {
        throw std::runtime_error("Could not create descriptor set layout!");
    }
    return;
}

void OffScreenVulkan::CreateDescriptorPool() {
    VkDescriptorPoolSize pool_size = {
        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,      // VkDescriptorType   type
        1                                               // uint32_t           descriptorCount
    };

    VkDescriptorPoolCreateInfo descriptor_pool_create_info = {
        VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,  // VkStructureType                sType
        nullptr,                                        // const void                    *pNext
        0,                                              // VkDescriptorPoolCreateFlags    flags
        1,                                              // uint32_t                       maxSets
        1,                                              // uint32_t                       poolSizeCount
        &pool_size                                      // const VkDescriptorPoolSize    *pPoolSizes
    };

    if (vkCreateDescriptorPool(m_hLogicalDevice, &descriptor_pool_create_info,
        nullptr, &m_descriptorSetParams.Pool) != VK_SUCCESS) {
        throw std::runtime_error("Could not create descriptor pool!");
    }
    return;
}

void OffScreenVulkan::AllocateDescriptorSet() {
    VkDescriptorSetAllocateInfo descriptor_set_allocate_info = {
        VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO, // VkStructureType                sType
        nullptr,                                        // const void                    *pNext
        m_descriptorSetParams.Pool,                      // VkDescriptorPool               descriptorPool
        1,                                              // uint32_t                       descriptorSetCount
        &m_descriptorSetParams.Layout                    // const VkDescriptorSetLayout   *pSetLayouts
    };

    if (vkAllocateDescriptorSets(m_hLogicalDevice, &descriptor_set_allocate_info,
        &m_descriptorSetParams.Handle) != VK_SUCCESS) {
        throw std::runtime_error("Could not allocate descriptor set!");
    }

    return;
}

void OffScreenVulkan::UpdateDescriptorSet() {
    VkDescriptorImageInfo image_info = {
        m_image.Sampler,                           // VkSampler     sampler
        m_image.View,                              // VkImageView   imageView
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL   // VkImageLayout imageLayout
    };

    // write the details of our UBO buffer into binding 0
    VkWriteDescriptorSet descriptor_writes = {
        VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,         // VkStructureType                sType
        nullptr,                                        // const void                    *pNext
        m_descriptorSetParams.Handle,                   // VkDescriptorSet                dstSet
        0,                                              // uint32_t                       dstBinding
        0,                                              // uint32_t                       dstArrayElement
        1,                                              // uint32_t                       descriptorCount
        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,      // VkDescriptorType               descriptorType
        &image_info,                                    // const VkDescriptorImageInfo   *pImageInfo
        nullptr,                                        // const VkDescriptorBufferInfo  *pBufferInfo
        nullptr                                         // const VkBufferView            *pTexelBufferView
    };

    //认为这个函数只是把VkDescriptorSet和实际资源关联起来？
    vkUpdateDescriptorSets(m_hLogicalDevice, 1, &descriptor_writes, 0, nullptr);

}

void OffScreenVulkan::CreateRenderPass(VkFormat _format) {
    VkAttachmentDescription attachment_descriptions[] = {
        {
            0,                                          // VkAttachmentDescriptionFlags   flags
            _format,                                    // VkFormat                       format
            VK_SAMPLE_COUNT_1_BIT,                      // VkSampleCountFlagBits          samples
            VK_ATTACHMENT_LOAD_OP_CLEAR,                // VkAttachmentLoadOp             loadOp
            VK_ATTACHMENT_STORE_OP_STORE,               // VkAttachmentStoreOp            storeOp
            VK_ATTACHMENT_LOAD_OP_DONT_CARE,            // VkAttachmentLoadOp             stencilLoadOp
            VK_ATTACHMENT_STORE_OP_DONT_CARE,           // VkAttachmentStoreOp            stencilStoreOp
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,   // VkImageLayout                  initialLayout;
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL    // VkImageLayout                  finalLayout
        }
    };

    VkAttachmentReference color_attachment_references[] = {
        {
            0,                                          // uint32_t                       attachment
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL    // VkImageLayout                  layout
        }
    };

    VkSubpassDescription subpass_descriptions[] = {
        {
            0,                                          // VkSubpassDescriptionFlags      flags
            VK_PIPELINE_BIND_POINT_GRAPHICS,            // VkPipelineBindPoint            pipelineBindPoint
            0,                                          // uint32_t                       inputAttachmentCount
            nullptr,                                    // const VkAttachmentReference   *pInputAttachments
            1,                                          // uint32_t                       colorAttachmentCount
            color_attachment_references,                // const VkAttachmentReference   *pColorAttachments
            nullptr,                                    // const VkAttachmentReference   *pResolveAttachments
            nullptr,                                    // const VkAttachmentReference   *pDepthStencilAttachment
            0,                                          // uint32_t                       preserveAttachmentCount
            nullptr                                     // const uint32_t*                pPreserveAttachments
        }
    };

    VkRenderPassCreateInfo render_pass_create_info = {
        VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,    // VkStructureType                sType
        nullptr,                                      // const void                    *pNext
        0,                                            // VkRenderPassCreateFlags        flags
        1,                                            // uint32_t                       attachmentCount
        attachment_descriptions,                      // const VkAttachmentDescription *pAttachments
        1,                                            // uint32_t                       subpassCount
        subpass_descriptions,                         // const VkSubpassDescription    *pSubpasses
        0,                                            // uint32_t                       dependencyCount
        nullptr                                       // const VkSubpassDependency     *pDependencies
    };

    if (vkCreateRenderPass(m_hLogicalDevice, &render_pass_create_info, nullptr, &m_renderPass) != VK_SUCCESS) {
        throw std::runtime_error("Could not create rendper pass!");
    }
}

void OffScreenVulkan::CreatePipelineLayout() {
    VkPipelineLayoutCreateInfo layout_create_info = {
        VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,  // VkStructureType                sType
        nullptr,                                        // const void                    *pNext
        0,                                              // VkPipelineLayoutCreateFlags    flags
        1,                                              // uint32_t                       setLayoutCount
        &m_descriptorSetParams.Layout,                   // const VkDescriptorSetLayout   *pSetLayouts
        0,                                              // uint32_t                       pushConstantRangeCount
        nullptr                                         // const VkPushConstantRange     *pPushConstantRanges
    };

    if (vkCreatePipelineLayout(m_hLogicalDevice, &layout_create_info, nullptr, &m_pipelineLayout) != VK_SUCCESS) {
        throw std::runtime_error("Could not create pipeline layout!");
    }
}

void OffScreenVulkan::CreatePipeline() {
    //TODO destory shader module
    VkShaderModule vertex_shader_module = CreateShaderModuleFromFile(m_hLogicalDevice, "/Users/liuye/vulkan/gpumix/src/Data06/vert.spv");
    VkShaderModule fragment_shader_module = CreateShaderModuleFromFile(m_hLogicalDevice, "/Users/liuye/vulkan/gpumix/src/Data06/frag.spv");

    std::vector<VkPipelineShaderStageCreateInfo> shader_stage_create_infos = {
        // Vertex shader
        {
            VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,        // VkStructureType                                sType
            nullptr,                                                    // const void                                    *pNext
            0,                                                          // VkPipelineShaderStageCreateFlags               flags
            VK_SHADER_STAGE_VERTEX_BIT,                                 // VkShaderStageFlagBits                          stage
            vertex_shader_module,                                 // VkShaderModule                                 module
            "main",                                                     // const char                                    *pName
            nullptr                                                     // const VkSpecializationInfo                    *pSpecializationInfo
        },
        // Fragment shader
        {
            VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,        // VkStructureType                                sType
            nullptr,                                                    // const void                                    *pNext
            0,                                                          // VkPipelineShaderStageCreateFlags               flags
            VK_SHADER_STAGE_FRAGMENT_BIT,                               // VkShaderStageFlagBits                          stage
            fragment_shader_module,                               // VkShaderModule                                 module
            "main",                                                     // const char                                    *pName
            nullptr                                                     // const VkSpecializationInfo                    *pSpecializationInfo
        }
    };

    VkVertexInputBindingDescription vertex_binding_description = {
        0,                             // uint32_t             binding
        sizeof(Vertex),            // uint32_t             stride
        VK_VERTEX_INPUT_RATE_VERTEX    // VkVertexInputRate    inputRate
    };

    VkVertexInputAttributeDescription vertex_attribute_descriptions[] = {
        {
            0,                                  // uint32_t    location
            vertex_binding_description.binding, // uint32_t    binding
            VK_FORMAT_R32G32_SFLOAT,            // VkFormat    format
            0                                   // uint32_t    offset
        },
        {
            1,                                    // uint32_t                                       location
            vertex_binding_description.binding,   // uint32_t                                       binding
            VK_FORMAT_R32G32_SFLOAT,              // VkFormat                                       format
            sizeof(glm::vec2)                     // uint32_t                                       offset
        }
    };

    VkPipelineVertexInputStateCreateInfo vertex_input_state_create_info = {
        VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,    // VkStructureType                                sType
        nullptr,                                                      // const void                                    *pNext
        0,                                                            // VkPipelineVertexInputStateCreateFlags          flags;
        1,                                                            // uint32_t                                       vertexBindingDescriptionCount
        &vertex_binding_description,                                  // const VkVertexInputBindingDescription         *pVertexBindingDescriptions
        2,                                                            // uint32_t                                       vertexAttributeDescriptionCount
        vertex_attribute_descriptions                                 // const VkVertexInputAttributeDescription       *pVertexAttributeDescriptions
    };

    VkPipelineInputAssemblyStateCreateInfo input_assembly_state_create_info = {
        VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,  // VkStructureType                                sType
        nullptr,                                                      // const void                                    *pNext
        0,                                                            // VkPipelineInputAssemblyStateCreateFlags        flags
        VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP,                         // VkPrimitiveTopology                            topology
        VK_FALSE                                                      // VkBool32                                       primitiveRestartEnable
    };

    VkPipelineViewportStateCreateInfo viewport_state_create_info = {
        VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,        // VkStructureType                                sType
        nullptr,                                                      // const void                                    *pNext
        0,                                                            // VkPipelineViewportStateCreateFlags             flags
        1,                                                            // uint32_t                                       viewportCount
        nullptr,                                                      // const VkViewport                              *pViewports
        1,                                                            // uint32_t                                       scissorCount
        nullptr                                                       // const VkRect2D                                *pScissors
    };

    VkPipelineRasterizationStateCreateInfo rasterization_state_create_info = {
        VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,   // VkStructureType                                sType
        nullptr,                                                      // const void                                    *pNext
        0,                                                            // VkPipelineRasterizationStateCreateFlags        flags
        VK_FALSE,                                                     // VkBool32                                       depthClampEnable
        VK_FALSE,                                                     // VkBool32                                       rasterizerDiscardEnable
        VK_POLYGON_MODE_FILL,                                         // VkPolygonMode                                  polygonMode
        VK_CULL_MODE_BACK_BIT,                                        // VkCullModeFlags                                cullMode
        VK_FRONT_FACE_COUNTER_CLOCKWISE,                              // VkFrontFace                                    frontFace
        VK_FALSE,                                                     // VkBool32                                       depthBiasEnable
        0.0f,                                                         // float                                          depthBiasConstantFactor
        0.0f,                                                         // float                                          depthBiasClamp
        0.0f,                                                         // float                                          depthBiasSlopeFactor
        1.0f                                                          // float                                          lineWidth
    };

    VkPipelineMultisampleStateCreateInfo multisample_state_create_info = {
        VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,     // VkStructureType                                sType
        nullptr,                                                      // const void                                    *pNext
        0,                                                            // VkPipelineMultisampleStateCreateFlags          flags
        VK_SAMPLE_COUNT_1_BIT,                                        // VkSampleCountFlagBits                          rasterizationSamples
        VK_FALSE,                                                     // VkBool32                                       sampleShadingEnable
        1.0f,                                                         // float                                          minSampleShading
        nullptr,                                                      // const VkSampleMask                            *pSampleMask
        VK_FALSE,                                                     // VkBool32                                       alphaToCoverageEnable
        VK_FALSE                                                      // VkBool32                                       alphaToOneEnable
    };

    VkPipelineColorBlendAttachmentState color_blend_attachment_state = {
        VK_FALSE,                                                     // VkBool32                                       blendEnable
        VK_BLEND_FACTOR_ONE,                                          // VkBlendFactor                                  srcColorBlendFactor
        VK_BLEND_FACTOR_ZERO,                                         // VkBlendFactor                                  dstColorBlendFactor
        VK_BLEND_OP_ADD,                                              // VkBlendOp                                      colorBlendOp
        VK_BLEND_FACTOR_ONE,                                          // VkBlendFactor                                  srcAlphaBlendFactor
        VK_BLEND_FACTOR_ZERO,                                         // VkBlendFactor                                  dstAlphaBlendFactor
        VK_BLEND_OP_ADD,                                              // VkBlendOp                                      alphaBlendOp
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |         // VkColorComponentFlags                          colorWriteMask
        VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT
    };

    VkPipelineColorBlendStateCreateInfo color_blend_state_create_info = {
        VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,     // VkStructureType                                sType
        nullptr,                                                      // const void                                    *pNext
        0,                                                            // VkPipelineColorBlendStateCreateFlags           flags
        VK_FALSE,                                                     // VkBool32                                       logicOpEnable
        VK_LOGIC_OP_COPY,                                             // VkLogicOp                                      logicOp
        1,                                                            // uint32_t                                       attachmentCount
        &color_blend_attachment_state,                                // const VkPipelineColorBlendAttachmentState     *pAttachments
        { 0.0f, 0.0f, 0.0f, 0.0f }                                    // float                                          blendConstants[4]
    };

    VkDynamicState dynamic_states[] = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR,
    };

    VkPipelineDynamicStateCreateInfo dynamic_state_create_info = {
        VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,         // VkStructureType                                sType
        nullptr,                                                      // const void                                    *pNext
        0,                                                            // VkPipelineDynamicStateCreateFlags              flags
        2,                                                            // uint32_t                                       dynamicStateCount
        dynamic_states                                                // const VkDynamicState                          *pDynamicStates
    };

    VkGraphicsPipelineCreateInfo pipeline_create_info = {
        VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,              // VkStructureType                                sType
        nullptr,                                                      // const void                                    *pNext
        0,                                                            // VkPipelineCreateFlags                          flags
        static_cast<uint32_t>(shader_stage_create_infos.size()),      // uint32_t                                       stageCount
        &shader_stage_create_infos[0],                                // const VkPipelineShaderStageCreateInfo         *pStages
        &vertex_input_state_create_info,                              // const VkPipelineVertexInputStateCreateInfo    *pVertexInputState;
        &input_assembly_state_create_info,                            // const VkPipelineInputAssemblyStateCreateInfo  *pInputAssemblyState
        nullptr,                                                      // const VkPipelineTessellationStateCreateInfo   *pTessellationState
        &viewport_state_create_info,                                  // const VkPipelineViewportStateCreateInfo       *pViewportState
        &rasterization_state_create_info,                             // const VkPipelineRasterizationStateCreateInfo  *pRasterizationState
        &multisample_state_create_info,                               // const VkPipelineMultisampleStateCreateInfo    *pMultisampleState
        nullptr,                                                      // const VkPipelineDepthStencilStateCreateInfo   *pDepthStencilState
        &color_blend_state_create_info,                               // const VkPipelineColorBlendStateCreateInfo     *pColorBlendState
        &dynamic_state_create_info,                                   // const VkPipelineDynamicStateCreateInfo        *pDynamicState
        m_pipelineLayout,                                        // VkPipelineLayout                               layout
        m_renderPass,                                            // VkRenderPass                                   renderPass
        0,                                                            // uint32_t                                       subpass
        VK_NULL_HANDLE,                                               // VkPipeline                                     basePipelineHandle
        -1                                                            // int32_t                                        basePipelineIndex
    };

    if (vkCreateGraphicsPipelines(m_hLogicalDevice, VK_NULL_HANDLE, 1, &pipeline_create_info, nullptr, &m_graphicsPipeline) != VK_SUCCESS) {
        throw std::runtime_error("Could not create grdphic pipeline!");
    }
}

void OffScreenVulkan::CreateVertexBuffer(int size) {
    m_vertexBuffer.Size = static_cast<uint32_t>(size);
    createBuffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_vertexBuffer);
}

void OffScreenVulkan::UpdateVertexData(const char * _data, int _size) {

    void *staging_buffer_memory_pointer;
    if (vkMapMemory(m_hLogicalDevice, m_stagingBuffer.Memory, 0, m_vertexBuffer.Size, 0, &staging_buffer_memory_pointer) != VK_SUCCESS) {
        throw std::runtime_error("Could map staging buffer!");
    }

    memcpy(staging_buffer_memory_pointer, _data, _size);// m_vertexBuffer.Size);

    VkMappedMemoryRange flush_range = {
        VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,              // VkStructureType                        sType
        nullptr,                                            // const void                            *pNext
        m_stagingBuffer.Memory,                        // VkDeviceMemory                         memory
        0,                                                  // VkDeviceSize                           offset
        m_vertexBuffer.Size                            // VkDeviceSize                           size
    };
    vkFlushMappedMemoryRanges(m_hLogicalDevice, 1, &flush_range);

    vkUnmapMemory(m_hLogicalDevice, m_stagingBuffer.Memory);

    // Prepare command buffer to copy data from staging buffer to a vertex buffer
    VkCommandBufferBeginInfo command_buffer_begin_info = GetCommandBufferOneTimeSubmitBeginInfo();

    VkCommandBuffer command_buffer = m_renderingResources[0].CommandBuffer;

    vkBeginCommandBuffer(command_buffer, &command_buffer_begin_info);

    VkBufferCopy buffer_copy_info = {
        0,                      // VkDeviceSize                           srcOffset
        0,                      // VkDeviceSize                           dstOffset
        m_vertexBuffer.Size     // VkDeviceSize                           size
    };
    vkCmdCopyBuffer(command_buffer, m_stagingBuffer.Handle, m_vertexBuffer.Handle, 1, &buffer_copy_info);

    VkBufferMemoryBarrier buffer_memory_barrier = {
        VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,            // VkStructureType                        sType;
        nullptr,                                            // const void                            *pNext
        VK_ACCESS_MEMORY_WRITE_BIT,                         // VkAccessFlags                          srcAccessMask
        VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT,                // VkAccessFlags                          dstAccessMask
        VK_QUEUE_FAMILY_IGNORED,                            // uint32_t                               srcQueueFamilyIndex
        VK_QUEUE_FAMILY_IGNORED,                            // uint32_t                               dstQueueFamilyIndex
        m_vertexBuffer.Handle,                         // VkBuffer                               buffer
        0,                                                  // VkDeviceSize                           offset
        VK_WHOLE_SIZE                                       // VkDeviceSize                           size
    };
    vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, 0, 0, nullptr, 1, &buffer_memory_barrier, 0, nullptr);

    vkEndCommandBuffer(command_buffer);

    // Submit command buffer and copy data from staging buffer to a vertex buffer
    VkSubmitInfo submit_info = {
        VK_STRUCTURE_TYPE_SUBMIT_INFO,                      // VkStructureType                        sType
        nullptr,                                            // const void                            *pNext
        0,                                                  // uint32_t                               waitSemaphoreCount
        nullptr,                                            // const VkSemaphore                     *pWaitSemaphores
        nullptr,                                            // const VkPipelineStageFlags            *pWaitDstStageMask;
        1,                                                  // uint32_t                               commandBufferCount
        &command_buffer,                                    // const VkCommandBuffer                 *pCommandBuffers
        0,                                                  // uint32_t                               signalSemaphoreCount
        nullptr                                             // const VkSemaphore                     *pSignalSemaphores
    };

    if (vkQueueSubmit(m_graphicQueueParams.Handle, 1, &submit_info, VK_NULL_HANDLE) != VK_SUCCESS) {
        throw std::runtime_error("Could not submit to queue!");
    }

    vkDeviceWaitIdle(m_hLogicalDevice);
}

void OffScreenVulkan::CreateRenderTarget(uint32_t width, uint32_t height, VkFormat _format)
{
    createRenderTarget(m_renderTarget.Target, width, height, _format,
        VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    m_renderTarget.Staging.Size = width * height * 4.1;
    createBuffer(VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, m_renderTarget.Staging);
    AllocateCommandBuffers(m_hCommandPool, 1, &m_renderTarget.Staging.CommandBuffer);

    VkFenceCreateInfo fenceCreateInfo = {};
    fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceCreateInfo.pNext = nullptr;

    if (vkCreateFence(m_hLogicalDevice, &fenceCreateInfo, nullptr, &m_renderTarget.Staging.Fence) != VK_SUCCESS) {
        throw std::runtime_error("Could not create a fence!");
    }
    AllocateCommandBuffers(m_hCommandPool, 1, &m_renderTarget.Staging.CommandBuffer);

    VkCommandBufferBeginInfo command_buffer_begin_info = {};
    command_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    if (vkBeginCommandBuffer(m_renderTarget.Staging.CommandBuffer, &command_buffer_begin_info) != VK_SUCCESS) {
        std::runtime_error("could not begin command buffer");
    }
    //---------------

    VkImageSubresourceRange image_subresource_range = {
        VK_IMAGE_ASPECT_COLOR_BIT,     // VkImageAspectFlags aspectMask
        0,                             // uint32_t           baseMipLevel
        1,                             // uint32_t           levelCount
        0,                             // uint32_t           baseArrayLayer
        1                              // uint32_t           layerCount
    };
    VkImageMemoryBarrier srcImageBeforCopyBarrier =
        GetSrcImageBeforeCopyMemoryBarrier(VK_QUEUE_FAMILY_IGNORED,
            VK_QUEUE_FAMILY_IGNORED, m_renderTarget.Target.Handle, image_subresource_range);
    VkImageMemoryBarrier beforeBarriers[1] = { srcImageBeforCopyBarrier };

    vkCmdPipelineBarrier(m_renderTarget.Staging.CommandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr,
        1, beforeBarriers);

    VkImageSubresourceLayers imageSubresource{};
    imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    imageSubresource.layerCount = 1;
    VkBufferImageCopy bufferImageCopy{};
    bufferImageCopy.bufferOffset = 0;
    bufferImageCopy.bufferRowLength = width;
    bufferImageCopy.bufferImageHeight = height;
    bufferImageCopy.imageSubresource = imageSubresource;
    bufferImageCopy.imageOffset = { 0,0,0 };
    bufferImageCopy.imageExtent = { (uint32_t)width, (uint32_t)height, 1 };

    vkCmdCopyImageToBuffer(m_renderTarget.Staging.CommandBuffer,
        m_renderTarget.Target.Handle, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        m_renderTarget.Staging.Handle,
        1, &bufferImageCopy);

    VkImageMemoryBarrier srcImageAfterCopyBarrier =
        GetSrcImageAfterCopyMemoryBarrier(VK_QUEUE_FAMILY_IGNORED,
            VK_QUEUE_FAMILY_IGNORED, m_renderTarget.Target.Handle, image_subresource_range);
    VkImageMemoryBarrier afterBarriers[1] = { srcImageAfterCopyBarrier };
    vkCmdPipelineBarrier(m_renderTarget.Staging.CommandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr,
        1, afterBarriers);

    if(vkEndCommandBuffer(m_renderTarget.Staging.CommandBuffer) != VK_SUCCESS){
        std::runtime_error("could not begin command buffer");
    }
}

void OffScreenVulkan::createRenderTarget(ImageParameters& _image, uint32_t width,
    uint32_t height, VkFormat _format, VkImageTiling _tiling,
    VkImageUsageFlags _usageFlags, VkMemoryPropertyFlagBits _memProps)
{
    VkImageCreateInfo image_create_info = Get2DImageCreateInfo(width, height, 
        _tiling, _usageFlags, _format);
    if (vkCreateImage(m_hLogicalDevice, &image_create_info, nullptr, &_image.Handle) != VK_SUCCESS) {
        throw std::runtime_error("Could create image!");
    }
    _image.Format = _format;
    _image.Extent3D = { width, height, 1 };

    allocateImageMemory(_image, _memProps);

    if (vkBindImageMemory(m_hLogicalDevice, _image.Handle, _image.Memory, 0) != VK_SUCCESS) {
        throw std::runtime_error("Could not bind memory to an image!");
    }

    createImageView(_image);
    createSampler(&_image.Sampler);
}

int OffScreenVulkan::ReadPixels(uint8_t * _buffer, int _len)
{
    copyRenderTargetToStagingBUffer();

    int len = m_renderTarget.Target.Extent3D.width *  m_renderTarget.Target.Extent3D.height * 4;
    void *map;
    if (vkMapMemory(m_hLogicalDevice, m_renderTarget.Staging.Memory, 0, len, 0, &map) != VK_SUCCESS) {
        throw std::runtime_error("Could not map memory of staging buffer!");
    }
    memcpy(_buffer, map, len);
    vkUnmapMemory(m_hLogicalDevice, m_renderTarget.Staging.Memory);
    return len;
}

void OffScreenVulkan::copyRenderTargetToStagingBUffer()
{

    // Submit command buffer and copy data from staging buffer to a vertex buffer
    VkSubmitInfo submit_info = {
        VK_STRUCTURE_TYPE_SUBMIT_INFO,    // VkStructureType           sType
        nullptr,                          // const void                *pNext
        0,                                // uint32_t                  waitSemaphoreCount
        nullptr,                          // const VkSemaphore         *pWaitSemaphores
        nullptr,                          // const VkPipelineStageFlags *pWaitDstStageMask;
        1,                                // uint32_t                  commandBufferCount
        &m_renderTarget.Staging.CommandBuffer, // const VkCommandBuffer     *pCommandBuffers
        0,                                // uint32_t                  signalSemaphoreCount
        nullptr                           // const VkSemaphore         *pSignalSemaphores
    };

    if (vkQueueSubmit(m_graphicQueueParams.Handle, 1, &submit_info, VK_NULL_HANDLE) != VK_SUCCESS) {
        throw std::runtime_error("Could not submit to queue!");
    }

    vkDeviceWaitIdle(m_hLogicalDevice);
    return;
}
