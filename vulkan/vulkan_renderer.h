#pragma once

#include "vulkan_types.h"
#include <GLFW/glfw3.h>
#include <vector>
#include <string>

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

private:
    VkPipelineLayout pipelineLayout;
    VkPipeline graphicsPipeline;
    VkRenderPass renderPass;

    static std::vector<char> readFile(const std::string& filename);
    VkShaderModule createShaderModule(VkDevice device, const std::vector<char>& code);
    void createRenderPass(VkDevice device, VkFormat swapChainImageFormat);
    void createGraphicsPipeline(VkDevice device, VkExtent2D swapChainExtent);
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
