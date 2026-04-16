#pragma once

#include "vulkan_types.h"
#include <GLFW/glfw3.h>
#include <vector>
#include <string>

struct TextureInfo {
    VkImage image;
    VkImageView imageView;
    VkSampler sampler;
    VkDeviceMemory imageMemory;
    uint32_t width;
    uint32_t height;
};

class VulkanDevice {
public:
    void createInstance();
    void createSurface(GLFWwindow* window, VkInstance instance, VkSurfaceKHR* surface);
    void pickPhysicalDevice(VkInstance instance, VkSurfaceKHR surface);
    void createLogicalDevice(VkSurfaceKHR surface);
    void cleanup(VkInstance instance, VkSurfaceKHR surface);

    VkInstance getInstance() const { return instance; }
    VkSurfaceKHR getSurface() const { return surface; }
    VkPhysicalDevice getPhysicalDevice() const { return physicalDevice; }
    VkDevice getDevice() const { return device; }
    VkQueue getGraphicsQueue() const { return graphicsQueue; }
    VkQueue getPresentQueue() const { return presentQueue; }

private:
    VkInstance instance;
    VkSurfaceKHR surface;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice device;
    VkQueue graphicsQueue;
    VkQueue presentQueue;

    bool checkValidationLayerSupport();
    std::vector<const char*> getRequiredExtensions();
    bool isDeviceSuitable(VkPhysicalDevice device, VkSurfaceKHR surface);
    bool checkDeviceExtensionSupport(VkPhysicalDevice device);
    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface);
    SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device, VkSurfaceKHR surface);
};

class VulkanSwapChain {
public:
    void create(VkDevice device, VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, 
                GLFWwindow* window, VkRenderPass renderPass);
    void cleanup(VkDevice device);
    void recreate(VkDevice device, VkPhysicalDevice physicalDevice, VkSurfaceKHR surface,
                  GLFWwindow* window, VkRenderPass renderPass);

    VkSwapchainKHR getSwapChain() const { return swapChain; }
    VkFormat getImageFormat() const { return swapChainImageFormat; }
    VkExtent2D getExtent() const { return swapChainExtent; }
    const std::vector<VkImage>& getImages() const { return swapChainImages; }
    const std::vector<VkImageView>& getImageViews() const { return swapChainImageViews; }
    const std::vector<VkFramebuffer>& getFramebuffers() const { return swapChainFramebuffers; }

private:
    VkSwapchainKHR swapChain;
    std::vector<VkImage> swapChainImages;
    VkFormat swapChainImageFormat;
    VkExtent2D swapChainExtent;
    std::vector<VkImageView> swapChainImageViews;
    std::vector<VkFramebuffer> swapChainFramebuffers;

    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, GLFWwindow* window);
    void createImageViews(VkDevice device);
    void createFramebuffers(VkDevice device, VkRenderPass renderPass);
};

class VulkanPipeline {
public:
    void create(VkDevice device, VkRenderPass renderPass, VkExtent2D swapChainExtent);
    void cleanup(VkDevice device);

    VkPipeline getPipeline() const { return graphicsPipeline; }
    VkPipelineLayout getLayout() const { return pipelineLayout; }
    VkRenderPass getRenderPass() const { return renderPass; }

protected:
    VkPipelineLayout pipelineLayout;
    VkPipeline graphicsPipeline;
    VkRenderPass renderPass;

    static std::vector<char> readFile(const std::string& filename);
    VkShaderModule createShaderModule(VkDevice device, const std::vector<char>& code);
    virtual void createRenderPass(VkDevice device, VkFormat swapChainImageFormat);
    virtual void createGraphicsPipeline(VkDevice device, VkExtent2D swapChainExtent);
};

class VulkanTexturePipeline : public VulkanPipeline {
public:
    void create(VkDevice device, VkRenderPass renderPass, VkExtent2D swapChainExtent);
    void loadTexture(VkDevice device, VkPhysicalDevice physicalDevice, VkQueue graphicsQueue,
                     VkCommandPool commandPool, const std::string& texturePath, TextureInfo& textureInfo);
    void cleanupTextures(VkDevice device, TextureInfo& textureInfo);

private:
    void createRenderPass(VkDevice device, VkFormat swapChainImageFormat) override;
    void createGraphicsPipeline(VkDevice device, VkExtent2D swapChainExtent) override;
    void createImage(VkDevice device, VkPhysicalDevice physicalDevice, uint32_t width, uint32_t height,
                     VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage,
                     VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory);
    VkImageView createImageView(VkDevice device, VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);
    void transitionImageLayout(VkDevice device, VkQueue graphicsQueue, VkCommandPool commandPool,
                               VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);
    void copyBufferToImage(VkDevice device, VkQueue graphicsQueue, VkCommandPool commandPool,
                           VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);
    void createBuffer(VkDevice device, VkPhysicalDevice physicalDevice, VkDeviceSize size,
                      VkBufferUsageFlags usage, VkMemoryPropertyFlags properties,
                      VkBuffer& buffer, VkDeviceMemory& bufferMemory);
    uint32_t findMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter,
                            VkMemoryPropertyFlags properties);
    void createTextureSampler(VkDevice device, VkSampler& sampler);
};

class VulkanCommandBuffer {
public:
    void create(VkDevice device, VkPhysicalDevice physicalDevice, VkSurfaceKHR surface);
    void cleanup(VkDevice device);

    VkCommandPool getCommandPool() const { return commandPool; }
    const std::vector<VkCommandBuffer>& getBuffers() const { return commandBuffers; }

private:
    VkCommandPool commandPool;
    std::vector<VkCommandBuffer> commandBuffers;

    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface);
};

class VulkanSyncObjects {
public:
    void create(VkDevice device);
    void cleanup(VkDevice device);

    const std::vector<VkSemaphore>& getImageAvailableSemaphores() const { return imageAvailableSemaphores; }
    const std::vector<VkSemaphore>& getRenderFinishedSemaphores() const { return renderFinishedSemaphores; }
    const std::vector<VkFence>& getInFlightFences() const { return inFlightFences; }

private:
    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    std::vector<VkFence> inFlightFences;
};
