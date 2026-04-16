#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

#include <iostream>
#include <stdexcept>
#include <vector>

#include "vulkan/vulkan_renderer.h"

class VulkanTriangleApplication {
public:
    void run() {
        initWindow();
        initVulkan();
        mainLoop();
        cleanup();
    }

private:
    GLFWwindow* window;
    VulkanDevice vulkanDevice;
    VulkanSwapChain vulkanSwapChain;
    VulkanPipeline vulkanPipeline;
    VulkanCommandBuffer vulkanCommandBuffer;
    VulkanSyncObjects vulkanSyncObjects;
    uint32_t currentFrame = 0;
    bool framebufferResized = false;

    static void framebufferResizeCallback(GLFWwindow* window, int width, int height) {
        auto app = reinterpret_cast<VulkanTriangleApplication*>(glfwGetWindowUserPointer(window));
        app->framebufferResized = true;
    }

    void initWindow() {
        glfwInit();
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

        window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan Triangle", nullptr, nullptr);
        glfwSetWindowUserPointer(window, this);
        glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
    }

    void initVulkan() {
        vulkanDevice.createInstance();
        vulkanDevice.createSurface(window, vulkanDevice.getInstance(), &surfaceRef);
        vulkanDevice.pickPhysicalDevice(vulkanDevice.getInstance(), surfaceRef);
        vulkanDevice.createLogicalDevice(surfaceRef);
        
        // Create pipeline first to get render pass
        vulkanPipeline.create(vulkanDevice.getDevice(), VK_NULL_HANDLE, VkExtent2D{WIDTH, HEIGHT});
        
        vulkanSwapChain.create(vulkanDevice.getDevice(), vulkanDevice.getPhysicalDevice(), 
                               surfaceRef, window, vulkanPipeline.getRenderPass());
        
        // Recreate pipeline with correct swap chain format
        vulkanPipeline.cleanup(vulkanDevice.getDevice());
        vulkanPipeline.create(vulkanDevice.getDevice(), vulkanPipeline.getRenderPass(), 
                              vulkanSwapChain.getExtent());
        
        vulkanCommandBuffer.create(vulkanDevice.getDevice(), vulkanDevice.getPhysicalDevice(), surfaceRef);
        vulkanSyncObjects.create(vulkanDevice.getDevice());
    }

    void mainLoop() {
        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();
            drawFrame();
        }
        vkDeviceWaitIdle(vulkanDevice.getDevice());
    }

    void cleanup() {
        vulkanSwapChain.cleanup(vulkanDevice.getDevice());
        vulkanPipeline.cleanup(vulkanDevice.getDevice());
        vulkanCommandBuffer.cleanup(vulkanDevice.getDevice());
        vulkanSyncObjects.cleanup(vulkanDevice.getDevice());
        vulkanDevice.cleanup(vulkanDevice.getInstance(), surfaceRef);
        glfwDestroyWindow(window);
        glfwTerminate();
    }

    void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex) {
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

        if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
            throw std::runtime_error("failed to begin recording command buffer!");
        }

        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = vulkanPipeline.getRenderPass();
        renderPassInfo.framebuffer = vulkanSwapChain.getFramebuffers()[imageIndex];
        renderPassInfo.renderArea.offset = {0, 0};
        renderPassInfo.renderArea.extent = vulkanSwapChain.getExtent();

        VkClearValue clearColor = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
        renderPassInfo.clearValueCount = 1;
        renderPassInfo.pClearValues = &clearColor;

        vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vulkanPipeline.getPipeline());

        vkCmdDraw(commandBuffer, 3, 1, 0, 0);

        vkCmdEndRenderPass(commandBuffer);

        if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
            throw std::runtime_error("failed to record command buffer!");
        }
    }

    void drawFrame() {
        vkWaitForFences(vulkanDevice.getDevice(), 1, &vulkanSyncObjects.getInFlightFences()[currentFrame], VK_TRUE, UINT64_MAX);

        uint32_t imageIndex;
        VkResult result = vkAcquireNextImageKHR(vulkanDevice.getDevice(), vulkanSwapChain.getSwapChain(), UINT64_MAX,
                                                 vulkanSyncObjects.getImageAvailableSemaphores()[currentFrame],
                                                 VK_NULL_HANDLE, &imageIndex);

        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized) {
            framebufferResized = false;
            vulkanSwapChain.recreate(vulkanDevice.getDevice(), vulkanDevice.getPhysicalDevice(), 
                                     surfaceRef, window, vulkanPipeline.getRenderPass());
            return;
        } else if (result != VK_SUCCESS) {
            throw std::runtime_error("failed to acquire swap chain image!");
        }

        vkResetFences(vulkanDevice.getDevice(), 1, &vulkanSyncObjects.getInFlightFences()[currentFrame]);

        vkResetCommandBuffer(vulkanCommandBuffer.getBuffers()[currentFrame], 0);
        recordCommandBuffer(vulkanCommandBuffer.getBuffers()[currentFrame], imageIndex);

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

        VkSemaphore waitSemaphores[] = {vulkanSyncObjects.getImageAvailableSemaphores()[currentFrame]};
        VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = waitSemaphores;
        submitInfo.pWaitDstStageMask = waitStages;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &vulkanCommandBuffer.getBuffers()[currentFrame];

        VkSemaphore signalSemaphores[] = {vulkanSyncObjects.getRenderFinishedSemaphores()[currentFrame]};
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSemaphores;

        if (vkQueueSubmit(vulkanDevice.getGraphicsQueue(), 1, &submitInfo, vulkanSyncObjects.getInFlightFences()[currentFrame]) != VK_SUCCESS) {
            throw std::runtime_error("failed to submit draw command buffer!");
        }

        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = signalSemaphores;

        VkSwapchainKHR swapChains[] = {vulkanSwapChain.getSwapChain()};
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = swapChains;
        presentInfo.pImageIndices = &imageIndex;

        result = vkQueuePresentKHR(vulkanDevice.getPresentQueue(), &presentInfo);

        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized) {
            framebufferResized = false;
            vulkanSwapChain.recreate(vulkanDevice.getDevice(), vulkanDevice.getPhysicalDevice(), 
                                     surfaceRef, window, vulkanPipeline.getRenderPass());
        } else if (result != VK_SUCCESS) {
            throw std::runtime_error("failed to present swap chain image!");
        }

        currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
    }

    VkSurfaceKHR surfaceRef;
};

int main() {
    VulkanTriangleApplication app;

    try {
        app.run();
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
