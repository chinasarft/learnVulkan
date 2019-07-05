#pragma once
#include "helper.h"
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

struct ImageParameters {
    VkImage                       Handle;
    VkImageView                   View;
    VkSampler                     Sampler;
    VkDeviceMemory                Memory;
    VkFormat                      Format;
    VkMemoryRequirements          Requirements;
    VkExtent3D                    Extent3D;

    ImageParameters() :
        Handle(VK_NULL_HANDLE),
        View(VK_NULL_HANDLE),
        Sampler(VK_NULL_HANDLE),
        Memory(VK_NULL_HANDLE),
        Format(VK_FORMAT_UNDEFINED),
        Requirements{ 0,0,0 },
        Extent3D{ 0,0,0 } {
    }
};

struct RenderingResourcesData {
    VkFramebuffer                         Framebuffer;
    VkCommandBuffer                       CommandBuffer;
    VkSemaphore                           ImageAvailableSemaphore;
    VkSemaphore                           FinishedRenderingSemaphore;
    VkFence                               Fence;

    RenderingResourcesData() :
        Framebuffer(VK_NULL_HANDLE),
        CommandBuffer(VK_NULL_HANDLE),
        ImageAvailableSemaphore(VK_NULL_HANDLE),
        FinishedRenderingSemaphore(VK_NULL_HANDLE),
        Fence(VK_NULL_HANDLE) {
    }
};

struct BufferParameters {
    VkBuffer                        Handle;
    VkDeviceMemory                  Memory;
    uint32_t                        Size;
    VkCommandBuffer                 CommandBuffer;
    VkFence                         Fence;

    BufferParameters() :
        Handle(VK_NULL_HANDLE),
        Memory(VK_NULL_HANDLE),
        Size(0) {
    }
};

struct RenderTargetParameters {
    ImageParameters Target;
    BufferParameters Staging; //for read pixels
};

//TODO is need
/*
如果内存同时host visible 和device local还有必要staging吗?
从GetPhysicalDeviceMemoryPropertie来看，很多属性都是
 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
 所以创建stagebuffer和image的memoryproperty可以在selectphysicaldevice的时候预先获取最佳
 的内存propertyfla：g
*/
struct BestMemoryPropertyParameters {
    VkMemoryPropertyFlags RenderTarget;
    VkMemoryPropertyFlags Texture;
    VkMemoryPropertyFlags Vertex;
    VkMemoryPropertyFlags Stage;
    BestMemoryPropertyParameters() :
        RenderTarget(0),
        Texture(0),
        Vertex(0),
        Stage(0)
    {}
};

struct Vertex {
    glm::vec2 pos;
    glm::vec2 texCoord;
};

struct MVP{
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
};

struct DescriptorSetParameters {
    VkDescriptorPool                Pool;
    VkDescriptorSetLayout           Layout;
    VkDescriptorSet                 Handle;

    DescriptorSetParameters() :
        Pool(VK_NULL_HANDLE),
        Layout(VK_NULL_HANDLE),
        Handle(VK_NULL_HANDLE) {
    }
};

class OffScreenVulkan {
public:
    OffScreenVulkan(VkInstance _instance);
    // 这个需要支持validation layer的扩展，所以最后和这里联动
    int SetupValidationLayerCallback(PFN_vkDebugReportCallbackEXT _pfnCallback);

    virtual bool SelectProperPhysicalDeviceAndQueueFamily(const std::vector<const char*> _enableExtensions, VkPhysicalDeviceType _deviceType = VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU);
    void DetermineBestMemoryPropertyFlag();
    void CreateLogicalDevice(const std::vector<const char*> _enableExtensions,
        const std::vector<const char*> _enableLayer);
    void AllocateCommandBuffers(VkCommandPool pool, uint32_t count, VkCommandBuffer *command_buffers);
    void CreateRenderingResources(int _resourceCount = 3);
    void CreateStagingBuffer();
    void CreateImage(uint32_t width, uint32_t height, VkImage *image, VkFormat _format);

    void CreateTexture(uint32_t data_size, int _width, int _height, VkFormat _format);
    void CopyTextureData(char *texture_data, uint32_t data_size, uint32_t width, uint32_t height);

    void CreateDescriptorSetLayout();
    void CreateDescriptorPool();
    void AllocateDescriptorSet();

    void UpdateDescriptorSet();
    void CreateRenderPass(VkFormat _format);

    void CreatePipelineLayout();
    void CreatePipeline();
    void CreateVertexBuffer(int size);
    void UpdateVertexData(const char * data, int size);

    void CreateRenderTarget(uint32_t widht, uint32_t height, VkFormat _format);

    void PrepareFrame(VkCommandBuffer command_buffer,
                    ImageParameters &image_parameters, VkFramebuffer &framebuffer);
    void Draw();
    int ReadPixels(uint8_t * _buffer, int _len);

protected:
    void createCommandPool(uint32_t _queueIndex);
    void createCommandBuffers();
    void createSemaphores();
    void createFences();

    void createBuffer(VkBufferUsageFlags usage, VkMemoryPropertyFlagBits memoryProperty, BufferParameters &buffer);
    void allocateBufferMemory(VkBuffer buffer, VkMemoryPropertyFlagBits property, VkDeviceMemory *memory);
    void allocateImageMemory(ImageParameters& _image, VkMemoryPropertyFlagBits property);
    void createImageView(ImageParameters &image_parameters);
    void createSampler(VkSampler *sampler);
    void createRenderTarget(ImageParameters& _image,uint32_t widht, uint32_t height,
        VkFormat _format, VkImageTiling _tiling, VkImageUsageFlags _usageFlags,
        VkMemoryPropertyFlagBits _memProps);
    void copyRenderTargetToStagingBUffer();

protected:
    VkInstance m_hInstance;
    VkDebugReportCallbackEXT m_hDebugCallback;
    VkDebugReportCallbackEXT m_hValidationCallback;
    QueueParameters m_graphicQueueParams;
    VkPhysicalDevice m_hSelectedPhsicalDevice;
    VkDevice m_hLogicalDevice;
    VkCommandPool m_hCommandPool;
    std::vector<RenderingResourcesData>  m_renderingResources;
    BufferParameters m_stagingBuffer;
    ImageParameters m_image;
    DescriptorSetParameters m_descriptorSetParams;
    VkRenderPass                          m_renderPass;
    VkPipelineLayout                      m_pipelineLayout;
    VkPipeline                            m_graphicsPipeline;
    BufferParameters                      m_vertexBuffer;
    BestMemoryPropertyParameters m_memoryPropertyFlag;
    RenderTargetParameters m_renderTarget;
};
