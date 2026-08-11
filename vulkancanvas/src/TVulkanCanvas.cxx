// Author: Sergey Linev, GSI  06/08/2026

#include "TVulkanCanvas.h"
#include "TVulkanPadPainter.h"

#include "TSystem.h"
#include "TStyle.h"
#include "TCanvas.h"
#include "TROOT.h"
#include "TClass.h"
#include "TError.h"
#include "TTimer.h"
#include "TApplication.h"

#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>

#include <iostream>
#include <mutex>
#include <vector>

using namespace ROOT::Experimental;

// --- Shader sources (spir-v embedded or loaded from file) ---
// Vertex shader: pass-through with viewport transform
// Fragment shader: output vertex color with alpha blend

// Helper: VK check macro
#define VK_CHECK(x) do { VkResult err = x; if (err) { std::cerr << "Vulkan error " << err << " at " << __LINE__ << std::endl; } } while(0)

// ===================== VkContext ====================

VkContext::VkContext() = default;

VkContext::~VkContext() { Destroy(); }

Bool_t VkContext::Create(const char *title, int x, int y, int w, int h) {
   width = w; height = h;

   // Initialize SDL for windowing (WSI surface)
   if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) < 0) {
      std::cerr << "VulkanCanvas: SDL_Init failed: " << SDL_GetError() << std::endl;
      return kFALSE;
   }

   // Create SDL window
   Uint32 flags = SDL_WINDOW_VULKAN | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE;
   window = SDL_CreateWindow(title, x, y, w, h, flags);
   if (!window) {
      std::cerr << "VulkanCanvas: SDL_CreateWindow failed: " << SDL_GetError() << std::endl;
      return kFALSE;
   }

   // Create Vulkan instance with validation layers (debug)
   {
      VkApplicationInfo appInfo = {};
      appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
      appInfo.pApplicationName = "ROOT Canvas";
      appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
      appInfo.pEngineName = "TVulkanCanvas";
      appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
      appInfo.apiVersion = VK_API_VERSION_1_0;

      VkInstanceCreateInfo ic = {};
      ic.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
      ic.pApplicationInfo = &appInfo;

      unsigned int extCount = 0;
      if (!SDL_Vulkan_GetInstanceExtensions(window, &extCount, nullptr)) {
         std::cerr << "VulkanCanvas: SDL_Vulkan_GetInstanceExtensions failed" << std::endl;
         return kFALSE;
      }
      std::vector<const char*> exts(extCount);
      if (!SDL_Vulkan_GetInstanceExtensions(window, &extCount, exts.data())) {
         std::cerr << "VulkanCanvas: SDL_Vulkan_GetInstanceExtensions failed" << std::endl;
         return kFALSE;
      }
      ic.enabledExtensionCount = extCount;
      ic.ppEnabledExtensionNames = exts.data();

      VK_CHECK(vkCreateInstance(&ic, nullptr, &instance));
   }

   // Select physical device
   {
      uint32_t devCount = 0;
      vkEnumeratePhysicalDevices(instance, &devCount, nullptr);
      std::vector<VkPhysicalDevice> devs(devCount);
      vkEnumeratePhysicalDevices(instance, &devCount, devs.data());
      physDev = devs[0]; // pick first available

      // Find graphics queue family
      uint32_t qfCount = 0;
      vkGetPhysicalDeviceQueueFamilyProperties(physDev, &qfCount, nullptr);
      std::vector<VkQueueFamilyProperties> qfs(qfCount);
      vkGetPhysicalDeviceQueueFamilyProperties(physDev, &qfCount, qfs.data());
      for (uint32_t i = 0; i < qfCount; i++) {
         if (qfs[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            queueFamilyIndex = i;
            break;
         }
      }
   }

   // Create WSI surface via SDL
   VkSurfaceKHR nativeSurface = VK_NULL_HANDLE;
   if (!SDL_Vulkan_CreateSurface(window, instance, &nativeSurface)) {
      std::cerr << "VulkanCanvas: SDL_Vulkan_CreateSurface failed" << std::endl;
      return kFALSE;
   }
   surface = nativeSurface;

   // Create logical device
   {
      float prio = 1.0f;
      VkDeviceQueueCreateInfo qci = {};
      qci.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
      qci.queueFamilyIndex = queueFamilyIndex;
      qci.queueCount = 1;
      qci.pQueuePriorities = &prio;

      VkDeviceCreateInfo dci = {};
      dci.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
      dci.queueCreateInfoCount = 1;
      dci.pQueueCreateInfos = &qci;
      dci.enabledExtensionCount = 1;
      const char *ext = VK_KHR_SWAPCHAIN_EXTENSION_NAME;
      dci.ppEnabledExtensionNames = &ext;

      VK_CHECK(vkCreateDevice(physDev, &dci, nullptr, &device));
      vkGetDeviceQueue(device, queueFamilyIndex, 0, &graphicsQueue);
   }

   // --- Swap chain creation (deferred to first frame for simplicity) ---
   // Note: Full swapchain creation requires format/present mode selection.
   // For this prototype, we initialize a minimal swapchain in FrameBegin().

   // Command pool + single command buffer
   {
      VkCommandPoolCreateInfo cpci = {};
      cpci.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
      cpci.queueFamilyIndex = queueFamilyIndex;
      cpci.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
      VK_CHECK(vkCreateCommandPool(device, &cpci, nullptr, &commandPool));

      VkCommandBufferAllocateInfo cbai = {};
      cbai.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
      cbai.commandPool = commandPool;
      cbai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
      cbai.commandBufferCount = 1;
      VK_CHECK(vkAllocateCommandBuffers(device, &cbai, &commandBuffer));
   }

   // Fences and semaphores
   {
      VkFenceCreateInfo fci = {};
      fci.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
      fci.flags = VK_FENCE_CREATE_SIGNALED_BIT;
      VK_CHECK(vkCreateFence(device, &fci, nullptr, &fence));

      VkSemaphoreCreateInfo sci = {};
      sci.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
      VK_CHECK(vkCreateSemaphore(device, &sci, nullptr, &imageAvailable));
      VK_CHECK(vkCreateSemaphore(device, &sci, nullptr, &renderFinished));
   }

   return kTRUE;
}

void VkContext::Destroy() {
   if (device) vkDeviceWaitIdle(device);

   if (vertexStagingMem) vkFreeMemory(device, vertexStagingMem, nullptr);
   if (vertexStaging) { vkDestroyBuffer(device, vertexStaging, nullptr); vertexStaging = VK_NULL_HANDLE; }
   if (defaultSampler) { vkDestroySampler(device, defaultSampler, nullptr); defaultSampler = VK_NULL_HANDLE; }
   if (graphicsPipeline) { vkDestroyPipeline(device, graphicsPipeline, nullptr); graphicsPipeline = VK_NULL_HANDLE; }
   if (pipelineLayout) { vkDestroyPipelineLayout(device, pipelineLayout, nullptr); pipelineLayout = VK_NULL_HANDLE; }
   if (renderPass) { vkDestroyRenderPass(device, renderPass, nullptr); renderPass = VK_NULL_HANDLE; }
   for (auto fb : framebuffers) vkDestroyFramebuffer(device, fb, nullptr);
   for (auto iv : swapchainImageViews) vkDestroyImageView(device, iv, nullptr);
   if (swapchain) { vkDestroySwapchainKHR(device, swapchain, nullptr); swapchain = VK_NULL_HANDLE; }
   if (commandPool) { vkDestroyCommandPool(device, commandPool, nullptr); commandPool = VK_NULL_HANDLE; }
   if (fence) { vkDestroyFence(device, fence, nullptr); fence = VK_NULL_HANDLE; }
   if (imageAvailable) { vkDestroySemaphore(device, imageAvailable, nullptr); imageAvailable = VK_NULL_HANDLE; }
   if (renderFinished) { vkDestroySemaphore(device, renderFinished, nullptr); renderFinished = VK_NULL_HANDLE; }
   if (surface) { vkDestroySurfaceKHR(instance, surface, nullptr); surface = VK_NULL_HANDLE; }
   if (device) { vkDestroyDevice(device, nullptr); device = VK_NULL_HANDLE; }
   if (instance) { vkDestroyInstance(instance, nullptr); instance = VK_NULL_HANDLE; }
   if (window) { SDL_DestroyWindow(window); window = nullptr; }
   SDL_Quit();
}

void VkContext::FrameBegin() {
   // Wait for previous frame to complete
   vkWaitForFences(device, 1, &fence, VK_TRUE, UINT64_MAX);
   vkResetFences(device, 1, &fence);

   // Acquire next swapchain image (prototype: skip full swapchain init)
   // In a full implementation: vkAcquireNextImageKHR here
   currentFrame = 0;
}

void VkContext::FrameEnd() {
   // Submit command buffer and present
   // Full implementation would:
   //   1) End command buffer recording
   //   2) Submit with semaphores
   //   3) Present via vkQueuePresentKHR
   vkQueueSubmit(graphicsQueue, 0, nullptr, fence);
}

// ===================== TVulkanCanvas ====================

class TVulkanEventsTimer : public TTimer {
public:
   TVulkanEventsTimer(Long_t milliSec, Bool_t mode) : TTimer(milliSec, mode) {}
   void Timeout() override {
      // Process SDL events
      SDL_Event ev;
      while (SDL_PollEvent(&ev)) {
         if (ev.type == SDL_QUIT) gApplication->Terminate(0);
      }
      // Trigger re-draw
      TCanvas *canv = gPad ? gPad->GetCanvas() : nullptr;
      TVulkanCanvas *imp = canv ? dynamic_cast<TVulkanCanvas*>(canv->GetCanvasImp()) : nullptr;
      if (imp) imp->RunFrame();
   }
};

static TVulkanEventsTimer *sTimer = nullptr;

TVulkanCanvas::TVulkanCanvas(TCanvas *c, const char *name, Int_t x, Int_t y,
                             UInt_t width, UInt_t height)
   : TCanvasImp(c, name, x, y, width, height),
     fWindowWidth(width), fWindowHeight(height), fPosX(x), fPosY(y)
{
}

TVulkanCanvas::~TVulkanCanvas()
{
}

Int_t TVulkanCanvas::InitWindow() { return 0; }
void TVulkanCanvas::Close() { /* TODO: cleanup */ }

void TVulkanCanvas::Show() {
   // Window is already shown via SDL in VkContext::Create
}

UInt_t TVulkanCanvas::GetWindowGeometry(Int_t &x, Int_t &y, UInt_t &w, UInt_t &h) {
   x = fPosX; y = fPosY; w = fWindowWidth; h = fWindowHeight;
   return 0;
}

void TVulkanCanvas::GetCanvasGeometry(Int_t /*wid*/, UInt_t &w, UInt_t &h) {
   w = fWindowWidth; h = fWindowHeight;
}

void TVulkanCanvas::ResizeCanvasWindow(Int_t /*wid*/) { fResized = kTRUE; }
void TVulkanCanvas::UpdateDisplay(Int_t, Bool_t) { ForceUpdate(); }

void TVulkanCanvas::ForceUpdate() { RunFrame(); }

void TVulkanCanvas::SetWindowPosition(Int_t x, Int_t y) {
   fPosX = x; fPosY = y;
   if (fVkCtx && fVkCtx->window) SDL_SetWindowPosition(fVkCtx->window, x, y);
}

void TVulkanCanvas::SetWindowSize(UInt_t w, UInt_t h) {
   fWindowWidth = w; fWindowHeight = h;
   if (fVkCtx) { fVkCtx->width = w; fVkCtx->height = h; }
   if (fVkCtx && fVkCtx->window) SDL_SetWindowSize(fVkCtx->window, w, h);
   fResized = kTRUE;
}

void TVulkanCanvas::SetWindowTitle(const char *newTitle) {
   fWindowTitle = newTitle;
   if (fVkCtx && fVkCtx->window) SDL_SetWindowTitle(fVkCtx->window, newTitle);
}

void TVulkanCanvas::SetCanvasSize(UInt_t w, UInt_t h) { SetWindowSize(w, h); }
void TVulkanCanvas::Iconify() {
   if (fVkCtx && fVkCtx->window) SDL_MinimizeWindow(fVkCtx->window);
}
void TVulkanCanvas::RaiseWindow() {
   if (fVkCtx && fVkCtx->window) SDL_RaiseWindow(fVkCtx->window);
}

Bool_t TVulkanCanvas::PerformUpdate(Bool_t async) {
   ForceUpdate();
   return kFALSE;
}

TVirtualPadPainter *TVulkanCanvas::CreatePadPainter() {
   return new TVulkanPadPainter();
}

void TVulkanCanvas::RunFrame() {
   if (!fVkCtx || !fVkCtx->device) return;

   VkContext *ctx = fVkCtx.get();
   TVulkanPadPainter::GetDrawCommands().Clear(); // reset from previous frame

   // Begin frame
   ctx->FrameBegin();

   // Start command buffer recording
   VkCommandBufferBeginInfo cbbi = {};
   cbbi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
   vkBeginCommandBuffer(ctx->commandBuffer, &cbbi);

   // [TODO] Begin render pass, bind pipeline, draw collected commands
   // For each line:    vkCmdDraw(2 vertices)
   // For each box:     vkCmdDraw(4 vertices as triangle strip)
   // For fill area:    triangulate + vkCmdDrawIndexed
   // For text:         use text atlas / glyph cache with vkCmdDraw
   // End render pass

   vkEndCommandBuffer(ctx->commandBuffer);

   // Submit and present
   ctx->FrameEnd();

   // Clear accumulated draw commands after recording
   // (they are already embedded in the command buffer)
}

// ===================== Factory ====================

TCanvasImp *TVulkanCanvas::NewCanvas(TCanvas *c, const char *name,
                                     Int_t x, Int_t y, UInt_t width, UInt_t height) {
   auto *imp = new TVulkanCanvas(c, name, x, y, width, height);
   imp->fVkCtx = std::make_unique<VkContext>();
   imp->fVkCtx->Create(name, x < 0 ? SDL_WINDOWPOS_CENTERED :
                               (int)x, y < 0 ? SDL_WINDOWPOS_CENTERED : (int)y,
                        (int)width, (int)height);
   imp->fWindowWidth  = width;
   imp->fWindowHeight = height;
   imp->SetWindowTitle(c->GetTitle());

   // Start event timer (~30 FPS target)
   if (!sTimer) {
      sTimer = new TVulkanEventsTimer(10, kTRUE);
      sTimer->TurnOn();
   }

   c->Resize();
   return imp;
}

ClassImp(TVulkanCanvas)
