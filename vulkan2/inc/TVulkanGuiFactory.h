// Author: Sergey Linev, GSI  06/08/2026
//
/*************************************************************************
 * Copyright (C) 1995-2026, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

#ifndef ROOT_TVulkanGuiFactory
#define ROOT_TVulkanGuiFactory

#include "TGuiFactory.h"

/** \class TVulkanGuiFactory
    \ingroup vulkancanvas

    Factory for ROOT GUI components using Vulkan as the rendering backend.
    Provides specialization for TCanvasImp class.

    Vulkan requires explicit device management: instance → physical device
    → logical device → swap chain. This factory delegates creation to
    TVulkanCanvas which manages those Vulkan objects per-window.
*/
class TVulkanGuiFactory : public TGuiFactory {

public:
   TVulkanGuiFactory(const char *name = "vulkan", const char *title = "ROOT Vulkan Gui");
   ~TVulkanGuiFactory() override = default;

   Bool_t UseVirtualX() const override { return kFALSE; }

   TCanvasImp *CreateCanvasImp(TCanvas *c, const char *title,
                                UInt_t width, UInt_t height) override;
   TCanvasImp *CreateCanvasImp(TCanvas *c, const char *title,
                                Int_t x, Int_t y, UInt_t width, UInt_t height) override;

   ClassDefOverride(TVulkanGuiFactory, 0) // Vulkan gui factory
};

#endif
