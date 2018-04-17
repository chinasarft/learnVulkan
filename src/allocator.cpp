#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"

VmaAllocatorCreateInfo allocatorInfo;

void a() {
    //allocatorInfo.physicalDevice = physicalDevice;
    //allocatorInfo.device = device;
    VmaAllocator allocator;

    vmaCreateAllocator(&allocatorInfo, &allocator);

    VkBufferCreateInfo bufferInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
    bufferInfo.size = 65536;
    bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    VmaAllocationCreateInfo allocInfo = {};
    allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    VkBuffer buffer;
    VmaAllocation allocation;
    vmaCreateBuffer(allocator, &bufferInfo, &allocInfo, &buffer, &allocation, nullptr);

    vmaCreateImage;
}