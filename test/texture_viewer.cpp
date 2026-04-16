#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <cstring>
#include <vector>

#include "../third_party/stb_image.h"

#include "../vulkan/vulkan_renderer.h"

// Window dimensions are defined in vulkan_types.h

class TextureViewerApplication {
public:
    void run(const std::string& texturePath) {
        initWindow();
        initVulkan(texturePath);
        mainLoop();
        cleanup();
    }

private:
    GLFWwindow* window;
    VulkanDevice device;
    VulkanSwapChain swapChain;
    VulkanTexturePipeline pipeline;
    VulkanCommandBuffer commandBuffer;
    VulkanSyncObjects syncObjects;
    
    TextureInfo textureInfo{};
    VkDescriptorPool descriptorPool;
    VkDescriptorSetLayout descriptorSetLayout;
    VkDescriptorSet descriptorSet;

    void initWindow() {
        glfwInit();

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

        window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan Texture Viewer", nullptr, nullptr);
    }

    void initVulkan(const std::string& texturePath) {
        device.createInstance();
        VkSurfaceKHR surface;
        device.createSurface(window, device.getInstance(), &surface);
        device.pickPhysicalDevice(device.getInstance(), surface);
        device.createLogicalDevice(surface);

        // Create swap chain with a temporary render pass first
        VkRenderPass tempRenderPass = VK_NULL_HANDLE;
        
        // We need to create render pass first for swap chain
        VkAttachmentDescription colorAttachment{};
        colorAttachment.format = VK_FORMAT_B8G8R8A8_SRGB;
        colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        VkAttachmentReference colorAttachmentRef{};
        colorAttachmentRef.attachment = 0;
        colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &colorAttachmentRef;

        VkSubpassDependency dependency{};
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass = 0;
        dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.srcAccessMask = 0;
        dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

        VkRenderPassCreateInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = 1;
        renderPassInfo.pAttachments = &colorAttachment;
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpass;
        renderPassInfo.dependencyCount = 1;
        renderPassInfo.pDependencies = &dependency;

        VkRenderPass renderPass;
        if (vkCreateRenderPass(device.getDevice(), &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
            throw std::runtime_error("failed to create render pass!");
        }

        swapChain.create(device.getDevice(), device.getPhysicalDevice(), 
                        surface, window, renderPass);
        
        // Destroy temporary render pass, pipeline will create its own
        vkDestroyRenderPass(device.getDevice(), renderPass, nullptr);

        // Create texture pipeline
        pipeline.create(device.getDevice(), VK_NULL_HANDLE, swapChain.getExtent());

        // Load texture
        commandBuffer.create(device.getDevice(), device.getPhysicalDevice(), 
                            surface);
        
        pipeline.loadTexture(device.getDevice(), device.getPhysicalDevice(), 
                            device.getGraphicsQueue(), commandBuffer.getCommandPool(),
                            texturePath, textureInfo);

        // Create descriptor pool and set
        createDescriptorPool();
        createDescriptorSet();

        // Create synchronization objects
        syncObjects.create(device.getDevice());
    }

    void createDescriptorPool() {
        VkDescriptorPoolSize poolSize{};
        poolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        poolSize.descriptorCount = 1;

        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.poolSizeCount = 1;
        poolInfo.pPoolSizes = &poolSize;
        poolInfo.maxSets = 1;

        if (vkCreateDescriptorPool(device.getDevice(), &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
            throw std::runtime_error("failed to create descriptor pool!");
        }
    }

    void createDescriptorSet() {
        // Get the descriptor set layout from pipeline (we need to store it)
        // For simplicity, recreate it here
        VkDescriptorSetLayoutBinding samplerBinding{};
        samplerBinding.binding = 0;
        samplerBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        samplerBinding.descriptorCount = 1;
        samplerBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        samplerBinding.pImmutableSamplers = nullptr;

        VkDescriptorSetLayoutCreateInfo descriptorLayout{};
        descriptorLayout.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        descriptorLayout.bindingCount = 1;
        descriptorLayout.pBindings = &samplerBinding;

        if (vkCreateDescriptorSetLayout(device.getDevice(), &descriptorLayout, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
            throw std::runtime_error("failed to create descriptor set layout!");
        }

        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = descriptorPool;
        allocInfo.descriptorSetCount = 1;
        allocInfo.pSetLayouts = &descriptorSetLayout;

        if (vkAllocateDescriptorSets(device.getDevice(), &allocInfo, &descriptorSet) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate descriptor set!");
        }

        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = textureInfo.imageView;
        imageInfo.sampler = textureInfo.sampler;

        VkWriteDescriptorSet descriptorWrite{};
        descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite.dstSet = descriptorSet;
        descriptorWrite.dstBinding = 0;
        descriptorWrite.dstArrayElement = 0;
        descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrite.descriptorCount = 1;
        descriptorWrite.pImageInfo = &imageInfo;

        vkUpdateDescriptorSets(device.getDevice(), 1, &descriptorWrite, 0, nullptr);
    }

    void mainLoop() {
        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();
            drawFrame();
        }

        vkDeviceWaitIdle(device.getDevice());
    }

    void drawFrame() {
        vkWaitForFences(device.getDevice(), 1, &syncObjects.getInFlightFences()[0], VK_TRUE, UINT64_MAX);

        uint32_t imageIndex;
        VkResult result = vkAcquireNextImageKHR(device.getDevice(), swapChain.getSwapChain(), 
                                                 UINT64_MAX, syncObjects.getImageAvailableSemaphores()[0], 
                                                 VK_NULL_HANDLE, &imageIndex);

        if (result == VK_ERROR_OUT_OF_DATE_KHR) {
            return;
        } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
            throw std::runtime_error("failed to acquire swap chain image!");
        }

        vkResetFences(device.getDevice(), 1, &syncObjects.getInFlightFences()[0]);

        vkResetCommandBuffer(commandBuffer.getBuffers()[0], 0);

        // Record command buffer
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

        if (vkBeginCommandBuffer(commandBuffer.getBuffers()[0], &beginInfo) != VK_SUCCESS) {
            throw std::runtime_error("failed to begin recording command buffer!");
        }

        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = pipeline.getRenderPass();
        renderPassInfo.framebuffer = swapChain.getFramebuffers()[imageIndex];
        renderPassInfo.renderArea.offset = {0, 0};
        renderPassInfo.renderArea.extent = swapChain.getExtent();

        VkClearValue clearColor = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
        renderPassInfo.clearValueCount = 1;
        renderPassInfo.pClearValues = &clearColor;

        vkCmdBeginRenderPass(commandBuffer.getBuffers()[0], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

        vkCmdBindPipeline(commandBuffer.getBuffers()[0], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.getPipeline());

        // Bind descriptor set
        vkCmdBindDescriptorSets(commandBuffer.getBuffers()[0], VK_PIPELINE_BIND_POINT_GRAPHICS,
                                pipeline.getLayout(), 0, 1, &descriptorSet, 0, nullptr);

        // Draw textured quad (two triangles)
        struct Vertex {
            float x, y, u, v;
        };

        std::vector<Vertex> vertices = {
            {-1.0f, -1.0f, 0.0f, 1.0f},
            { 1.0f, -1.0f, 1.0f, 1.0f},
            { 1.0f,  1.0f, 1.0f, 0.0f},
            {-1.0f, -1.0f, 0.0f, 1.0f},
            { 1.0f,  1.0f, 1.0f, 0.0f},
            {-1.0f,  1.0f, 0.0f, 0.0f}
        };

        // Create vertex buffer
        VkBuffer vertexBuffer;
        VkDeviceMemory vertexBufferMemory;
        
        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = sizeof(vertices[0]) * vertices.size();
        bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        vkCreateBuffer(device.getDevice(), &bufferInfo, nullptr, &vertexBuffer);

        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements(device.getDevice(), vertexBuffer, &memRequirements);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        
        // Find memory type
        VkPhysicalDeviceMemoryProperties memProperties;
        vkGetPhysicalDeviceMemoryProperties(device.getPhysicalDevice(), &memProperties);
        for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
            if ((memRequirements.memoryTypeBits & (1 << i)) && 
                (memProperties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)) {
                allocInfo.memoryTypeIndex = i;
                break;
            }
        }

        vkAllocateMemory(device.getDevice(), &allocInfo, nullptr, &vertexBufferMemory);
        vkBindBufferMemory(device.getDevice(), vertexBuffer, vertexBufferMemory, 0);

        void* data;
        vkMapMemory(device.getDevice(), vertexBufferMemory, 0, bufferInfo.size, 0, &data);
        memcpy(data, vertices.data(), (size_t)bufferInfo.size);
        vkUnmapMemory(device.getDevice(), vertexBufferMemory);

        VkBuffer vertexBuffers[] = {vertexBuffer};
        VkDeviceSize offsets[] = {0};
        vkCmdBindVertexBuffers(commandBuffer.getBuffers()[0], 0, 1, vertexBuffers, offsets);

        vkCmdDraw(commandBuffer.getBuffers()[0], static_cast<uint32_t>(vertices.size()), 1, 0, 0);

        vkCmdEndRenderPass(commandBuffer.getBuffers()[0]);

        if (vkEndCommandBuffer(commandBuffer.getBuffers()[0]) != VK_SUCCESS) {
            throw std::runtime_error("failed to record command buffer!");
        }

        // Submit
        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

        VkSemaphore waitSemaphores[] = {syncObjects.getImageAvailableSemaphores()[0]};
        VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = waitSemaphores;
        submitInfo.pWaitDstStageMask = waitStages;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer.getBuffers()[0];

        VkSemaphore signalSemaphores[] = {syncObjects.getRenderFinishedSemaphores()[0]};
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSemaphores;

        if (vkQueueSubmit(device.getGraphicsQueue(), 1, &submitInfo, syncObjects.getInFlightFences()[0]) != VK_SUCCESS) {
            throw std::runtime_error("failed to submit draw command buffer!");
        }

        // Present
        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = signalSemaphores;

        VkSwapchainKHR swapChains[] = {swapChain.getSwapChain()};
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = swapChains;
        presentInfo.pImageIndices = &imageIndex;

        vkQueuePresentKHR(device.getPresentQueue(), &presentInfo);

        // Cleanup vertex buffer
        vkDestroyBuffer(device.getDevice(), vertexBuffer, nullptr);
        vkFreeMemory(device.getDevice(), vertexBufferMemory, nullptr);
    }

    void cleanup() {
        vkDeviceWaitIdle(device.getDevice());

        pipeline.cleanupTextures(device.getDevice(), textureInfo);
        
        vkDestroyDescriptorSetLayout(device.getDevice(), descriptorSetLayout, nullptr);
        vkDestroyDescriptorPool(device.getDevice(), descriptorPool, nullptr);

        syncObjects.cleanup(device.getDevice());
        commandBuffer.cleanup(device.getDevice());
        pipeline.cleanup(device.getDevice());
        swapChain.cleanup(device.getDevice());
        device.cleanup(device.getInstance(), surface);

        glfwDestroyWindow(window);
        glfwTerminate();
    }

    VkSurfaceKHR surface;
};

int main(int argc, char* argv[]) {
    std::string texturePath = "texture.png";
    
    if (argc > 1) {
        texturePath = argv[1];
    }

    TextureViewerApplication app;
    
    try {
        app.run(texturePath);
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
