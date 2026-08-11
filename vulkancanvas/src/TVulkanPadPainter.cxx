// Author: Sergey Linev, GSI  06/08/2026

#include "TVulkanPadPainter.h"

#include "TError.h"
#include "TSystem.h"
#include "TStyle.h"
#include "TPad.h"
#include "TROOT.h"
#include "TColor.h"

#include <vector>
#include <string>

using namespace ROOT::Experimental;

const Float_t kScale = 0.75f * 0.93376068f;

VkDrawCommands &TVulkanPadPainter::GetDrawCommands() {
   static thread_local VkDrawCommands cmds;
   return cmds;
}

void TVulkanPadPainter::SetOpacity(Int_t percent) {}

static VkVertex MakeVert(Double_t px, Double_t py, UInt_t colId) {
   VkVertex v = {};
   v.x = (float)px; v.y = (float)py;
   if (colId > 0 && gROOT) {
      auto *tc = dynamic_cast<TColor*>(gROOT->GetColor(colId));
      if (tc) { v.r = tc->GetRed(); v.g = tc->GetGreen(); v.b = tc->GetBlue(); }
      else   { v.r = v.g = v.b = 1.0f; }
   } else { v.r = v.g = v.b = 1.0f; }
   v.a = 1.0f;
   return v;
}

void TVulkanPadPainter::DrawLine(Double_t x1, Double_t y1, Double_t x2, Double_t y2) {
   auto att = GetAttLine();
   if (att.GetLineWidth() <= 0) return;
   Int_t px1 = gPad->XtoAbsPixel(x1), py1 = gPad->YtoAbsPixel(y1);
   Int_t px2 = gPad->XtoAbsPixel(x2), py2 = gPad->YtoAbsPixel(y2);
   VkCmdLine cmd;
   cmd.v[0] = MakeVert(px1, py1, att.GetLineColor());
   cmd.v[1] = MakeVert(px2, py2, att.GetLineColor());
   GetDrawCommands().lines.push_back(cmd);
}

void TVulkanPadPainter::DrawLineNDC(Double_t u1, Double_t v1, Double_t u2, Double_t v2) {
   auto att = GetAttLine();
   if (att.GetLineWidth() <= 0) return;
   Int_t px1 = gPad->UtoAbsPixel(u1), py1 = gPad->VtoAbsPixel(v1);
   Int_t px2 = gPad->UtoAbsPixel(u2), py2 = gPad->VtoAbsPixel(v2);
   VkCmdLine cmd;
   cmd.v[0] = MakeVert(px1, py1, att.GetLineColor());
   cmd.v[1] = MakeVert(px2, py2, att.GetLineColor());
   GetDrawCommands().lines.push_back(cmd);
}

void TVulkanPadPainter::DrawBox(Double_t x1, Double_t y1, Double_t x2, Double_t y2,
                                EBoxMode mode) {
   auto attLine = GetAttLine();
   auto attFill = GetAttFill();
   Int_t px1 = gPad->XtoAbsPixel(x1), py1 = gPad->YtoAbsPixel(y1);
   Int_t px2 = gPad->XtoAbsPixel(x2), py2 = gPad->YtoAbsPixel(y2);
   if (mode == kHollow && attLine.GetLineWidth() <= 0) return;
   VkCmdBox cmd;
   UInt_t col = (mode == kHollow) ? attLine.GetLineColor() : attFill.GetFillColor();
   cmd.v[0] = MakeVert(px1, py1, col);
   cmd.v[1] = MakeVert(px2, py1, col);
   cmd.v[2] = MakeVert(px2, py2, col);
   cmd.v[3] = MakeVert(px1, py2, col);
   GetDrawCommands().boxes.push_back(cmd);
}

void TVulkanPadPainter::DrawFillArea(Int_t n, const Double_t *x, const Double_t *y) {
   auto att = GetAttFill();
   if (n < 3 || att.GetFillStyle() <= 0) return;
   VkCmdFillArea cmd;
   cmd.n = n; cmd.verts.resize(n);
   UInt_t col = att.GetFillColor();
   for (Int_t i = 0; i < n; i++) {
      Int_t px = gPad->XtoAbsPixel(x[i]), py = gPad->YtoAbsPixel(y[i]);
      cmd.verts[i] = MakeVert(px, py, col);
   }
   GetDrawCommands().fillAreas.push_back(std::move(cmd));
}

void TVulkanPadPainter::DrawFillArea(Int_t n, const Float_t *x, const Float_t *y) {
   std::vector<Double_t> xd(n), yd(n);
   for (int i = 0; i < n; i++) { xd[i] = x[i]; yd[i] = y[i]; }
   DrawFillArea(n, xd.data(), yd.data());
}

void TVulkanPadPainter::DrawPolyLine(Int_t n, const Double_t *x, const Double_t *y) {
   auto att = GetAttLine();
   if (n < 2 || att.GetLineWidth() <= 0) return;
   VkCmdPolyLine cmd; cmd.n = n; cmd.verts.resize(n);
   UInt_t col = att.GetLineColor();
   for (Int_t i = 0; i < n; i++) {
      Int_t px = gPad->XtoAbsPixel(x[i]), py = gPad->YtoAbsPixel(y[i]);
      cmd.verts[i] = MakeVert(px, py, col);
   }
   GetDrawCommands().polyLines.push_back(std::move(cmd));
}

void TVulkanPadPainter::DrawPolyLine(Int_t n, const Float_t *x, const Float_t *y) {
   std::vector<Double_t> xd(n), yd(n);
   for (int i = 0; i < n; i++) { xd[i] = x[i]; yd[i] = y[i]; }
   DrawPolyLine(n, xd.data(), yd.data());
}

void TVulkanPadPainter::DrawPolyLineNDC(Int_t n, const Double_t *u, const Double_t *v) {
   auto att = GetAttLine();
   if (n < 2 || att.GetLineWidth() <= 0) return;
   VkCmdPolyLine cmd; cmd.n = n; cmd.verts.resize(n);
   UInt_t col = att.GetLineColor();
   for (Int_t i = 0; i < n; i++) {
      Int_t px = gPad->UtoAbsPixel(u[i]), py = gPad->VtoAbsPixel(v[i]);
      cmd.verts[i] = MakeVert(px, py, col);
   }
   GetDrawCommands().polyLines.push_back(std::move(cmd));
}

void TVulkanPadPainter::DrawPolyMarker(Int_t n, const Double_t *x, const Double_t *y) {
   auto att = GetAttMarker();
   if (n < 1) return;
   VkCmdPolyMarker cmd; cmd.n = n; cmd.verts.resize(n);
   UInt_t col = att.GetMarkerColor();
   for (Int_t i = 0; i < n; i++) {
      Int_t px = gPad->XtoAbsPixel(x[i]), py = gPad->YtoAbsPixel(y[i]);
      cmd.verts[i] = MakeVert(px, py, col);
   }
   GetDrawCommands().polyMarkers.push_back(std::move(cmd));
}

void TVulkanPadPainter::DrawPolyMarker(Int_t n, const Float_t *x, const Float_t *y) {
   std::vector<Double_t> xd(n), yd(n);
   for (int i = 0; i < n; i++) { xd[i] = x[i]; yd[i] = y[i]; }
   DrawPolyMarker(n, xd.data(), yd.data());
}

void TVulkanPadPainter::PaintTextHelper(float px, float py, const char *text) {
   auto att = GetAttText();
   VkCmdText cmd;
   cmd.x = px; cmd.y = py; cmd.text = text;
   cmd.color = att.GetTextColor();
   GetDrawCommands().texts.push_back(cmd);
}

void TVulkanPadPainter::DrawText(Double_t x, Double_t y, const char *text, ETextMode /*mode*/) {
   Int_t px = gPad->XtoAbsPixel(x), py = gPad->YtoAbsPixel(y);
   PaintTextHelper((float)px, (float)py, text);
}

void TVulkanPadPainter::DrawText(Double_t x, Double_t y, const wchar_t *text,
                                 ETextMode /*mode*/) {
   std::wstring ws(text);
   std::string utf8(ws.begin(), ws.end());
   DrawText(x, y, utf8.c_str(), TVirtualPadPainter::kClear);
}

void TVulkanPadPainter::DrawTextNDC(Double_t u, Double_t v, const char *text,
                                    ETextMode /*mode*/) {
   Int_t px = gPad->UtoAbsPixel(u), py = gPad->VtoAbsPixel(v);
   PaintTextHelper((float)px, (float)py, text);
}

void TVulkanPadPainter::DrawTextNDC(Double_t u, Double_t v, const wchar_t *text,
                                    ETextMode /*mode*/) {
   std::wstring ws(text);
   std::string utf8(ws.begin(), ws.end());
   DrawTextNDC(u, v, utf8.c_str(), TVirtualPadPainter::kClear);
}

void TVulkanPadPainter::DrawTextUrl(Double_t x, Double_t y, const char *text,
                                    const char * /*url*/) {
   Int_t px = gPad->XtoAbsPixel(x), py = gPad->YtoAbsPixel(y);
   PaintTextHelper((float)px, (float)py, text);
}

// ---- Cursor / Image / Pixels (stubs) ----
void TVulkanPadPainter::SetCursor(Int_t, ECursor) {}
void TVulkanPadPainter::SaveImage(TVirtualPad *, const char *, Int_t) const {}
void TVulkanPadPainter::DrawPixels(const unsigned char *, UInt_t, UInt_t,
                                    Int_t, Int_t, Bool_t) {}

// ---- Text measurement (stubs using font-free estimation) ----
void TVulkanPadPainter::GetTextExtent(Font_t /*font*/, Double_t size, UInt_t &w,
                                       UInt_t &h, const char *mess) {
   w = (UInt_t)(std::strlen(mess) * size * 0.6);
   h = (UInt_t)(size);
}

void TVulkanPadPainter::GetTextExtent(Font_t /*font*/, Double_t size, UInt_t &w,
                                       UInt_t &h, const wchar_t *mess) {
   w = (UInt_t)(std::wcslen(mess) * size * 0.6);
   h = (UInt_t)(size);
}

void TVulkanPadPainter::GetTextAscentDescent(Font_t /*font*/, Double_t size,
                                              UInt_t &a, UInt_t &d, const char * /*mess*/) {
   a = (UInt_t)(size * 0.8);
   d = (UInt_t)(size * 0.2);
}

void TVulkanPadPainter::GetTextAscentDescent(Font_t /*font*/, Double_t size,
                                              UInt_t &a, UInt_t &d, const wchar_t * /*mess*/) {
   a = (UInt_t)(size * 0.8);
   d = (UInt_t)(size * 0.2);
}

UInt_t TVulkanPadPainter::GetTextAdvance(Font_t /*font*/, Double_t size,
                                          const char *text, Bool_t /*kern*/) {
   return (UInt_t)(std::strlen(text) * size * 0.6);
}

ClassImp(TVulkanPadPainter)
