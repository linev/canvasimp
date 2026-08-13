// Author:  Sergey Linev, GSI  12/08/2026

/*************************************************************************
 * Copyright (C) 1995-2026, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

#include "TGtk4PadPainter.h"
#include "TError.h"
#include "TSystem.h"
#include "TStyle.h"
#include "TEnv.h"
#include "TMath.h"
#include "TPad.h"
#include "TPoint.h"
#include "TROOT.h"
#include "TColor.h"
#include "RStipples.h"

#include <memory>
#include <map>

#include "Gtk4DrawArea.h"

#include <cairomm/context.h>



using namespace ROOT::Experimental;

// to scale fonts to the same size as in the TTF
const Float_t kScale = 0.75 * 0.93376068;

/** \class TGtk4PadPainter
    \ingroup gtk4canvas
    \brief Implement TVirtualPadPainter for Gtk4 graphics

   Uses Gtk4 DrawArea which creates Cairo::Context during update.
*/

//////////////////////////////////////////////////////////////////////////
/// Set opacity - similar to TVirtualPS usecase

void TGtk4PadPainter::SetOpacity(Int_t percent)
{
   fAttFill.SetFillStyle(4000 + percent);
}

//////////////////////////////////////////////////////////////////////////
/// Set cursor

void TGtk4PadPainter::SetCursor(Int_t, ECursor cursor)
{
   /*
   switch(cursor) {
      case kBottomLeft: fDrawArea->setCursor(Qt::SizeBDiagCursor); break;
      case kBottomRight: fDrawArea->setCursor(Qt::SizeFDiagCursor); break;
      case kTopLeft: fDrawArea->setCursor(Qt::SizeFDiagCursor); break;
      case kTopRight: fDrawArea->setCursor(Qt::SizeBDiagCursor); break;
      case kBottomSide: fDrawArea->setCursor(Qt::SizeVerCursor); break;
      case kLeftSide: fDrawArea->setCursor(Qt::SizeHorCursor); break;
      case kTopSide: fDrawArea->setCursor(Qt::SizeVerCursor); break;
      case kRightSide: fDrawArea->setCursor(Qt::SizeHorCursor); break;
      case kMove: fDrawArea->setCursor(Qt::DragMoveCursor); break;
      case kCross: fDrawArea->setCursor(Qt::CrossCursor); break;
      case kArrowHor: fDrawArea->setCursor(Qt::SizeHorCursor); break;
      case kArrowVer: fDrawArea->setCursor(Qt::UpArrowCursor); break;
      case kHand: fDrawArea->setCursor(Qt::OpenHandCursor); break;
      case kRotate: fDrawArea->setCursor(Qt::ClosedHandCursor); break;
      case kPointer: fDrawArea->setCursor(Qt::ArrowCursor); break;
      case kArrowRight: fDrawArea->setCursor(Qt::SizeHorCursor); break;
      case kCaret: fDrawArea->setCursor(Qt::WaitCursor); break;
      case kWatch: fDrawArea->setCursor(Qt::WaitCursor); break;
      case kNoDrop: fDrawArea->setCursor(Qt::ForbiddenCursor); break;
      default:
         fDrawArea->unsetCursor();
   }
   */
}

////////////////////////////////////////////////////////////////////////////////
///Noop, for non-gl pad TASImage calls gVirtualX->CopyArea.

void TGtk4PadPainter::DrawPixels(const unsigned char * /*pixelData*/, UInt_t /*width*/, UInt_t /*height*/,
                             Int_t /*dstX*/, Int_t /*dstY*/, Bool_t /*enableAlphaBlending*/)
{

}


////////////////////////////////////////////////////////////////////////////////
/// Assign color to cairo context

void TGtk4PadPainter::SetGtk4Color(Color_t id)
{
   auto ctx = fDrawArea->GetContext();
   if (!ctx)
      return;

   auto c = gROOT->GetColor(id);
   if (c)
      ctx->set_source_rgba(c->GetRed(), c->GetGreen(), c->GetBlue(), c->GetAlpha());
   else
      ctx->set_source_rgb(0, 0, 0);
}

////////////////////////////////////////////////////////////////////////////////
/// Asign pen attributes to draw line

Bool_t TGtk4PadPainter::SetLinePen()
{
   auto ctx = fDrawArea->GetContext();
   if (!ctx)
      return kFALSE;

   auto &att = GetAttLine();
   if (att.GetLineWidth() <= 0)
      return kFALSE;

   SetGtk4Color(att.GetLineColor());
   ctx->set_line_width(att.GetLineWidth());

   return kTRUE;

/*
   auto style = att.GetLineStyle();

   QPen customPen;
   customPen.setColor(GetQColor(att.GetLineColor()));
   customPen.setWidth(att.GetLineWidth());
   customPen.setStyle(Qt::SolidLine);

   TString patt;

   if (style > 1)
      patt = gStyle->GetLineStyleString(style);

   if (patt.Length() > 2) {
      QList<qreal> pattern;
      std::unique_ptr<TObjArray> tokens(patt.Tokenize(" "));
      for (Int_t j = 0; j < tokens->GetEntries(); j++) {
         int it = std::stoi(tokens->At(j)->GetName());
         pattern.push_back(0.25 * it);
      }
      if (pattern.size() > 1) {
         customPen.setStyle(Qt::CustomDashLine);
         customPen.setDashPattern(pattern);
      }
   }

   return customPen;
*/
}


////////////////////////////////////////////////////////////////////////////////
/// Asign brush attributes to fill line

Bool_t TGtk4PadPainter::SetFillBrush()
{
   auto ctx = fDrawArea->GetContext();
   if (!ctx)
      return kFALSE;

   auto &att = GetAttFill();

   SetGtk4Color(att.GetFillColor());
   return kTRUE;

/*
   Int_t style = att.GetFillStyle() / 1000;

   if (style == 1)
      return QBrush(GetQColor(att.GetFillColor()));

   if (style == 3) {
      Int_t fasi  = att.GetFillStyle() % 1000;
      Int_t stn = (fasi >= 1 && fasi <=25) ? fasi : 2;
      QBitmap bitmap = QBitmap::fromData(QSize(16, 16), (uchar *)gStipples[stn]);
      QImage image = bitmap.toImage();
      image.setColor(0, qRgba(0, 0, 0, 0)); // transparent
      image.setColor(1, GetQColor(att.GetFillColor()).rgba());
      return QBrush(QPixmap::fromImage(image.copy()));
   }

   return QBrush(Qt::NoBrush);
*/
}





////////////////////////////////////////////////////////////////////////////////
/// Paint a simple line.

void TGtk4PadPainter::DrawLine(Double_t x1, Double_t y1, Double_t x2, Double_t y2)
{
   if (!SetLinePen())
      return;

   const Int_t px1 = gPad->XtoAbsPixel(x1);
   const Int_t py1 = gPad->YtoAbsPixel(y1);
   const Int_t px2 = gPad->XtoAbsPixel(x2);
   const Int_t py2 = gPad->YtoAbsPixel(y2);

   auto ctx = fDrawArea->GetContext();

   ctx->move_to(px1, py1);
   ctx->line_to(px2, py2);
}


////////////////////////////////////////////////////////////////////////////////
/// Paint a simple line in normalized coordinates.

void TGtk4PadPainter::DrawLineNDC(Double_t u1, Double_t v1, Double_t u2, Double_t v2)
{
   if (!SetLinePen())
      return;

   const Int_t px1 = gPad->UtoAbsPixel(u1);
   const Int_t py1 = gPad->VtoAbsPixel(v1);
   const Int_t px2 = gPad->UtoAbsPixel(u2);
   const Int_t py2 = gPad->VtoAbsPixel(v2);

   auto ctx = fDrawArea->GetContext();
   ctx->move_to(px1, py1);
   ctx->line_to(px2, py2);
}

////////////////////////////////////////////////////////////////////////////////
/// Paint a simple box.

void TGtk4PadPainter::DrawBox(Double_t x1, Double_t y1, Double_t x2, Double_t y2, EBoxMode mode)
{
   const Int_t px1 = gPad->XtoAbsPixel(x1);
   const Int_t py1 = gPad->YtoAbsPixel(y1);
   const Int_t px2 = gPad->XtoAbsPixel(x2);
   const Int_t py2 = gPad->YtoAbsPixel(y2);

   auto ctx = fDrawArea->GetContext();
   if (!ctx)
      return;

   ctx->rectangle(TMath::Min(px1, px2), TMath::Min(py1, py2), TMath::Abs(px2 - px1), TMath::Abs(py2 - py1));

   if (mode == TVirtualPadPainter::kHollow) {
      if (SetLinePen())
         ctx->stroke();
   } else {
      if (SetFillBrush())
         ctx->fill();
   }
}

////////////////////////////////////////////////////////////////////////////////
/// Paint filled area.

void TGtk4PadPainter::DrawFillArea(Int_t nPoints, const Double_t *xs, const Double_t *ys)
{
   auto ctx = fDrawArea->GetContext();

   if ((nPoints < 3) || !ctx)
      return;

   for (Int_t n = 0; n < nPoints; ++n) {
      auto px = gPad->XtoAbsPixel(xs[n]);
      auto py = gPad->YtoAbsPixel(ys[n]);
      if (n == 0)
         ctx->move_to(px, py);
      else
         ctx->line_to(px, py);
   }

   ctx->close_path();

   if (SetFillBrush())
      ctx->fill();
}

////////////////////////////////////////////////////////////////////////////////
/// Paint filled area.

void TGtk4PadPainter::DrawFillArea(Int_t nPoints, const Float_t *xs, const Float_t *ys)
{
   auto ctx = fDrawArea->GetContext();
   if ((nPoints < 3) || !ctx)
      return;

   for (Int_t n = 0; n < nPoints; ++n) {
      auto px = gPad->XtoAbsPixel(xs[n]);
      auto py = gPad->YtoAbsPixel(ys[n]);
      if (n == 0)
         ctx->move_to(px, py);
      else
         ctx->line_to(px, py);
   }

   ctx->close_path();

   if (SetFillBrush())
      ctx->fill();
}

////////////////////////////////////////////////////////////////////////////////
/// Paint Polyline.

void TGtk4PadPainter::DrawPolyLine(Int_t nPoints, const Double_t *xs, const Double_t *ys)
{
   auto ctx = fDrawArea->GetContext();
   if ((nPoints < 2) || !ctx)
      return;

   for (Int_t n = 0; n < nPoints; ++n) {
      auto px = gPad->XtoAbsPixel(xs[n]);
      auto py = gPad->YtoAbsPixel(ys[n]);
      if (n == 0)
         ctx->move_to(px, py);
      else
         ctx->line_to(px, py);
   }

   if (SetLinePen())
      ctx->stroke();
}

////////////////////////////////////////////////////////////////////////////////
/// Paint polyline.

void TGtk4PadPainter::DrawPolyLine(Int_t nPoints, const Float_t *xs, const Float_t *ys)
{
   auto ctx = fDrawArea->GetContext();
   if ((nPoints < 2) || !ctx)
      return;

   for (Int_t n = 0; n < nPoints; ++n) {
      auto px = gPad->XtoAbsPixel(xs[n]);
      auto py = gPad->YtoAbsPixel(ys[n]);
      if (n == 0)
         ctx->move_to(px, py);
      else
         ctx->line_to(px, py);
   }

   if (SetLinePen())
      ctx->stroke();
}

////////////////////////////////////////////////////////////////////////////////
/// Paint polyline in normalized coordinates.

void TGtk4PadPainter::DrawPolyLineNDC(Int_t nPoints, const Double_t *u, const Double_t *v)
{
   auto ctx = fDrawArea->GetContext();
   if ((nPoints < 2) || !ctx)
      return;

   for (Int_t n = 0; n < nPoints; ++n) {
      auto px = gPad->UtoAbsPixel(u[n]);
      auto py = gPad->VtoAbsPixel(v[n]);
      if (n == 0)
         ctx->move_to(px, py);
      else
         ctx->line_to(px, py);
   }

   if (SetLinePen())
      ctx->stroke();
}

////////////////////////////////////////////////////////////////////////////////
/// Helper function to paint markers

template<typename T>
void drawMarkersHelper(Int_t nPoints, const T *x, const T *y,
                       const TAttMarker &attr, Cairo::Context *ctx)
{
   if (!ctx || nPoints < 1)
      return;

   auto markerLineWidth = TAttMarker::GetMarkerLineWidth(attr.GetMarkerStyle());
   Int_t markerSize = 0;              ///< size of simple markers
   std::vector<TPoint> markerShape;   ///< marker shape points
   auto markerType = attr.GetMarkerShape(markerSize, markerShape);

   if ((markerType == TAttMarker::kShapeDot) || (markerLineWidth > 0)) {
      ctx->set_line_width(markerLineWidth);
   } else {
   }

   for (Int_t n = 0; n < nPoints; ++n) {
      Int_t px = gPad->XtoAbsPixel(x[n]);
      Int_t py = gPad->YtoAbsPixel(y[n]);
      Int_t cnt = 0;

      switch(markerType) {
         case TAttMarker::kShapeDot:
            ctx->rectangle(px + 0.5, py + 0.5, 1., 1.);
            ctx->fill();
            break;
         case TAttMarker::kShapeCircle:
            ctx->arc(px, py, markerSize / 2, 0.0, 2.0 * TMath::Pi());
            ctx->stroke();
            break;
         case TAttMarker::kShapeFilledCircle:
            // hollow or filled circle
            ctx->arc(px, py, markerSize / 2, 0.0, 2.0 * TMath::Pi());
            ctx->fill();
            break;
         case TAttMarker::kShapePolyLine:
            // hollow polygon
            for (auto &pnt : markerShape)
               if (cnt++)
                  ctx->line_to(px + pnt.fX, py + pnt.fY);
               else
                  ctx->move_to(px + pnt.fX, py + pnt.fY);
            ctx->stroke();
            break;
         case TAttMarker::kShapeFilledArea:
            // filled polygon
            for (auto &pnt : markerShape)
               if (cnt++)
                  ctx->line_to(px + pnt.fX, py + pnt.fY);
               else
                  ctx->move_to(px + pnt.fX, py + pnt.fY);
            ctx->close_path();
            ctx->fill();
            break;
         case TAttMarker::kShapeSegments:
            // segmented line
            for (size_t i = 0; i < markerShape.size() - 1; i += 2) {
               ctx->move_to(px + markerShape[i].fX, py + markerShape[i].fY);
               ctx->line_to(px + markerShape[i+1].fX, py + markerShape[i+1].fY);
               if (i == markerShape.size() - 2)
                  ctx->stroke();
               else
                  ctx->stroke_preserve();
            }
            break;
         case TAttMarker::kShapeTriangles:
            // filled triangles
            for (size_t i = 0; i < markerShape.size() - 2; i += 3) {
               ctx->move_to(px + markerShape[i].fX, py + markerShape[i].fY);
               ctx->line_to(px + markerShape[i+1].fX, py + markerShape[i+1].fY);
               ctx->line_to(px + markerShape[i+2].fX, py + markerShape[i+2].fY);
               ctx->close_path();
               if (i == markerShape.size() - 3)
                  ctx->fill();
               else
                  ctx->fill_preserve();
            }
            break;
      }
   }
}

////////////////////////////////////////////////////////////////////////////////
/// Paint polymarker.

void TGtk4PadPainter::DrawPolyMarker(Int_t nPoints, const Double_t *x, const Double_t *y)
{
   SetGtk4Color(GetAttMarker().GetMarkerColor());

   drawMarkersHelper<Double_t>(nPoints, x, y,
                              GetAttMarker(), fDrawArea->GetContext());
}

////////////////////////////////////////////////////////////////////////////////
/// Paint polymarker.

void TGtk4PadPainter::DrawPolyMarker(Int_t nPoints, const Float_t *x, const Float_t *y)
{
   SetGtk4Color(GetAttMarker().GetMarkerColor());

   drawMarkersHelper<Float_t>(nPoints, x, y,
                              GetAttMarker(), fDrawArea->GetContext());
}

////////////////////////////////////////////////////////////////////////////////
/// Paint text.

void TGtk4PadPainter::DrawText(Double_t x, Double_t y, const char *text, ETextMode /*mode*/)
{
   const Int_t px = gPad->XtoAbsPixel(x);
   const Int_t py = gPad->YtoAbsPixel(y);

   PaintGtk4String(px, py, text);
}

////////////////////////////////////////////////////////////////////////////////
/// Paint text with url

void TGtk4PadPainter::DrawTextUrl(Double_t x, Double_t y, const char *text, const char * /* url */)
{
   const Int_t px = gPad->XtoAbsPixel(x);
   const Int_t py = gPad->YtoAbsPixel(y);

   PaintGtk4String(px, py, text);
}

////////////////////////////////////////////////////////////////////////////////
/// Special version working with wchar_t and required by TMathText.

void TGtk4PadPainter::DrawText(Double_t x, Double_t y, const wchar_t *text, ETextMode /*mode*/)
{
   const Int_t px = gPad->XtoAbsPixel(x);
   const Int_t py = gPad->YtoAbsPixel(y);

   PaintGtk4String(px, py, "some wchar");
}

////////////////////////////////////////////////////////////////////////////////
/// Paint text in normalized coordinates.

void TGtk4PadPainter::DrawTextNDC(Double_t u, Double_t v, const char *text, ETextMode /*mode*/)
{
   const Int_t px = gPad->UtoAbsPixel(u);
   const Int_t py = gPad->VtoAbsPixel(v);

   PaintGtk4String(px, py, text);
}

////////////////////////////////////////////////////////////////////////////////
/// Paint text in normalized coordinates.

void TGtk4PadPainter::DrawTextNDC(Double_t  u, Double_t v, const wchar_t *text, ETextMode /*mode*/)
{
   const Int_t px = gPad->UtoAbsPixel(u);
   const Int_t py = gPad->VtoAbsPixel(v);

   PaintGtk4String(px, py, "some wchar");
}


////////////////////////////////////////////////////////////////////////////////
/// Produce image

void TGtk4PadPainter::SaveImage(TVirtualPad * /* pad */, const char * /* fileName */, Int_t /* gtype */) const
{
}


////////////////////////////////////////////////////////////////////////////////
/// Return font family for specified ROOT font id
/// If necessary, register TTF font to Qt first

/*
QString TGtk4PadPainter::GetFontFamily(Font_t fontnumber)
{
   // TODO: make special generic method, used from several places
   static const char *fonttable[][2] = {
     { "Root.TTFont.0", "FreeSansBold.otf" },
     { "Root.TTFont.1", "FreeSerifItalic.otf" },
     { "Root.TTFont.2", "FreeSerifBold.otf" },
     { "Root.TTFont.3", "FreeSerifBoldItalic.otf" },
     { "Root.TTFont.4", "texgyreheros-regular.otf" },
     { "Root.TTFont.5", "texgyreheros-italic.otf" },
     { "Root.TTFont.6", "texgyreheros-bold.otf" },
     { "Root.TTFont.7", "texgyreheros-bolditalic.otf" },
     { "Root.TTFont.8", "FreeMono.otf" },
     { "Root.TTFont.9", "FreeMonoOblique.otf" },
     { "Root.TTFont.10", "FreeMonoBold.otf" },
     { "Root.TTFont.11", "FreeMonoBoldOblique.otf" },
     { "Root.TTFont.12", "symbol.ttf" },
     { "Root.TTFont.13", "FreeSerif.otf" },
     { "Root.TTFont.14", "wingding.ttf" },
     { "Root.TTFont.15", "symbol.ttf" },
     { "Root.TTFont.STIXGen", "STIXGeneral.otf" },
     { "Root.TTFont.STIXGenIt", "STIXGeneralItalic.otf" },
     { "Root.TTFont.STIXGenBd", "STIXGeneralBol.otf" },
     { "Root.TTFont.STIXGenBdIt", "STIXGeneralBolIta.otf" },
     { "Root.TTFont.STIXSiz1Sym", "STIXSiz1Sym.otf" },
     { "Root.TTFont.STIXSiz1SymBd", "STIXSiz1SymBol.otf" },
     { "Root.TTFont.STIXSiz2Sym", "STIXSiz2Sym.otf" },
     { "Root.TTFont.STIXSiz2SymBd", "STIXSiz2SymBol.otf" },
     { "Root.TTFont.STIXSiz3Sym", "STIXSiz3Sym.otf" },
     { "Root.TTFont.STIXSiz3SymBd", "STIXSiz3SymBol.otf" },
     { "Root.TTFont.STIXSiz4Sym", "STIXSiz4Sym.otf" },
     { "Root.TTFont.STIXSiz4SymBd", "STIXSiz4SymBol.otf" },
     { "Root.TTFont.STIXSiz5Sym", "STIXSiz5Sym.otf" },
     { "Root.TTFont.ME", "DroidSansFallback.ttf" },
     { "Root.TTFont.CJKMing", "DroidSansFallback.ttf" },
     { "Root.TTFont.CJKGothic", "DroidSansFallback.ttf" }
   };

   int fontid = fontnumber / 10;
   if (fontid < 0 || fontid > 31)
      fontid = 0;

   static std::map<int, QString> registeredFonts;

   auto iter = registeredFonts.find(fontid);
   if (iter != registeredFonts.end())
      return iter->second;

   const char *ttpath = gEnv->GetValue("Root.TTFontPath",
                                        TROOT::GetTTFFontDir());

   TString fname = gEnv->GetValue(fonttable[fontid][0], fonttable[fontid][1]);

   const char *ttfont = gSystem->FindFile(ttpath, fname, kReadPermission);

   if (!ttfont) {
      ::Error("TGtk4PadPainter::GetFontFamily", "Not found font %s in configured path %s", fname.Data(), ttpath);
      return "";
   }

   int qtId = QFontDatabase::addApplicationFont(ttfont);
   if (qtId == -1) {
      ::Error("TGtk4PadPainter::GetFontFamily", "No able to add font %s to QFontDataBase", ttfont);
      return "";
   }

   QString fontFamily = QFontDatabase::applicationFontFamilies(qtId).at(0);

   registeredFonts[fontid] = fontFamily;

   return fontFamily;
}

*/

Bool_t TGtk4PadPainter::SelectFont(Font_t id, Float_t size)
{
   auto ctx = fDrawArea->GetContext();
   if (!ctx)
      return kFALSE;

   ctx->select_font_face("Arial", Cairo::ToyFontFace::Slant::NORMAL, Cairo::ToyFontFace::Weight::NORMAL);

   Int_t pixelsize = (Int_t) (size*kScale+0.5);

   ctx->set_font_size(pixelsize);

   return kTRUE;
}


////////////////////////////////////////////////////////////////////////////////
/// Actual text painting image

void TGtk4PadPainter::PaintGtk4String(int x, int y, const char *s)
{
   const TAttText &att = GetAttText();

   auto textsize = att.GetTextSizePixels(*gPad);

   if (!SelectFont(att.GetTextFont(), textsize))
      return;


   auto ctx = fDrawArea->GetContext();

   Cairo::TextExtents extents;
   ctx->get_text_extents(s, extents);

   Int_t txalh = att.GetTextAlign() / 10;
   Int_t txalv = att.GetTextAlign() % 10;

   switch (txalh) {
      case 0:
      case 1: break; //left
      case 2: x -= extents.width / 2 + extents.x_bearing; break; //center
      case 3: x -= extents.width + extents.x_bearing; break; //right
   }

   switch (txalv) {
      case 1: break; //bottom
      case 2: y -= extents.height / 2 + extents.y_bearing; break; // middle
      case 3: y -= extents.height + extents.y_bearing; break; //top
   }

   ctx->move_to(x, y);

   SetGtk4Color(att.GetTextColor());

   ctx->show_text(s);
   // TODO: handle rotation
}

////////////////////////////////////////////////////////////////////////////////
/// Returns text extent

void TGtk4PadPainter::GetTextExtent(Font_t font, Double_t size, UInt_t &w, UInt_t &h, const char *mess)
{
   if (!SelectFont(font, size))
      return;

   auto ctx = fDrawArea->GetContext();

   Cairo::TextExtents extents;
   ctx->get_text_extents(mess, extents);
   w = extents.width;
   h = extents.height;
}

////////////////////////////////////////////////////////////////////////////////
/// Returns text extent

void TGtk4PadPainter::GetTextExtent(Font_t font, Double_t size, UInt_t &w, UInt_t &h, const wchar_t *mess)
{
   if (!SelectFont(font, size))
      return;

   auto ctx = fDrawArea->GetContext();

   Cairo::TextExtents extents;
   ctx->get_text_extents("Any text", extents);
   w = extents.width;
   h = extents.height;
}

////////////////////////////////////////////////////////////////////////////////
/// Returns text accent / descent

void TGtk4PadPainter::GetTextAscentDescent(Font_t font, Double_t size, UInt_t &a, UInt_t &d, const char *mess)
{
   if (!SelectFont(font, size)) {
      a = d = 0;
      return;
   }

   auto ctx = fDrawArea->GetContext();

   Cairo::FontExtents font_metrics;

   ctx->get_font_extents(font_metrics);

   a = font_metrics.ascent;
   d = font_metrics.descent;
}

////////////////////////////////////////////////////////////////////////////////
/// Returns text accent / descent

void TGtk4PadPainter::GetTextAscentDescent(Font_t font, Double_t size, UInt_t &a, UInt_t &d, const wchar_t *mess)
{
   if (!SelectFont(font, size)) {
      a = d = 0;
      return;
   }

   auto ctx = fDrawArea->GetContext();

   Cairo::FontExtents font_metrics;

   ctx->get_font_extents(font_metrics);

   a = font_metrics.ascent;
   d = font_metrics.descent;
}

////////////////////////////////////////////////////////////////////////////////
/// Returns text advance

UInt_t TGtk4PadPainter::GetTextAdvance(Font_t font, Double_t size, const char *mess, Bool_t)
{
   if (!SelectFont(font, size))
      return 0;

   auto ctx = fDrawArea->GetContext();

   Cairo::TextExtents extents;
   ctx->get_text_extents(mess, extents);
   return extents.x_advance;
}
