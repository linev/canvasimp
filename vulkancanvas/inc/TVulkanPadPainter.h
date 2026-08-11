// Author: Sergey Linev, GSI  06/08/2026

/*************************************************************************
 * Copyright (C) 1995-2026, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

#ifndef ROOT_TVulkanPadPainter
#define ROOT_TVulkanPadPainter

#include "TPadPainterBase.h"
#include "Rtypes.h"
#include <vector>
#include <string>
#include <map>

struct VkBuffer_T;            typedef VkBuffer_T *VkBuffer;
struct VkDeviceMemory_T;      typedef VkDeviceMemory_T *VkDeviceMemory;
struct VkCommandBuffer_T;     typedef VkCommandBuffer_T *VkCommandBuffer;

namespace ROOT { namespace Experimental {

class TVulkanCanvas;

struct VkVertex { float x, y, r, g, b, a; };  // position + color
struct VkCmdLine   { VkVertex v[2]; };
struct VkCmdBox    { VkVertex v[4]; };
struct VkCmdFillArea { int n; std::vector<VkVertex> verts; };
struct VkCmdPolyLine { int n; std::vector<VkVertex> verts; };
struct VkCmdPolyMarker { int n; std::vector<VkVertex> verts; };
struct VkCmdText { float x, y; std::string text; Color_t color; };

// ─── Draw command queue (collected during pad paint) ──────────────────
struct VkDrawCommands {
   std::vector<VkCmdLine>     lines;
   std::vector<VkCmdBox>      boxes;
   std::vector<VkCmdFillArea> fillAreas;
   std::vector<VkCmdPolyLine> polyLines;
   std::vector<VkCmdPolyMarker> polyMarkers;
   std::vector<VkCmdText>     texts;
   void Clear() { lines.clear(); boxes.clear(); fillAreas.clear();
                  polyLines.clear(); polyMarkers.clear(); texts.clear(); }
};

class TVulkanPadPainter : public TPadPainterBase {
friend class TVulkanCanvas;
protected:
   static VkDrawCommands &GetDrawCommands();
   void PaintTextHelper(float px, float py, const char *text);
public:
   TVulkanPadPainter() = default;
   ~TVulkanPadPainter() override = default;
   Bool_t HasTTFonts() const override { return kTRUE; }
   Bool_t IsNative() const override { return kTRUE; }
   Bool_t IsSupportAlpha() const override { return kTRUE; }
   void SetOpacity(Int_t percent) override;
   Int_t CreateDrawable(UInt_t, UInt_t) override { return 1; }
   void ClearDrawable() override {}
   void CopyDrawable(Int_t, Int_t, Int_t) override {}
   void DestroyDrawable(Int_t) override {}
   void SelectDrawable(Int_t) override {}
   void SetDoubleBuffer(Int_t, Int_t) override {}
   void SetCursor(Int_t, ECursor) override;
   void SaveImage(TVirtualPad *, const char *, Int_t) const override;
   void DrawPixels(const unsigned char *, UInt_t, UInt_t, Int_t, Int_t, Bool_t) override;
   void DrawLine(Double_t x1, Double_t y1, Double_t x2, Double_t y2) override;
   void DrawLineNDC(Double_t u1, Double_t v1, Double_t u2, Double_t v2) override;
   void DrawBox(Double_t x1, Double_t y1, Double_t x2, Double_t y2, EBoxMode mode) override;
   void DrawFillArea(Int_t n, const Double_t *x, const Double_t *y) override;
   void DrawFillArea(Int_t n, const Float_t *x, const Float_t *y) override;
   void DrawPolyLine(Int_t n, const Double_t *x, const Double_t *y) override;
   void DrawPolyLine(Int_t n, const Float_t *x, const Float_t *y) override;
   void DrawPolyLineNDC(Int_t n, const Double_t *u, const Double_t *v) override;
   void DrawPolyMarker(Int_t n, const Double_t *x, const Double_t *y) override;
   void DrawPolyMarker(Int_t n, const Float_t *x, const Float_t *y) override;
   void DrawText(Double_t x, Double_t y, const char *text, ETextMode mode) override;
   void DrawText(Double_t x, Double_t y, const wchar_t *text, ETextMode mode) override;
   void DrawTextNDC(Double_t u, Double_t v, const char *text, ETextMode mode) override;
   void DrawTextNDC(Double_t u, Double_t v, const wchar_t *text, ETextMode mode) override;
   void DrawTextUrl(Double_t x, Double_t y, const char *text, const char *url) override;
   void GetTextExtent(Font_t font, Double_t size, UInt_t &w, UInt_t &h,
                      const char *mess) override;
   void GetTextExtent(Font_t font, Double_t size, UInt_t &w, UInt_t &h,
                      const wchar_t *mess) override;
   void GetTextAscentDescent(Font_t font, Double_t size, UInt_t &a, UInt_t &d,
                             const char *mess) override;
   void GetTextAscentDescent(Font_t font, Double_t size, UInt_t &a, UInt_t &d,
                             const wchar_t *mess) override;
   UInt_t GetTextAdvance(Font_t font, Double_t size, const char *text, Bool_t kern) override;
// private:
//   ClassDefOverride(TVulkanPadPainter, 0)
};

} // namespace Experimental
} // namespace ROOT
#endif
