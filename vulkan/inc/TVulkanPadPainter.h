// TVulkanPadPainter.h
//
// Equivalent of TRaylibPadPainter in the raylib version. Where that class
// called DrawLineV()/DrawRectangle()/DrawTriangle() directly, this one calls
// the equivalent VulkanRenderer::Add*() methods, which just accumulate
// vertices for the current frame instead of drawing immediately.
//
// NOTE: TVirtualPadPainter's exact virtual method list/signatures vary a bit
// between ROOT versions. Check $ROOTSYS/include/TVirtualPadPainter.h for
// your version and adjust overrides accordingly - the ones below cover the
// primitives a basic 2D canvas actually needs (lines, boxes, fill areas,
// text) plus attribute get/set, matching what TPadPainter/TRaylibPadPainter
// implement.

#pragma once

#include "TPadPainterBase.h"

class VulkanRenderer;

class TVulkanPadPainter : public TPadPainterBase {
public:
   explicit TVulkanPadPainter(VulkanRenderer *renderer);
   ~TVulkanPadPainter() override = default;

   Bool_t HasTTFonts() const override { return kTRUE; }
   Bool_t IsNative() const override { return kTRUE; }
   Bool_t IsSupportAlpha() const override { return kTRUE; }
   void SetOpacity(Int_t percent) override {}
   Int_t CreateDrawable(UInt_t, UInt_t) override { return 1; }
   void ClearDrawable() override {}
   void CopyDrawable(Int_t, Int_t, Int_t) override {}
   void DestroyDrawable(Int_t) override {}
   void SelectDrawable(Int_t) override {}
   void SetDoubleBuffer(Int_t, Int_t) override {}
   void SetCursor(Int_t, ECursor) override {}
   void SaveImage(TVirtualPad *, const char *, Int_t) const override {}
   void DrawPixels(const unsigned char *, UInt_t, UInt_t, Int_t, Int_t, Bool_t) override {}

   // --- Primitives -------------------------------------------------------
   void DrawLine(Double_t x1, Double_t y1, Double_t x2, Double_t y2) override;
   void DrawLineNDC(Double_t u1, Double_t v1, Double_t u2, Double_t v2) override;

   void DrawBox(Double_t x1, Double_t y1, Double_t x2, Double_t y2, TVirtualPadPainter::EBoxMode mode) override;

   void DrawFillArea(Int_t n, const Double_t *x, const Double_t *y) override;
   void DrawFillArea(Int_t n, const Float_t *x, const Float_t *y) override;

   void DrawPolyLine(Int_t n, const Float_t *x, const Float_t *y) override;
   void DrawPolyLine(Int_t n, const Double_t *x, const Double_t *y) override;
   void DrawPolyLineNDC(Int_t n, const Double_t *u, const Double_t *v) override;

   void DrawPolyMarker(Int_t n, const Float_t *x, const Float_t *y) override;
   void DrawPolyMarker(Int_t n, const Double_t *x, const Double_t *y) override;

   void DrawText(Double_t x, Double_t y, const char *text, TVirtualPadPainter::ETextMode mode) override;
   void DrawText(Double_t x, Double_t y, const wchar_t *text, TVirtualPadPainter::ETextMode mode) override {}
   void DrawTextNDC(Double_t u, Double_t v, const char *text, TVirtualPadPainter::ETextMode mode) override;
   void DrawTextNDC(Double_t u, Double_t v, const wchar_t *text, TVirtualPadPainter::ETextMode mode) override {}

   void DrawTextUrl(Double_t x, Double_t y, const char *text, const char *url) override {}


   // --- Pad/canvas geometry helpers --------------------------------------

private:
   void GetColorRGBA(Color_t colorIndex, float &r, float &g, float &b, float &a) const;
   // Converts NDC (0..1, origin bottom-left, ROOT convention) to this pad's
   // pixel coordinates (origin top-left, Vulkan/raylib convention)
   void NDCtoPixel(Double_t u, Double_t v, float &px, float &py) const;

   VulkanRenderer *fRenderer = nullptr; // not owned
};
