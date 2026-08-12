// VulkanRenderer.h
//
// Minimal 2D batched renderer on top of Vulkan. This exists purely so the
// ROOT-facing classes (TVulkanCanvasImp / TVulkanPadPainter) can call simple
// functions like AddLine()/AddFilledQuad(), the same way the raylib version
// calls DrawLineV()/DrawRectangle() directly. Everything Vulkan-specific
// (instance, device, swapchain, pipeline, command buffers, sync objects)
// lives here and nowhere else.
//
// This is a SKELETON: enough structure to show where each piece goes, not a
// production-hardened renderer. Missing on purpose (see bottom of file):
// swapchain recreation robustness, MSAA, proper font atlas text rendering,
// multi-canvas support.

#pragma once

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <vector>
#include <cstdint>

struct Vertex2D {
   float x, y;          // pixel-space position (converted to NDC in the vertex shader)
   float r, g, b, a;     // color
};

class VulkanRenderer {
public:
   VulkanRenderer(GLFWwindow *window, uint32_t width, uint32_t height);
   ~VulkanRenderer();

   // Call once per frame, before issuing any Add*() calls
   void BeginFrame();

   // Immediate-mode style primitives - these just push vertices into the
   // current frame's CPU-side buffer, nothing is drawn yet
   void AddLine(float x1, float y1, float x2, float y2, float r, float g, float b, float a, float lineWidth = 1.0f);
   void AddFilledTriangle(float x1, float y1, float x2, float y2, float x3, float y3,
                           float r, float g, float b, float a);
   void AddFilledQuad(float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4,
                       float r, float g, float b, float a);
   void AddFilledPolygon(const std::vector<float> &xs, const std::vector<float> &ys,
                          float r, float g, float b, float a); // simple fan triangulation, convex only

   // Flushes the accumulated vertex buffer through a command buffer and presents
   void EndFrame();

   void OnResize(uint32_t width, uint32_t height);

private:
   void CreateInstance();
   void CreateSurface(GLFWwindow *window);
   void PickPhysicalDevice();
   void CreateLogicalDevice();
   void CreateSwapchain(uint32_t width, uint32_t height);
   void CreateRenderPass();
   void CreateGraphicsPipeline();     // loads precompiled canvas.vert.spv / canvas.frag.spv
   void CreateFramebuffers();
   void CreateCommandPoolAndBuffers();
   void CreateSyncObjects();
   void CreateVertexBuffer();         // host-visible, re-uploaded every frame (simplest possible approach)
   void DestroySwapchain();           // for resize / shutdown

   VkInstance               fInstance = VK_NULL_HANDLE;
   VkSurfaceKHR              fSurface = VK_NULL_HANDLE;
   VkPhysicalDevice          fPhysicalDevice = VK_NULL_HANDLE;
   VkDevice                  fDevice = VK_NULL_HANDLE;
   VkQueue                   fGraphicsQueue = VK_NULL_HANDLE;
   VkQueue                   fPresentQueue = VK_NULL_HANDLE;

   VkSwapchainKHR             fSwapchain = VK_NULL_HANDLE;
   VkFormat                   fSwapchainFormat{};
   VkExtent2D                 fSwapchainExtent{};
   std::vector<VkImage>       fSwapchainImages;
   std::vector<VkImageView>   fSwapchainImageViews;
   std::vector<VkFramebuffer> fFramebuffers;

   VkRenderPass               fRenderPass = VK_NULL_HANDLE;
   VkPipelineLayout           fPipelineLayout = VK_NULL_HANDLE;
   VkPipeline                 fPipeline = VK_NULL_HANDLE;

   VkCommandPool              fCommandPool = VK_NULL_HANDLE;
   std::vector<VkCommandBuffer> fCommandBuffers;

   VkSemaphore fImageAvailable = VK_NULL_HANDLE;
   VkSemaphore fRenderFinished = VK_NULL_HANDLE;
   VkFence     fInFlightFence = VK_NULL_HANDLE;

   VkBuffer       fVertexBuffer = VK_NULL_HANDLE;
   VkDeviceMemory fVertexBufferMemory = VK_NULL_HANDLE;
   void          *fVertexBufferMapped = nullptr;
   static constexpr VkDeviceSize kVertexBufferCapacity = 1024 * 1024; // bytes, generous for a 2D canvas

   std::vector<Vertex2D> fFrameVertices; // CPU-side accumulation, cleared each BeginFrame()
   uint32_t fCurrentImageIndex = 0;

   uint32_t fWidth = 0, fHeight = 0;
};

// --- What's intentionally left out of this skeleton ------------------------
// - Validation layers / debug messenger setup (add for development builds)
// - Swapchain recreation on VK_ERROR_OUT_OF_DATE_KHR / window resize race
// - Anti-aliasing for line edges (raylib gets this via GPU line smoothing;
//   here you'd either widen lines into quads + alpha falloff, or MSAA)
// - Text rendering: needs a font atlas (e.g. stb_truetype) baked to a texture,
//   a second pipeline with a sampler, and UV coords added to Vertex2D
// - Double buffering the vertex buffer to avoid stalling the GPU while the
//   CPU writes the next frame's vertices
