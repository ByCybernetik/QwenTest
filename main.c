#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <vulkan/vulkan.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct {
    SDL_Window *window;
    VkInstance instance;
    VkPhysicalDevice physical_device;
    VkDevice device;
    VkQueue queue;
    VkSurfaceKHR surface;
    VkSwapchainKHR swapchain;
    VkFormat swapchain_format;
    VkExtent2D swapchain_extent;
    VkImageView *swapchain_image_views;
    uint32_t swapchain_image_count;
    VkRenderPass render_pass;
    VkFramebuffer *framebuffers;
    VkCommandPool command_pool;
    VkCommandBuffer *command_buffers;
    VkBuffer vertex_buffer;
    VkDeviceMemory vertex_buffer_memory;
    VkPipelineLayout pipeline_layout;
    VkPipeline graphics_pipeline;
} AppState;

static const float vertices[] = { 0.0f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f };

// Vertex shader SPIR-V
static const uint32_t vert_spv[] = {
    0x07230203,0x00010000,0x000d0000,0x00000025,0x00000000,0x00020011,0x00000001,0x0006000b,
    0x00000001,0x4c534c47,0x6474732e,0x3035342e,0x00000000,0x0003000e,0x00000000,0x00000001,
    0x0008000f,0x00000000,0x00000004,0x6e69616d,0x00000000,0x00000011,0x0000001c,0x00000020,
    0x00030003,0x00000002,0x000001c2,0x000a0004,0x475f4c47,0x4c474f4f,0x70635f45,0x74735f70,
    0x5f656c79,0x656e696c,0x7269645f,0x69746365,0x00006576,0x00080004,0x475f4c47,0x4c474f4f,
    0x6e695f45,0x64756c63,0x69645f65,0x65746369,0x00657669,0x00040005,0x00000004,0x6e69616d,
    0x00000000,0x00060005,0x0000000b,0x505f6c67,0x65567265,0x78657472,0x00000000,0x00060006,
    0x0000000b,0x00000000,0x505f6c67,0x7469736f,0x006e6f69,0x00070006,0x0000000b,0x00000001,
    0x505f6c67,0x746e696f,0x657a6953,0x00000000,0x00070006,0x0000000b,0x00000002,0x435f6c67,
    0x4470696c,0x61747369,0x0065636e,0x00070006,0x0000000b,0x00000003,0x435f6c67,0x446c6c75,
    0x61747369,0x0065636e,0x00030005,0x0000000d,0x00000000,0x00050005,0x00000011,0x67617266,
    0x4f5f6465,0x00747570,0x00060005,0x00000016,0x736e6f63,0x745f7461,0x6f6f636c,0x00007264,
    0x00050005,0x0000001c,0x67617266,0x65567265,0x00000078,0x00040047,0x0000000b,0x0000001e,
    0x00000000,0x00040047,0x0000000d,0x0000001e,0x00000000,0x00040047,0x00000016,0x0000001e,
    0x00000000,0x00040047,0x0000001c,0x0000001e,0x00000000,0x00020013,0x00000002,0x00030021,
    0x00000003,0x00000002,0x00030016,0x00000006,0x00000020,0x00040017,0x00000007,0x00000006,
    0x00000002,0x00040015,0x00000008,0x00000020,0x00000000,0x0004002b,0x00000008,0x00000009,
    0x00000000,0x0004001c,0x0000000a,0x00000007,0x00000009,0x00040020,0x0000000c,0x00000006,
    0x0000000a,0x0004003b,0x0000000c,0x0000000d,0x00000006,0x00040015,0x0000000e,0x00000020,
    0x00000001,0x0004002b,0x0000000e,0x0000000f,0x00000001,0x0004001c,0x00000010,0x00000007,
    0x0000000f,0x00040020,0x00000011,0x00000006,0x00000010,0x0004003b,0x00000011,0x00000016,
    0x00000006,0x00040020,0x00000017,0x00000006,0x00000007,0x00040020,0x00000019,0x00000006,
    0x00000006,0x0004002b,0x00000006,0x0000001b,0xbf000000,0x00000005,0x0000001c,0x00000006,
    0x0000001b,0x0004003b,0x0000001c,0x00000020,0x00000006,0x00040020,0x00000021,0x00000006,
    0x00000006,0x00050036,0x00000002,0x00000004,0x00000000,0x00000003,0x000200f8,0x00000005,
    0x0004003d,0x00000007,0x00000012,0x0000000d,0x00050041,0x00000017,0x00000013,0x00000016,
    0x00000012,0x0004003d,0x00000006,0x00000014,0x00000013,0x00050051,0x00000006,0x00000015,
    0x00000014,0x00000000,0x00050051,0x00000006,0x00000018,0x00000014,0x00000001,0x00070050,
    0x00000007,0x0000001a,0x00000015,0x00000018,0x0000001b,0x0000001b,0x00050041,0x00000019,
    0x0000001d,0x0000001c,0x00000012,0x0004003d,0x00000006,0x0000001e,0x0000001d,0x00050051,
    0x00000006,0x0000001f,0x0000001e,0x00000000,0x00050051,0x00000006,0x00000022,0x0000001e,
    0x00000001,0x00070050,0x00000007,0x00000023,0x0000001f,0x00000022,0x0000001b,0x0000001b,
    0x00050091,0x00000007,0x00000024,0x0000001a,0x00000023,0x00050041,0x00000021,0x00000025,
    0x00000020,0x00000012,0x0003003e,0x00000025,0x00000024,0x000100fd,0x00010038
};

// Fragment shader SPIR-V (red)
static const uint32_t frag_spv[] = {
    0x07230203,0x00010000,0x000d0000,0x0000000f,0x00000000,0x00020011,0x00000001,0x0006000b,
    0x00000001,0x4c534c47,0x6474732e,0x3035342e,0x00000000,0x0003000e,0x00000000,0x00000001,
    0x0007000f,0x00000004,0x00000004,0x6e69616d,0x00000000,0x00000009,0x0000000c,0x00030010,
    0x00000004,0x00000007,0x00030003,0x00000002,0x000001c2,0x000a0004,0x475f4c47,0x4c474f4f,
    0x70635f45,0x74735f70,0x5f656c79,0x656e696c,0x7269645f,0x69746365,0x00006576,0x00080004,
    0x475f4c47,0x4c474f4f,0x6e695f45,0x64756c63,0x69645f65,0x65746369,0x00657669,0x00040005,
    0x00000004,0x6e69616d,0x00000000,0x00050005,0x00000009,0x63786574,0x64726f6f,0x00000000,
    0x00040047,0x00000009,0x0000001e,0x00000000,0x00040047,0x0000000c,0x0000001e,0x00000000,
    0x00020013,0x00000002,0x00030021,0x00000003,0x00000002,0x00030016,0x00000006,0x00000020,
    0x00040017,0x00000007,0x00000006,0x00000004,0x00040020,0x00000008,0x00000006,0x00000007,
    0x0004003b,0x00000008,0x00000009,0x00000006,0x00040017,0x0000000a,0x00000006,0x00000001,
    0x0004002b,0x00000006,0x0000000b,0x3f800000,0x0004002c,0x0000000a,0x0000000d,0x0000000b,
    0x0004002c,0x0000000a,0x0000000e,0x00000000,0x00050051,0x00000006,0x0000000f,0x00000009,
    0x00000000,0x00050051,0x00000006,0x00000010,0x00000009,0x00000001,0x00050051,0x00000006,
    0x00000011,0x00000009,0x00000002,0x00070050,0x00000007,0x00000012,0x0000000f,0x00000010,
    0x00000011,0x0000000d,0x00050091,0x00000007,0x00000013,0x00000012,0x0000000e,0x0004003d,
    0x00000007,0x00000014,0x00000009,0x00050041,0x0000000c,0x00000015,0x00000009,0x00000014,
    0x0003003e,0x00000015,0x00000013,0x000100fd,0x00010038
};

static void pick_gpu(AppState *s) {
    uint32_t n = 0; vkEnumeratePhysicalDevices(s->instance, &n, NULL);
    if (!n) { SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "No Vulkan GPU"); exit(1); }
    VkPhysicalDevice *d = malloc(n * sizeof(VkPhysicalDevice));
    vkEnumeratePhysicalDevices(s->instance, &n, d); s->physical_device = d[0]; free(d);
}

static void create_device(AppState *s) {
    uint32_t n = 0; vkGetPhysicalDeviceQueueFamilyProperties(s->physical_device, &n, NULL);
    VkQueueFamilyProperties *qf = malloc(n * sizeof(VkQueueFamilyProperties));
    vkGetPhysicalDeviceQueueFamilyProperties(s->physical_device, &n, qf);
    uint32_t qi = 0; for (uint32_t i = 0; i < n; i++) if (qf[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) { qi = i; break; }
    free(qf);
    float prio = 1.0f;
    VkDeviceQueueCreateInfo dqci = { .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO, .queueFamilyIndex = qi, .queueCount = 1, .pQueuePriorities = &prio };
    VkDeviceCreateInfo dci = { .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO, .queueCreateInfoCount = 1, .pQueueCreateInfos = &dqci };
    if (vkCreateDevice(s->physical_device, &dci, NULL, &s->device) != VK_SUCCESS) { SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to create device"); exit(1); }
    vkGetDeviceQueue(s->device, qi, 0, &s->queue);
}

static void create_surface(AppState *s, SDL_Window *w) {
    if (!SDL_Vulkan_CreateSurface(w, s->instance, NULL, &s->surface)) { SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to create surface"); exit(1); }
}

static void create_swapchain(AppState *s, SDL_Window *w) {
    uint32_t fn; vkGetPhysicalDeviceSurfaceFormatsKHR(s->physical_device, s->surface, &fn, NULL);
    VkSurfaceFormatKHR *fs = malloc(fn * sizeof(VkSurfaceFormatKHR)); vkGetPhysicalDeviceSurfaceFormatsKHR(s->physical_device, s->surface, &fn, fs);
    VkSurfaceFormatCHF sf = fs[0]; for (uint32_t i = 0; i < fn; i++) if (fs[i].format == VK_FORMAT_B8G8R8A8_SRGB) { sf = fs[i]; break; } free(fs);
    uint32_t mn; vkGetPhysicalDeviceSurfacePresentModesKHR(s->physical_device, s->surface, &mn, NULL);
    VkPresentModeKHR *ms = malloc(mn * sizeof(VkPresentModeKHR)); vkGetPhysicalDeviceSurfacePresentModesKHR(s->physical_device, s->surface, &mn, ms);
    VkPresentModeKHR pm = VK_PRESENT_MODE_FIFO_KHR; for (uint32_t i = 0; i < mn; i++) if (ms[i] == VK_PRESENT_MODE_MAILBOX_KHR) { pm = ms[i]; break; } free(ms);
    VkSurfaceCapabilitiesKHR cap; vkGetPhysicalDeviceSurfaceCapabilitiesKHR(s->physical_device, s->surface, &cap);
    VkExtent2D ex = cap.currentExtent; if (ex.width == UINT32_MAX) { int ww, wh; SDL_GetWindowSizeInPixels(w, &ww, &wh); ex.width = ww; ex.height = wh; }
    uint32_t ic = cap.minImageCount + 1; if (cap.maxImageCount > 0 && ic > cap.maxImageCount) ic = cap.maxImageCount;
    VkSwapchainCreateInfoKHR sci = { .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR, .surface = s->surface, .minImageCount = ic,
        .imageFormat = sf.format, .imageColorSpace = sf.colorSpace, .imageExtent = ex, .imageArrayLayers = 1,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, .preTransform = cap.currentTransform,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR, .presentMode = pm, .clipped = VK_TRUE };
    if (vkCreateSwapchainKHR(s->device, &sci, NULL, &s->swapchain) != VK_SUCCESS) { SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to create swapchain"); exit(1); }
    s->swapchain_format = sf.format; s->swapchain_extent = ex;
    vkGetSwapchainImagesKHR(s->device, s->swapchain, &ic, NULL); s->swapchain_image_count = ic;
}

static void create_image_views(AppState *s) {
    s->swapchain_image_views = malloc(s->swapchain_image_count * sizeof(VkImageView));
    VkImage *imgs = malloc(s->swapchain_image_count * sizeof(VkImage)); vkGetSwapchainImagesKHR(s->device, s->swapchain, &s->swapchain_image_count, imgs);
    for (uint32_t i = 0; i < s->swapchain_image_count; i++) {
        VkImageViewCreateInfo ivci = { .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO, .image = imgs[i], .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = s->swapchain_format, .subresourceRange = { .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .levelCount = 1, .layerCount = 1 }};
        if (vkCreateImageView(s->device, &ivci, NULL, &s->swapchain_image_views[i]) != VK_SUCCESS) { SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to create image view"); exit(1); }
    } free(imgs);
}

static void create_render_pass(AppState *s) {
    VkAttachmentDescription att = { .format = s->swapchain_format, .samples = VK_SAMPLE_COUNT_1_BIT, .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE, .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED, .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR };
    VkAttachmentReference ref = { .attachment = 0, .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
    VkSubpassDescription sub = { .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS, .colorAttachmentCount = 1, .pColorAttachments = &ref };
    VkRenderPassCreateInfo rpci = { .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO, .attachmentCount = 1, .pAttachments = &att, .subpassCount = 1, .pSubpasses = &sub };
    if (vkCreateRenderPass(s->device, &rpci, NULL, &s->render_pass) != VK_SUCCESS) { SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to create render pass"); exit(1); }
}

static uint32_t find_mem(AppState *s, uint32_t flt, VkMemoryPropertyFlags pr) {
    VkPhysicalDeviceMemoryProperties mp; vkGetPhysicalDeviceMemoryProperties(s->physical_device, &mp);
    for (uint32_t i = 0; i < mp.memoryTypeCount; i++) if ((flt & (1u << i)) && (mp.memoryTypes[i].propertyFlags & pr) == pr) return i;
    return UINT32_MAX;
}

static void create_buf(AppState *s, VkDeviceSize sz, VkBufferUsageFlags us, VkMemoryPropertyFlags pr, VkBuffer *b, VkDeviceMemory *m) {
    VkBufferCreateInfo bci = { .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO, .size = sz, .usage = us }; vkCreateBuffer(s->device, &bci, NULL, b);
    VkMemoryRequirements mr; vkGetBufferMemoryRequirements(s->device, *b, &mr);
    VkMemoryAllocateInfo mai = { .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO, .allocationSize = mr.size, .memoryTypeIndex = find_mem(s, mr.memoryTypeBits, pr) };
    vkAllocateMemory(s->device, &mai, NULL, m); vkBindBufferMemory(s->device, *b, *m, 0);
}

static void copy_buf(AppState *s, VkBuffer src, VkBuffer dst, VkDeviceSize sz) {
    VkCommandBufferAllocateInfo cai = { .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO, .commandPool = s->command_pool, .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY, .commandBufferCount = 1 };
    VkCommandBuffer cb; vkAllocateCommandBuffers(s->device, &cai, &cb);
    VkCommandBufferBeginInfo cbbi = { .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO, .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT };
    vkBeginCommandBuffer(cb, &cbbi); VkBufferCopy cp = { .size = sz }; vkCmdCopyBuffer(cb, src, dst, 1, &cp); vkEndCommandBuffer(cb);
    VkSubmitInfo si = { .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO, .commandBufferCount = 1, .pCommandBuffers = &cb };
    vkQueueSubmit(s->queue, 1, &si, VK_NULL_HANDLE); vkQueueWaitIdle(s->queue); vkFreeCommandBuffers(s->device, s->command_pool, 1, &cb);
}

static void create_vertex_buffer(AppState *s) {
    VkDeviceSize sz = sizeof(vertices); VkBuffer stg; VkDeviceMemory stgm;
    create_buf(s, sz, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &stg, &stgm);
    void *data; vkMapMemory(s->device, stgm, 0, sz, 0, &data); memcpy(data, vertices, sz); vkUnmapMemory(s->device, stgm);
    create_buf(s, sz, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &s->vertex_buffer, &s->vertex_buffer_memory);
    copy_buf(s, stg, s->vertex_buffer, sz); vkDestroyBuffer(s->device, stg, NULL); vkFreeMemory(s->device, stgm, NULL);
}

static void create_framebuffers(AppState *s) {
    s->framebuffers = malloc(s->swapchain_image_count * sizeof(VkFramebuffer));
    for (uint32_t i = 0; i < s->swapchain_image_count; i++) {
        VkImageView att = s->swapchain_image_views[i];
        VkFramebufferCreateInfo fci = { .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO, .renderPass = s->render_pass, .attachmentCount = 1, .pAttachments = &att,
            .width = s->swapchain_extent.width, .height = s->swapchain_extent.height, .layers = 1 };
        vkCreateFramebuffer(s->device, &fci, NULL, &s->framebuffers[i]);
    }
}

static void create_command_pool(AppState *s) {
    uint32_t n = 0; vkGetPhysicalDeviceQueueFamilyProperties(s->physical_device, &n, NULL);
    VkQueueFamilyProperties *qf = malloc(n * sizeof(VkQueueFamilyProperties)); vkGetPhysicalDeviceQueueFamilyProperties(s->physical_device, &n, qf);
    uint32_t qi = 0; for (uint32_t i = 0; i < n; i++) if (qf[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) { qi = i; break; } free(qf);
    VkCommandPoolCreateInfo cpci = { .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO, .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, .queueFamilyIndex = qi };
    vkCreateCommandPool(s->device, &cpci, NULL, &s->command_pool);
}

static void create_command_buffers(AppState *s) {
    s->command_buffers = malloc(s->swapchain_image_count * sizeof(VkCommandBuffer));
    VkCommandBufferAllocateInfo cai = { .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO, .commandPool = s->command_pool, .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY, .commandBufferCount = s->swapchain_image_count };
    vkAllocateCommandBuffers(s->device, &cai, s->command_buffers);
}

static void record_cb(AppState *s, uint32_t idx) {
    VkCommandBufferBeginInfo cbbi = { .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
    vkBeginCommandBuffer(s->command_buffers[idx], &cbbi);
    VkClearValue cv = { .color = { {0.0f, 0.0f, 0.0f, 1.0f} } };
    VkRenderPassBeginInfo rpbi = { .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO, .renderPass = s->render_pass, .framebuffer = s->framebuffers[idx],
        .renderArea = { .extent = s->swapchain_extent }, .clearValueCount = 1, .pClearValues = &cv };
    vkCmdBeginRenderPass(s->command_buffers[idx], &rpbi, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(s->command_buffers[idx], VK_PIPELINE_BIND_POINT_GRAPHICS, s->graphics_pipeline);
    VkBuffer vbs[] = { s->vertex_buffer }; VkDeviceSize offs[] = {0};
    vkCmdBindVertexBuffers(s->command_buffers[idx], 0, 1, vbs, offs);
    vkCmdDraw(s->command_buffers[idx], 3, 1, 0, 0);
    vkCmdEndRenderPass(s->command_buffers[idx]); vkEndCommandBuffer(s->command_buffers[idx]);
}

static VkShaderModule create_sm(AppState *s, const uint32_t *c, size_t sz) {
    VkShaderModuleCreateInfo smci = { .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO, .codeSize = sz, .pCode = c };
    VkShaderModule sm; vkCreateShaderModule(s->device, &smci, NULL, &sm); return sm;
}

static void create_pipeline(AppState *s) {
    VkShaderModule vs = create_sm(s, vert_spv, sizeof(vert_spv)); VkShaderModule fs = create_sm(s, frag_spv, sizeof(frag_spv));
    VkPipelineShaderStageCreateInfo vss = { .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, .stage = VK_SHADER_STAGE_VERTEX_BIT, .module = vs, .pName = "main" };
    VkPipelineShaderStageCreateInfo fss = { .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, .stage = VK_SHADER_STAGE_FRAGMENT_BIT, .module = fs, .pName = "main" };
    VkPipelineShaderStageCreateInfo stages[] = { vss, fss };
    VkVertexInputBindingDescription vibd = { .binding = 0, .stride = 8, .inputRate = VK_VERTEX_INPUT_RATE_VERTEX };
    VkVertexInputAttributeDescription viad = { .location = 0, .binding = 0, .format = VK_FORMAT_R32G32_SFLOAT, .offset = 0 };
    VkPipelineVertexInputStateCreateInfo visci = { .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO, .vertexBindingDescriptionCount = 1, .pVertexBindingDescriptions = &vibd, .vertexAttributeDescriptionCount = 1, .pVertexAttributeDescriptions = &viad };
    VkPipelineInputAssemblyStateCreateInfo piasci = { .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO, .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST };
    VkViewport vp = { .width = (float)s->swapchain_extent.width, .height = (float)s->swapchain_extent.height, .maxDepth = 1.0f };
    VkRect2D sc = { .extent = s->swapchain_extent };
    VkPipelineViewportStateCreateInfo pvsci = { .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO, .viewportCount = 1, .pViewports = &vp, .scissorCount = 1, .pScissors = &sc };
    VkPipelineRasterizationStateCreateInfo prsci = { .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO, .polygonMode = VK_POLYGON_MODE_FILL, .lineWidth = 1.0f, .cullMode = VK_CULL_MODE_BACK_BIT, .frontFace = VK_FRONT_FACE_CLOCKWISE };
    VkPipelineMultisampleStateCreateInfo pmsci = { .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO, .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT };
    VkPipelineColorBlendAttachmentState pcbas = { .colorWriteMask = 0xF };
    VkPipelineColorBlendStateCreateInfo pcbsci = { .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO, .attachmentCount = 1, .pAttachments = &pcbas };
    VkPipelineLayoutCreateInfo plci = { .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
    vkCreatePipelineLayout(s->device, &plci, NULL, &s->pipeline_layout);
    VkGraphicsPipelineCreateInfo gpci = { .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO, .stageCount = 2, .pStages = stages,
        .pVertexInputState = &visci, .pInputAssemblyState = &piasci, .pViewportState = &pvsci, .pRasterizationState = &prsci,
        .pMultisampleState = &pmsci, .pColorBlendState = &pcbsci, .layout = s->pipeline_layout, .renderPass = s->render_pass };
    vkCreateGraphicsPipelines(s->device, VK_NULL_HANDLE, 1, &gpci, NULL, &s->graphics_pipeline);
    vkDestroyShaderModule(s->device, fs, NULL); vkDestroyShaderModule(s->device, vs, NULL);
}

static void cleanup_sc(AppState *s) {
    for (uint32_t i = 0; i < s->swapchain_image_count; i++) { vkDestroyImageView(s->device, s->swapchain_image_views[i], NULL); vkDestroyFramebuffer(s->device, s->framebuffers[i], NULL); }
    free(s->swapchain_image_views); free(s->framebuffers); vkDestroySwapchainKHR(s->device, s->swapchain, NULL);
}

static void init_vk(AppState *s, SDL_Window *w) {
    uint32_t ec; const char **ex = SDL_Vulkan_GetInstanceExtensions(w, &ec, NULL);
    VkApplicationInfo ai = { .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO, .pApplicationName = "Vulkan Triangle", .apiVersion = VK_API_VERSION_1_0 };
    VkInstanceCreateInfo ici = { .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO, .pApplicationInfo = &ai, .enabledExtensionCount = ec, .ppEnabledExtensionNames = ex };
    vkCreateInstance(&ici, NULL, &s->instance);
    pick_gpu(s); create_surface(s, w); create_device(s); create_swapchain(s, w); create_image_views(s); create_render_pass(s);
    create_command_pool(s); create_vertex_buffer(s); create_framebuffers(s); create_command_buffers(s); create_pipeline(s);
    for (uint32_t i = 0; i < s->swapchain_image_count; i++) record_cb(s, i);
}

static void cleanup_vk(AppState *s) {
    vkDeviceWaitIdle(s->device); vkDestroyBuffer(s->device, s->vertex_buffer, NULL); vkFreeMemory(s->device, s->vertex_buffer_memory, NULL);
    cleanup_sc(s); vkDestroyPipeline(s->device, s->graphics_pipeline, NULL); vkDestroyPipelineLayout(s->device, s->pipeline_layout, NULL);
    vkDestroyCommandPool(s->device, s->command_pool, NULL); free(s->command_buffers); vkDestroyRenderPass(s->device, s->render_pass, NULL);
    vkDestroyDevice(s->device, NULL); vkDestroySurfaceKHR(s->instance, s->surface, NULL); vkDestroyInstance(s->instance, NULL);
}

static SDL_AppResult app_init(void **appstate, SDL_AppState *sd) {
    AppState *s = calloc(1, sizeof(AppState)); *appstate = s;
    if (!SDL_Init(SDL_INIT_VIDEO)) { SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL_Init: %s", SDL_GetError()); return SDL_APP_FAILURE; }
    s->window = SDL_CreateWindow("Vulkan Triangle", 800, 600, SDL_WINDOW_VULKAN);
    if (!s->window) { SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL_CreateWindow: %s", SDL_GetError()); return SDL_APP_FAILURE; }
    init_vk(s, s->window); return SDL_APP_CONTINUE;
}

static SDL_AppResult app_iterate(void *appstate) {
    AppState *s = (AppState*)appstate; SDL_Event ev; while (SDL_PollEvent(&ev)) { if (ev.type == SDL_EVENT_QUIT) return SDL_APP_SUCCESS; }
    uint32_t idx; VkResult r = vkAcquireNextImageKHR(s->device, s->swapchain, UINT64_MAX, VK_NULL_HANDLE, VK_NULL_HANDLE, &idx);
    if (r == VK_SUCCESS || r == VK_SUBOPTIMAL_KHR) {
        VkSubmitInfo si = { .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO, .commandBufferCount = 1, .pCommandBuffers = &s->command_buffers[idx] };
        vkQueueSubmit(s->queue, 1, &si, VK_NULL_HANDLE);
        VkPresentInfoKHR pi = { .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR, .swapchainCount = 1, .pSwapchains = &s->swapchain, .pImageIndices = &idx };
        vkQueuePresentKHR(s->queue, &pi);
    }
    return SDL_APP_CONTINUE;
}

static void app_quit(void *appstate) {
    AppState *s = (AppState*)appstate; cleanup_vk(s); SDL_DestroyWindow(s->window); SDL_Quit(); free(s);
}

SDL_AppInit_func app_init_ptr = app_init;
SDL_AppIterate_func app_iterate_ptr = app_iterate;
SDL_AppQuit_func app_quit_ptr = app_quit;
