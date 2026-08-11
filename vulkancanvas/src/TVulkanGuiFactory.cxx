// Author: Sergey Linev, GSI  06/08/2026

#include "TVulkanGuiFactory.h"
#include "TVulkanCanvas.h"

using namespace ROOT::Experimental;

TVulkanGuiFactory::TVulkanGuiFactory(const char *name, const char *title)
   : TGuiFactory(name, title)
{
}

TCanvasImp *TVulkanGuiFactory::CreateCanvasImp(TCanvas *c, const char *title,
                                                UInt_t width, UInt_t height)
{
   return TVulkanCanvas::NewCanvas(c, title, -1, -1, width, height);
}

TCanvasImp *TVulkanGuiFactory::CreateCanvasImp(TCanvas *c, const char *title,
                                                Int_t x, Int_t y, UInt_t width, UInt_t height)
{
   return TVulkanCanvas::NewCanvas(c, title, x, y, width, height);
}

ClassImp(TVulkanGuiFactory)