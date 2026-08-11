// Author: Sergey Linev, GSI  06/08/2026

#include "TVulkanGuiFactory.h"
#include "TVulkanCanvasImp.h"

TVulkanGuiFactory::TVulkanGuiFactory(const char *name, const char *title)
   : TGuiFactory(name, title)
{
}

TCanvasImp *TVulkanGuiFactory::CreateCanvasImp(TCanvas *c, const char *title,
                                                UInt_t width, UInt_t height)
{
   return new TVulkanCanvasImp(c, title, width, height);
}

TCanvasImp *TVulkanGuiFactory::CreateCanvasImp(TCanvas *c, const char *title,
                                                [[maybe_unused]] Int_t x,
                                                [[maybe_unused]] Int_t y,
                                                UInt_t width, UInt_t height)
{
   return new TVulkanCanvasImp(c, title, width, height);
}

ClassImp(TVulkanGuiFactory)