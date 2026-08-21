// Author: Sergey Linev, GSI  06/08/2026

/*************************************************************************
 * Copyright (C) 1995-2026, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

#ifndef ROOT_TRaylibPadPainter
#define ROOT_TRaylibPadPainter

#include "TPadPainterBase.h"
#include "Rtypes.h"

#include <map>
#include <string>
#include <list>

#include <raylib.h>


namespace ROOT {
namespace Experimental {

class TRaylibCanvas;

/** \class TRaylibPadPainter
    \ingroup raylibcanvas
    \brief Implement TVirtualPadPainter for raylib immediate-mode graphics

    Uses raylib drawing primitives called within BeginDrawing()/EndDrawing() block.
    The painter is active only during the render pass triggered by TRaylibCanvas.
*/
class TRaylibPadPainter : public TPadPainterBase {

friend class TRaylibCanvas;

protected:
   // Font cache - maps ROOT font ID to loaded raylib Font

   // Map ROOT color to raylib Color
   static Color GetRaylibColor(Color_t id);

   // Current global alpha for transparency support
   float GetCurrentAlpha() const;

   void CleanupTextures();

   std::list<Texture2D> fTextures;

public:

   TRaylibPadPainter() = default;
   ~TRaylibPadPainter() override = default;

   Bool_t HasTTFonts() const override { return kTRUE; }
   Bool_t IsNative() const override { return kTRUE; }
   Bool_t IsSupportAlpha() const override { return kTRUE; }

   void SetOpacity(Int_t percent) override;

   // Off-screen management (not needed for immediate mode)
   Int_t CreateDrawable(UInt_t, UInt_t) override { return 1; }
   void ClearDrawable() override {}
   void CopyDrawable(Int_t, Int_t, Int_t) override {}
   void DestroyDrawable(Int_t) override {}
   void SelectDrawable(Int_t) override {}
   void SetDoubleBuffer(Int_t /* device */, Int_t /* mode */) override {}

   // Cursor
   void SetCursor(Int_t, ECursor) override;

   // Image save
   void SaveImage(TVirtualPad *, const char *, Int_t) const override;

   // TASImage support (noop for immediate mode)
   void DrawPixels(const unsigned char *pixelData, UInt_t width, UInt_t height,
                   Int_t dstX, Int_t dstY, Bool_t enableAlphaBlending) override;

   // Drawing primitives
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

   void DrawTTFglyphs(Int_t px, Int_t py, TTFhandle &ttf, [[maybe_unused]] ETextMode mode) override;

private:
   TRaylibPadPainter(const TRaylibPadPainter &) = delete;
   TRaylibPadPainter(TRaylibPadPainter &&) = delete;
   TRaylibPadPainter &operator=(const TRaylibPadPainter &) = delete;
   TRaylibPadPainter &operator=(TRaylibPadPainter &&) = delete;

   ClassDefOverride(TRaylibPadPainter, 0) // Pad painter on raylib canvas
};

} // namespace Experimental
} // namespace ROOT

#endif