// VulkanRenderer.cxx
//
// This is the part that has no equivalent in the raylib version at all -
// raylib's InitWindow() does everything below internally. Trimmed for
// readability: production code needs more validation-layer setup, format/
// present-mode selection logic, and swapchain-recreation-on-resize handling.

#include "VulkanRenderer.h"
#include <fstream>
#include <stdexcept>
#include <cstring>
#include <cmath>
#include <algorithm>

static std::vector<char> ReadFile(const char *path)
{
   std::ifstream file(path, std::ios::ate | std::ios::binary);
   if (!file.is_open()) throw std::runtime_error(std::string("failed to open shader: ") + path);
   size_t size = (size_t)file.tellg();
   std::vector<char> buffer(size);
   file.seekg(0);
   file.read(buffer.data(), size);
   return buffer;
}

VulkanRenderer::VulkanRenderer(GLFWwindow *window, uint32_t width, uint32_t height)
   : fWidth(width), fHeight(height)
{
   CreateInstance();
   CreateSurface(window);
   PickPhysicalDevice();
   CreateLogicalDevice();
   CreateSwapchain(width, height);
   CreateRenderPass();
   CreateGraphicsPipeline();
   CreateFramebuffers();
   CreateCommandPoolAndBuffers();
   CreateSyncObjects();
   CreateVertexBuffer();
}

VulkanRenderer::~VulkanRenderer()
{
   if (fDevice) vkDeviceWaitIdle(fDevice);

   if (fVertexBufferMemory) vkUnmapMemory(fDevice, fVertexBufferMemory);
   if (fVertexBuffer) vkDestroyBuffer(fDevice, fVertexBuffer, nullptr);
   if (fVertexBufferMemory) vkFreeMemory(fDevice, fVertexBufferMemory, nullptr);

   if (fImageAvailable) vkDestroySemaphore(fDevice, fImageAvailable, nullptr);
   if (fRenderFinished) vkDestroySemaphore(fDevice, fRenderFinished, nullptr);
   if (fInFlightFence) vkDestroyFence(fDevice, fInFlightFence, nullptr);
   if (fCommandPool) vkDestroyCommandPool(fDevice, fCommandPool, nullptr);

   DestroySwapchain();

   if (fPipeline) vkDestroyPipeline(fDevice, fPipeline, nullptr);
   if (fPipelineLayout) vkDestroyPipelineLayout(fDevice, fPipelineLayout, nullptr);
   if (fRenderPass) vkDestroyRenderPass(fDevice, fRenderPass, nullptr);
   if (fDevice) vkDestroyDevice(fDevice, nullptr);
   if (fSurface) vkDestroySurfaceKHR(fInstance, fSurface, nullptr);
   if (fInstance) vkDestroyInstance(fInstance, nullptr);
}

void VulkanRenderer::CreateInstance()
{
   VkApplicationInfo appInfo{ VK_STRUCTURE_TYPE_APPLICATION_INFO };
   appInfo.pApplicationName = "ROOT Vulkan Canvas";
   appInfo.apiVersion = VK_API_VERSION_1_2;

   uint32_t glfwExtCount = 0;
   const char **glfwExts = glfwGetRequiredInstanceExtensions(&glfwExtCount);

   VkInstanceCreateInfo ci{ VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
   ci.pApplicationInfo = &appInfo;
   ci.enabledExtensionCount = glfwExtCount;
   ci.ppEnabledExtensionNames = glfwExts;
   // NOTE: add VK_LAYER_KHRONOS_validation here for debug builds

   if (vkCreateInstance(&ci, nullptr, &fInstance) != VK_SUCCESS)
      throw std::runtime_error("failed to create Vulkan instance");
}

void VulkanRenderer::CreateSurface(GLFWwindow *window)
{
   if (glfwCreateWindowSurface(fInstance, window, nullptr, &fSurface) != VK_SUCCESS)
      throw std::runtime_error("failed to create window surface");
}

void VulkanRenderer::PickPhysicalDevice()
{
   uint32_t count = 0;
   vkEnumeratePhysicalDevices(fInstance, &count, nullptr);
   if (count == 0) throw std::runtime_error("no Vulkan-capable GPU found");
   std::vector<VkPhysicalDevice> devices(count);
   vkEnumeratePhysicalDevices(fInstance, &count, devices.data());
   // Simplification: take the first device that supports the surface at all.
   // Real code should score devices (discrete GPU preferred, check required
   // queue families / extensions / swapchain support) before picking one.
   fPhysicalDevice = devices[0];
}

void VulkanRenderer::CreateLogicalDevice()
{
   uint32_t queueFamilyCount = 0;
   vkGetPhysicalDeviceQueueFamilyProperties(fPhysicalDevice, &queueFamilyCount, nullptr);
   std::vector<VkQueueFamilyProperties> families(queueFamilyCount);
   vkGetPhysicalDeviceQueueFamilyProperties(fPhysicalDevice, &queueFamilyCount, families.data());

   uint32_t queueFamilyIndex = 0;
   for (uint32_t i = 0; i < queueFamilyCount; ++i) {
      VkBool32 presentSupport = false;
      vkGetPhysicalDeviceSurfaceSupportKHR(fPhysicalDevice, i, fSurface, &presentSupport);
      if ((families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) && presentSupport) {
         queueFamilyIndex = i;
         break;
      }
   }

   float priority = 1.0f;
   VkDeviceQueueCreateInfo qci{ VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO };
   qci.queueFamilyIndex = queueFamilyIndex;
   qci.queueCount = 1;
   qci.pQueuePriorities = &priority;

   const char *deviceExts[] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

   VkDeviceCreateInfo dci{ VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO };
   dci.queueCreateInfoCount = 1;
   dci.pQueueCreateInfos = &qci;
   dci.enabledExtensionCount = 1;
   dci.ppEnabledExtensionNames = deviceExts;

   if (vkCreateDevice(fPhysicalDevice, &dci, nullptr, &fDevice) != VK_SUCCESS)
      throw std::runtime_error("failed to create logical device");

   vkGetDeviceQueue(fDevice, queueFamilyIndex, 0, &fGraphicsQueue);
   fPresentQueue = fGraphicsQueue; // same family here for simplicity
}

void VulkanRenderer::CreateSwapchain(uint32_t width, uint32_t height)
{
   VkSurfaceCapabilitiesKHR caps;
   vkGetPhysicalDeviceSurfaceCapabilitiesKHR(fPhysicalDevice, fSurface, &caps);

   fSwapchainFormat = VK_FORMAT_B8G8R8A8_UNORM; // real code: query supported formats first
   fSwapchainExtent = { width, height };

   VkSwapchainCreateInfoKHR sci{ VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR };
   sci.surface = fSurface;
   sci.minImageCount = std::max<uint32_t>(2, caps.minImageCount);
   sci.imageFormat = fSwapchainFormat;
   sci.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
   sci.imageExtent = fSwapchainExtent;
   sci.imageArrayLayers = 1;
   sci.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
   sci.preTransform = caps.currentTransform;
   sci.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
   sci.presentMode = VK_PRESENT_MODE_FIFO_KHR; // vsync; use MAILBOX if available for lower latency
   sci.clipped = VK_TRUE;

   if (vkCreateSwapchainKHR(fDevice, &sci, nullptr, &fSwapchain) != VK_SUCCESS)
      throw std::runtime_error("failed to create swapchain");

   uint32_t imageCount = 0;
   vkGetSwapchainImagesKHR(fDevice, fSwapchain, &imageCount, nullptr);
   fSwapchainImages.resize(imageCount);
   vkGetSwapchainImagesKHR(fDevice, fSwapchain, &imageCount, fSwapchainImages.data());

   fSwapchainImageViews.resize(imageCount);
   for (uint32_t i = 0; i < imageCount; ++i) {
      VkImageViewCreateInfo ivci{ VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
      ivci.image = fSwapchainImages[i];
      ivci.viewType = VK_IMAGE_VIEW_TYPE_2D;
      ivci.format = fSwapchainFormat;
      ivci.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
      vkCreateImageView(fDevice, &ivci, nullptr, &fSwapchainImageViews[i]);
   }
}

void VulkanRenderer::CreateRenderPass()
{
   VkAttachmentDescription colorAttachment{};
   colorAttachment.format = fSwapchainFormat;
   colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
   colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
   colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
   colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
   colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

   VkAttachmentReference colorRef{ 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
   VkSubpassDescription subpass{};
   subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
   subpass.colorAttachmentCount = 1;
   subpass.pColorAttachments = &colorRef;

   VkRenderPassCreateInfo rpci{ VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };
   rpci.attachmentCount = 1;
   rpci.pAttachments = &colorAttachment;
   rpci.subpassCount = 1;
   rpci.pSubpasses = &subpass;

   if (vkCreateRenderPass(fDevice, &rpci, nullptr, &fRenderPass) != VK_SUCCESS)
      throw std::runtime_error("failed to create render pass");
}

void VulkanRenderer::CreateGraphicsPipeline()
{
   // Expects canvas.vert.spv / canvas.frag.spv compiled offline via:
   //   glslc canvas.vert -o canvas.vert.spv
   //   glslc canvas.frag -o canvas.frag.spv
   auto vertCode = ReadFile("shaders/canvas.vert.spv");
   auto fragCode = ReadFile("shaders/canvas.frag.spv");

   auto makeModule = [&](const std::vector<char> &code) {
      VkShaderModuleCreateInfo smci{ VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
      smci.codeSize = code.size();
      smci.pCode = reinterpret_cast<const uint32_t *>(code.data());
      VkShaderModule module;
      vkCreateShaderModule(fDevice, &smci, nullptr, &module);
      return module;
   };
   VkShaderModule vertModule = makeModule(vertCode);
   VkShaderModule fragModule = makeModule(fragCode);

   VkPipelineShaderStageCreateInfo stages[2]{};
   stages[0] = { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
   stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
   stages[0].module = vertModule;
   stages[0].pName = "main";
   stages[1] = { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
   stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
   stages[1].module = fragModule;
   stages[1].pName = "main";

   VkVertexInputBindingDescription binding{ 0, sizeof(Vertex2D), VK_VERTEX_INPUT_RATE_VERTEX };
   VkVertexInputAttributeDescription attrs[2] = {
      { 0, 0, VK_FORMAT_R32G32_SFLOAT,       offsetof(Vertex2D, x) },
      { 1, 0, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(Vertex2D, r) },
   };
   VkPipelineVertexInputStateCreateInfo vertexInput{ VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };
   vertexInput.vertexBindingDescriptionCount = 1;
   vertexInput.pVertexBindingDescriptions = &binding;
   vertexInput.vertexAttributeDescriptionCount = 2;
   vertexInput.pVertexAttributeDescriptions = attrs;

   VkPipelineInputAssemblyStateCreateInfo inputAssembly{ VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
   inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

   VkViewport viewport{ 0, 0, (float)fSwapchainExtent.width, (float)fSwapchainExtent.height, 0.0f, 1.0f };
   VkRect2D scissor{ {0, 0}, fSwapchainExtent };
   VkPipelineViewportStateCreateInfo viewportState{ VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO };
   viewportState.viewportCount = 1; viewportState.pViewports = &viewport;
   viewportState.scissorCount = 1; viewportState.pScissors = &scissor;

   VkPipelineRasterizationStateCreateInfo raster{ VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };
   raster.polygonMode = VK_POLYGON_MODE_FILL;
   raster.cullMode = VK_CULL_MODE_NONE; // 2D canvas: winding order isn't meaningful here
   raster.lineWidth = 1.0f;

   VkPipelineMultisampleStateCreateInfo msaa{ VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };
   msaa.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

   VkPipelineColorBlendAttachmentState blendAttachment{};
   blendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                     VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
   blendAttachment.blendEnable = VK_TRUE; // needed for translucent fill areas
   blendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
   blendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
   blendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
   blendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
   blendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
   blendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

   VkPipelineColorBlendStateCreateInfo blend{ VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };
   blend.attachmentCount = 1;
   blend.pAttachments = &blendAttachment;

   // push constant: {2D scale, 2D offset} to convert pixel coords -> NDC in the vertex shader,
   // this is what changes when the window resizes
   VkPushConstantRange pushRange{ VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(float) * 4 };
   VkPipelineLayoutCreateInfo plci{ VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
   plci.pushConstantRangeCount = 1;
   plci.pPushConstantRanges = &pushRange;
   vkCreatePipelineLayout(fDevice, &plci, nullptr, &fPipelineLayout);

   VkGraphicsPipelineCreateInfo pci{ VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };
   pci.stageCount = 2;
   pci.pStages = stages;
   pci.pVertexInputState = &vertexInput;
   pci.pInputAssemblyState = &inputAssembly;
   pci.pViewportState = &viewportState;
   pci.pRasterizationState = &raster;
   pci.pMultisampleState = &msaa;
   pci.pColorBlendState = &blend;
   pci.layout = fPipelineLayout;
   pci.renderPass = fRenderPass;
   pci.subpass = 0;

   if (vkCreateGraphicsPipelines(fDevice, VK_NULL_HANDLE, 1, &pci, nullptr, &fPipeline) != VK_SUCCESS)
      throw std::runtime_error("failed to create graphics pipeline");

   vkDestroyShaderModule(fDevice, vertModule, nullptr);
   vkDestroyShaderModule(fDevice, fragModule, nullptr);
}

void VulkanRenderer::CreateFramebuffers()
{
   fFramebuffers.resize(fSwapchainImageViews.size());
   for (size_t i = 0; i < fSwapchainImageViews.size(); ++i) {
      VkFramebufferCreateInfo fci{ VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
      fci.renderPass = fRenderPass;
      fci.attachmentCount = 1;
      fci.pAttachments = &fSwapchainImageViews[i];
      fci.width = fSwapchainExtent.width;
      fci.height = fSwapchainExtent.height;
      fci.layers = 1;
      vkCreateFramebuffer(fDevice, &fci, nullptr, &fFramebuffers[i]);
   }
}

void VulkanRenderer::CreateCommandPoolAndBuffers()
{
   VkCommandPoolCreateInfo pci{ VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
   pci.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
   vkCreateCommandPool(fDevice, &pci, nullptr, &fCommandPool);

   fCommandBuffers.resize(1); // single frame-in-flight for simplicity
   VkCommandBufferAllocateInfo cbai{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
   cbai.commandPool = fCommandPool;
   cbai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
   cbai.commandBufferCount = 1;
   vkAllocateCommandBuffers(fDevice, &cbai, fCommandBuffers.data());
}

void VulkanRenderer::CreateSyncObjects()
{
   VkSemaphoreCreateInfo semInfo{ VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
   VkFenceCreateInfo fenceInfo{ VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
   fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
   vkCreateSemaphore(fDevice, &semInfo, nullptr, &fImageAvailable);
   vkCreateSemaphore(fDevice, &semInfo, nullptr, &fRenderFinished);
   vkCreateFence(fDevice, &fenceInfo, nullptr, &fInFlightFence);
}

static uint32_t FindMemoryType(VkPhysicalDevice phys, uint32_t typeBits, VkMemoryPropertyFlags props)
{
   VkPhysicalDeviceMemoryProperties memProps;
   vkGetPhysicalDeviceMemoryProperties(phys, &memProps);
   for (uint32_t i = 0; i < memProps.memoryTypeCount; ++i)
      if ((typeBits & (1 << i)) && (memProps.memoryTypes[i].propertyFlags & props) == props)
         return i;
   throw std::runtime_error("no suitable memory type for vertex buffer");
}

void VulkanRenderer::CreateVertexBuffer()
{
   VkBufferCreateInfo bci{ VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
   bci.size = kVertexBufferCapacity;
   bci.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
   bci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
   vkCreateBuffer(fDevice, &bci, nullptr, &fVertexBuffer);

   VkMemoryRequirements memReq;
   vkGetBufferMemoryRequirements(fDevice, fVertexBuffer, &memReq);

   VkMemoryAllocateInfo mai{ VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
   mai.allocationSize = memReq.size;
   mai.memoryTypeIndex = FindMemoryType(fPhysicalDevice, memReq.memoryTypeBits,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
   vkAllocateMemory(fDevice, &mai, nullptr, &fVertexBufferMemory);
   vkBindBufferMemory(fDevice, fVertexBuffer, fVertexBufferMemory, 0);
   vkMapMemory(fDevice, fVertexBufferMemory, 0, kVertexBufferCapacity, 0, &fVertexBufferMapped);
}

void VulkanRenderer::DestroySwapchain()
{
   for (auto fb : fFramebuffers) vkDestroyFramebuffer(fDevice, fb, nullptr);
   for (auto view : fSwapchainImageViews) vkDestroyImageView(fDevice, view, nullptr);
   if (fSwapchain) vkDestroySwapchainKHR(fDevice, fSwapchain, nullptr);
   fFramebuffers.clear();
   fSwapchainImageViews.clear();
}

// --- Frame lifecycle ---------------------------------------------------

void VulkanRenderer::BeginFrame()
{
   fFrameVertices.clear();
}

void VulkanRenderer::AddLine(float x1, float y1, float x2, float y2,
                              float r, float g, float b, float a, float lineWidth)
{
   // Vulkan's native line topology doesn't reliably give you a controllable
   // width across GPUs, so lines are expanded into a thin quad (2 triangles)
   // - the same trick raylib itself uses internally for thick lines.
   float dx = x2 - x1, dy = y2 - y1;
   float len = std::sqrt(dx * dx + dy * dy);
   if (len < 1e-6f) return;
   float nx = -dy / len * (lineWidth * 0.5f);
   float ny =  dx / len * (lineWidth * 0.5f);

   AddFilledQuad(x1 + nx, y1 + ny, x2 + nx, y2 + ny, x2 - nx, y2 - ny, x1 - nx, y1 - ny, r, g, b, a);
}

void VulkanRenderer::AddFilledTriangle(float x1, float y1, float x2, float y2, float x3, float y3,
                                        float r, float g, float b, float a)
{
   fFrameVertices.push_back({ x1, y1, r, g, b, a });
   fFrameVertices.push_back({ x2, y2, r, g, b, a });
   fFrameVertices.push_back({ x3, y3, r, g, b, a });
}

void VulkanRenderer::AddFilledQuad(float x1, float y1, float x2, float y2,
                                    float x3, float y3, float x4, float y4,
                                    float r, float g, float b, float a)
{
   AddFilledTriangle(x1, y1, x2, y2, x3, y3, r, g, b, a);
   AddFilledTriangle(x1, y1, x3, y3, x4, y4, r, g, b, a);
}

void VulkanRenderer::AddFilledPolygon(const std::vector<float> &xs, const std::vector<float> &ys,
                                       float r, float g, float b, float a)
{
   // Simple fan triangulation - only correct for convex polygons.
   // ROOT fill areas can be concave (TVirtualPadPainter::DrawFillArea), so a
   // real implementation needs proper polygon triangulation (e.g. ear
   // clipping, or the earcut library) here instead.
   if (xs.size() < 3) return;
   for (size_t i = 1; i + 1 < xs.size(); ++i)
      AddFilledTriangle(xs[0], ys[0], xs[i], ys[i], xs[i + 1], ys[i + 1], r, g, b, a);
}

void VulkanRenderer::EndFrame()
{
   vkWaitForFences(fDevice, 1, &fInFlightFence, VK_TRUE, UINT64_MAX);
   vkResetFences(fDevice, 1, &fInFlightFence);

   vkAcquireNextImageKHR(fDevice, fSwapchain, UINT64_MAX, fImageAvailable, VK_NULL_HANDLE, &fCurrentImageIndex);

   size_t bytes = fFrameVertices.size() * sizeof(Vertex2D);
   if (bytes > 0) memcpy(fVertexBufferMapped, fFrameVertices.data(), bytes);

   VkCommandBuffer cmd = fCommandBuffers[0];
   vkResetCommandBuffer(cmd, 0);

   VkCommandBufferBeginInfo begin{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
   vkBeginCommandBuffer(cmd, &begin);

   VkClearValue clearColor{ { { 1.0f, 1.0f, 1.0f, 1.0f } } }; // white background, like TCanvas default
   VkRenderPassBeginInfo rpBegin{ VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
   rpBegin.renderPass = fRenderPass;
   rpBegin.framebuffer = fFramebuffers[fCurrentImageIndex];
   rpBegin.renderArea = { {0, 0}, fSwapchainExtent };
   rpBegin.clearValueCount = 1;
   rpBegin.pClearValues = &clearColor;

   vkCmdBeginRenderPass(cmd, &rpBegin, VK_SUBPASS_CONTENTS_INLINE);
   vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, fPipeline);

   // push constant: pixel-space -> NDC, i.e. ndc = pos * scale + offset
   float pushData[4] = {
      2.0f / fSwapchainExtent.width, 2.0f / fSwapchainExtent.height,
      -1.0f, -1.0f
   };
   vkCmdPushConstants(cmd, fPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(pushData), pushData);

   VkDeviceSize offset = 0;
   vkCmdBindVertexBuffers(cmd, 0, 1, &fVertexBuffer, &offset);
   if (!fFrameVertices.empty())
      vkCmdDraw(cmd, (uint32_t)fFrameVertices.size(), 1, 0, 0);

   vkCmdEndRenderPass(cmd);
   vkEndCommandBuffer(cmd);

   VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
   VkSubmitInfo submit{ VK_STRUCTURE_TYPE_SUBMIT_INFO };
   submit.waitSemaphoreCount = 1;
   submit.pWaitSemaphores = &fImageAvailable;
   submit.pWaitDstStageMask = &waitStage;
   submit.commandBufferCount = 1;
   submit.pCommandBuffers = &cmd;
   submit.signalSemaphoreCount = 1;
   submit.pSignalSemaphores = &fRenderFinished;
   vkQueueSubmit(fGraphicsQueue, 1, &submit, fInFlightFence);

   VkPresentInfoKHR present{ VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
   present.waitSemaphoreCount = 1;
   present.pWaitSemaphores = &fRenderFinished;
   present.swapchainCount = 1;
   present.pSwapchains = &fSwapchain;
   present.pImageIndices = &fCurrentImageIndex;
   vkQueuePresentKHR(fPresentQueue, &present);
}

void VulkanRenderer::OnResize(uint32_t width, uint32_t height)
{
   vkDeviceWaitIdle(fDevice);
   DestroySwapchain();
   fWidth = width; fHeight = height;
   CreateSwapchain(width, height);
   CreateFramebuffers();
}
