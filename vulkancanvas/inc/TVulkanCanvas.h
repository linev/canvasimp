// Author: Sergey Linev, GSI  06/08/2026

/*************************************************************************
 * Copyright (C) 1995-2026, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

#ifndef ROOT_TVulkanCanvas
#define ROOT_TVulkanCanvas

#include "TCanvasImp.h"
#include "TString.h"
#include <string>
#include <memory>
#include <vector>

#include <vulkan/vulkan.h>


class TTimer;

struct VkInstance_T;          typedef VkInstance_T *VkInstance;
struct VkPhysicalDevice_T;    typedef VkPhysicalDevice_T *VkPhysicalDevice;
struct VkDevice_T;            typedef VkDevice_T *VkDevice;
struct VkSurfaceKHR_T;        typedef VkSurfaceKHR_T *VkSurfaceKHR;
struct VkSwapchainKHR_T;      typedef VkSwapchainKHR_T *VkSwapchainKHR;
struct VkCommandPool_T;       typedef VkCommandPool_T *VkCommandPool;
struct VkCommandBuffer_T;     typedef VkCommandBuffer_T *VkCommandBuffer;
struct VkFence_T;             typedef VkFence_T *VkFence;
struct VkSemaphore_T;         typedef VkSemaphore_T *VkSemaphore;
struct VkQueue_T;             typedef VkQueue_T *VkQueue;
struct VkFramebuffer_T;       typedef VkFramebuffer_T *VkFramebuffer;
struct VkPipeline_T;          typedef VkPipeline_T *VkPipeline;
struct VkPipelineLayout_T;    typedef VkPipelineLayout_T *VkPipelineLayout;
struct VkRenderPass_T;        typedef VkRenderPass_T *VkRenderPass;
struct VkDescriptorSetLayout_T; typedef VkDescriptorSetLayout_T *VkDescriptorSetLayout;
struct VkSampler_T;           typedef VkSampler_T *VkSampler;
struct VkImage_T;             typedef VkImage_T *VkImage;
struct VkImageView_T;         typedef VkImageView_T *VkImageView;
struct VkDeviceMemory_T;      typedef VkDeviceMemory_T *VkDeviceMemory;
struct VkBuffer_T;            typedef VkBuffer_T *VkBuffer;
struct SDL_Window;

namespace ROOT {
namespace Experimental {

class TVulkanPadPainter;

// --- Internal Vulkan context managed per-canvas ---
struct VkContext {
   VkInstance       instance = VK_NULL_HANDLE;
   VkPhysicalDevice physDev  = VK_NULL_HANDLE;
   VkDevice         device   = VK_NULL_HANDLE;
   uint32_t         queueFamilyIndex = 0;
   VkQueue          graphicsQueue = VK_NULL_HANDLE;
   VkSurfaceKHR     surface = VK_NULL_HANDLE;
   VkSwapchainKHR   swapchain = VK_NULL_HANDLE;
   uint32_t         imageCount = 0;
   std::vector<VkImage>       swapchainImages;
   std::vector<VkImageView>   swapchainImageViews;
   std::vector<VkFramebuffer> framebuffers;
   uint32_t         currentFrame = 0;
   VkRenderPass         renderPass = VK_NULL_HANDLE;
   VkPipelineLayout     pipelineLayout = VK_NULL_HANDLE;
   VkPipeline           graphicsPipeline = VK_NULL_HANDLE;
   VkCommandPool   commandPool = VK_NULL_HANDLE;
   VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
   VkFence         fence = VK_NULL_HANDLE;
   VkSemaphore     imageAvailable = VK_NULL_HANDLE;
   VkSemaphore     renderFinished = VK_NULL_HANDLE;
   VkDescriptorSetLayout descSetLayout = VK_NULL_HANDLE;
   VkSampler defaultSampler = VK_NULL_HANDLE;
   VkBuffer      vertexStaging = VK_NULL_HANDLE;
   VkDeviceMemory vertexStagingMem = VK_NULL_HANDLE;
   int width  = 1200;
   int height = 800;
   SDL_Window *window = nullptr;
   VkContext();
   ~VkContext();
   Bool_t Create(const char *title, int x, int y, int w, int h);
   void   Destroy();
   void   FrameBegin();
   void   FrameEnd();
   VkImageView   GetSwapchainView() const { return swapchainImageViews[currentFrame]; }
   VkFramebuffer GetFramebuffer() const   { return framebuffers[currentFrame]; }
};


class TVulkanCanvas : public TCanvasImp {

protected:
   Int_t fWindowWidth  = 0;
   Int_t fWindowHeight = 0;
   Int_t fPosX = 0;
   Int_t fPosY = 0;
   Bool_t fResized = kTRUE;
   Bool_t fMenuBar = kFALSE;
   Bool_t fStatusBar = kTRUE;
   std::string fWindowTitle;
   TString fStatusMessage;
   std::unique_ptr<VkContext> fVkCtx;
   Bool_t PerformUpdate(Bool_t async) override;
   TVirtualPadPainter *CreatePadPainter() override;

public:
   TVulkanCanvas(TCanvas *c, const char *name, Int_t x, Int_t y,
                 UInt_t width, UInt_t height);
   ~TVulkanCanvas() override;
   Int_t InitWindow() override;
   void Close() override;
   void Show() override;
   UInt_t GetWindowGeometry(Int_t &x, Int_t &y, UInt_t &w, UInt_t &h) override;
   void GetCanvasGeometry(Int_t wid, UInt_t &w, UInt_t &h) override;
   void ResizeCanvasWindow(Int_t wid) override;
   void UpdateDisplay(Int_t = 0, Bool_t = kFALSE) override;
   void ShowMenuBar(Bool_t on = kTRUE) override { fMenuBar = on; fResized = kTRUE; }
   void ShowStatusBar(Bool_t on = kTRUE) override { fStatusBar = on; fResized = kTRUE; }
   void ShowEditor(Bool_t = kTRUE) override {}
   void ShowToolBar(Bool_t = kTRUE) override {}
   void ShowToolTips(Bool_t = kTRUE) override {}
   void ForceUpdate() override;
   void SetWindowPosition(Int_t x, Int_t y) override;
   void SetWindowSize(UInt_t w, UInt_t h) override;
   void SetWindowTitle(const char *newTitle) override;
   void SetCanvasSize(UInt_t w, UInt_t h) override;
   void Iconify() override;
   void RaiseWindow() override;
   Bool_t HasEditor() const override { return kFALSE; }
   Bool_t HasMenuBar() const override { return fMenuBar; }
   Bool_t HasStatusBar() const override { return fStatusBar; }
   Bool_t HasToolBar() const override { return kFALSE; }
   Bool_t HasToolTips() const override { return kFALSE; }
   void RunFrame();
   static TCanvasImp *NewCanvas(TCanvas *c, const char *name, Int_t x, Int_t y,
                                 UInt_t width, UInt_t height);
//    ClassDefOverride(TVulkanCanvas, 0)
};

} // namespace Experimental
} // namespace ROOT

#endif
