// TVulkanPadPainter.cxx

#include "TVulkanPadPainter.h"
#include "VulkanRenderer.h"

#include "TColor.h"
#include "TROOT.h"
#include "TVirtualPad.h"
#include "TAttText.h" // ETextMode etc, exact header depends on ROOT version

TVulkanPadPainter::TVulkanPadPainter(VulkanRenderer *renderer)
   : fRenderer(renderer)
{
}

void TVulkanPadPainter::GetColorRGBA(Color_t colorIndex, float &r, float &g, float &b, float &a) const
{
   TColor *c = gROOT->GetColor(colorIndex);
   if (c) {
      r = (float)c->GetRed();
      g = (float)c->GetGreen();
      b = (float)c->GetBlue();
      a = (float)c->GetAlpha();
   } else {
      r = g = b = 0.0f; // fall back to black if the index isn't a known color
      a = 1;
   }
}

void TVulkanPadPainter::NDCtoPixel(Double_t u, Double_t v, float &px, float &py) const
{
   // Same idea as the raylib version's NDC->pixel conversion: needs the
   // current pad's pixel width/height, available via gPad while painting.
   Int_t w = gPad ? gPad->GetWw() : 1;
   Int_t h = gPad ? gPad->GetWh() : 1;
   px = (float)(u * w);
   py = (float)((1.0 - v) * h); // ROOT's v=0 is bottom, our pixel y=0 is top
}

void TVulkanPadPainter::DrawLine(Double_t x1, Double_t y1, Double_t x2, Double_t y2)
{
   float r, g, b, a;
   auto &att = GetAttLine();
   GetColorRGBA(att.GetLineColor(), r, g, b, a);
   // NOTE: this assumes x1/y1/x2/y2 already arrive in pixel space, matching
   // how TRaylibPadPainter's DrawLine consumes them. If your pad hasn't
   // already converted user coordinates to pixels at this point, apply
   // gPad's XtoPixel()/YtoPixel() here first.
   fRenderer->AddLine((float)x1, (float)y1, (float)x2, (float)y2, r, g, b, a, (float)att.GetLineWidth());
}

void TVulkanPadPainter::DrawLineNDC(Double_t u1, Double_t v1, Double_t u2, Double_t v2)
{
   float x1, y1, x2, y2;
   auto &att = GetAttLine();
   NDCtoPixel(u1, v1, x1, y1);
   NDCtoPixel(u2, v2, x2, y2);
   float r, g, b, a;
   GetColorRGBA(att.GetLineColor(), r, g, b, a);
   fRenderer->AddLine(x1, y1, x2, y2, r, g, b, a, (float)att.GetLineWidth());
}

void TVulkanPadPainter::DrawBox(Double_t x1, Double_t y1, Double_t x2, Double_t y2, TVirtualPadPainter::EBoxMode mode)
{
   float r, g, b, a;
   if (mode == TVirtualPadPainter::kHollow) {
      auto &att = GetAttLine();
      GetColorRGBA(att.GetLineColor(), r, g, b, a);
      fRenderer->AddLine((float)x1, (float)y1, (float)x2, (float)y1, r, g, b, a, (float)att.GetLineWidth());
      fRenderer->AddLine((float)x2, (float)y1, (float)x2, (float)y2, r, g, b, a, (float)att.GetLineWidth());
      fRenderer->AddLine((float)x2, (float)y2, (float)x1, (float)y2, r, g, b, a, (float)att.GetLineWidth());
      fRenderer->AddLine((float)x1, (float)y2, (float)x1, (float)y1, r, g, b, a, (float)att.GetLineWidth());
   } else {
      auto &att = GetAttFill();
      GetColorRGBA(att.GetFillColor(), r, g, b, a);
      fRenderer->AddFilledQuad((float)x1, (float)y1, (float)x2, (float)y1,
                                (float)x2, (float)y2, (float)x1, (float)y2, r, g, b, a);
   }
}

void TVulkanPadPainter::DrawFillArea(Int_t n, const Double_t *x, const Double_t *y)
{
   std::vector<float> xs(n), ys(n);
   for (Int_t i = 0; i < n; ++i) { xs[i] = (float)x[i]; ys[i] = (float)y[i]; }
   float r, g, b, a;
   auto &att = GetAttFill();
   GetColorRGBA(att.GetFillColor(), r, g, b, a);
   fRenderer->AddFilledPolygon(xs, ys, r, g, b, a); // convex-only, see VulkanRenderer.h note
}

void TVulkanPadPainter::DrawFillArea(Int_t n, const Float_t *x, const Float_t *y)
{
   std::vector<float> xs(x, x + n), ys(y, y + n);
   float r, g, b, a;
   auto &att = GetAttFill();
   GetColorRGBA(att.GetFillColor(), r, g, b, a);
   fRenderer->AddFilledPolygon(xs, ys, r, g, b, a);
}

void TVulkanPadPainter::DrawPolyLine(Int_t n, const Double_t *x, const Double_t *y)
{
   float r, g, b, a;
   auto &att = GetAttLine();
   GetColorRGBA(att.GetLineColor(), r, g, b, a);
   for (Int_t i = 0; i + 1 < n; ++i)
      fRenderer->AddLine((float)x[i], (float)y[i], (float)x[i + 1], (float)y[i + 1], r, g, b, a, (float)att.GetLineWidth());
}

void TVulkanPadPainter::DrawPolyLineNDC(Int_t n, const Double_t *u, const Double_t *v)
{
   float r, g, b, a;
   auto &att = GetAttLine();
   GetColorRGBA(att.GetLineColor(), r, g, b, a);
   for (Int_t i = 0; i + 1 < n; ++i) {
      float x1, y1, x2, y2;
      NDCtoPixel(u[i], v[i], x1, y1);
      NDCtoPixel(u[i + 1], v[i + 1], x2, y2);
      fRenderer->AddLine(x1, y1, x2, y2, r, g, b, a, (float)att.GetLineWidth());
   }
}

void TVulkanPadPainter::DrawPolyMarker(Int_t n, const Float_t *x, const Float_t *y)
{
   // Simplest possible marker: a small filled square per point. Real
   // marker styles (circle, cross, star, etc. - see TAttMarker) would need
   // per-style geometry here, same gap the raylib version likely has too.
   float r, g, b, a;
   auto &att = GetAttMarker();
   GetColorRGBA(att.GetMarkerColor(), r, g, b, a);
   const float half = 2.0f;
   for (Int_t i = 0; i < n; ++i) {
      float px = (float)x[i], py = (float)y[i];
      fRenderer->AddFilledQuad(px - half, py - half, px + half, py - half,
                                px + half, py + half, px - half, py + half, r, g, b, a);
   }
}


void TVulkanPadPainter::DrawPolyMarker(Int_t n, const Double_t *x, const Double_t *y)
{
   // Simplest possible marker: a small filled square per point. Real
   // marker styles (circle, cross, star, etc. - see TAttMarker) would need
   // per-style geometry here, same gap the raylib version likely has too.
   float r, g, b, a;
   auto &att = GetAttMarker();
   GetColorRGBA(att.GetMarkerColor(), r, g, b, a);
   const float half = 2.0f;
   for (Int_t i = 0; i < n; ++i) {
      float px = (float)x[i], py = (float)y[i];
      fRenderer->AddFilledQuad(px - half, py - half, px + half, py - half,
                                px + half, py + half, px - half, py + half, r, g, b, a);
   }
}

void TVulkanPadPainter::DrawText(Double_t /*x*/, Double_t /*y*/, const char * /*text*/,
                                  TVirtualPadPainter::ETextMode /*mode*/)
{
   // Text rendering needs a font atlas + textured pipeline (see the
   // "intentionally left out" note in VulkanRenderer.h) - not wired up in
   // this skeleton. Plug in a stb_truetype-baked atlas and a second
   // pipeline with a sampler descriptor to fill this in.
}

void TVulkanPadPainter::DrawTextNDC(Double_t /*u*/, Double_t /*v*/, const char * /*text*/,
                                     TVirtualPadPainter::ETextMode /*mode*/)
{
   // Same as DrawText, in NDC space - convert with NDCtoPixel() then forward.
}
