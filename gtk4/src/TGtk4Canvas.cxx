// Author: Sergey Linev, GSI   12/08/2026

/*************************************************************************
 * Copyright (C) 1995-2026, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

#include "TGtk4Canvas.h"

#include "TGtk4PadPainter.h"

#include "TSystem.h"
#include "TStyle.h"
#include "TCanvas.h"
#include "TROOT.h"
#include "TClass.h"

#include <cstdio>
#include <cstring>
#include <iostream>
#include <memory>

#include <gtkmm.h>

#include "Gtk4CanvasWindow.h"

using namespace ROOT::Experimental;

/** \class TGtk4Canvas
    \ingroup gtk4canvas
    \brief Basic TCanvasImp ABI implementation for Gtk4
*/

////////////////////////////////////////////////////////////////////////////////
/// Constructor

TGtk4Canvas::TGtk4Canvas(TCanvas *c, const char *name, Int_t x, Int_t y, UInt_t width, UInt_t height)
   : TCanvasImp(c, name, x, y, width, height)
{
}


////////////////////////////////////////////////////////////////////////////////
/// Destructor

TGtk4Canvas::~TGtk4Canvas()
{
   // delete fTimer;
}


////////////////////////////////////////////////////////////////////////////////
/// Initialize window for the Gtk4 canvas

Int_t TGtk4Canvas::InitWindow()
{
   return 111222333; // should not be used at all
}

////////////////////////////////////////////////////////////////////////////////
/// Creates pad painter

TVirtualPadPainter *TGtk4Canvas::CreatePadPainter()
{
   return new TGtk4PadPainter(fDrawArea);
}


//////////////////////////////////////////////////////////////////////////////////////////
/// Close canvas - not implemented

void TGtk4Canvas::Close()
{
}

//////////////////////////////////////////////////////////////////////////////////////////
/// Show canvas

void TGtk4Canvas::Show()
{
   if (fCanvasWindow)
      fCanvasWindow->present();
}

//////////////////////////////////////////////////////////////////////////////////////////
/// Returns kTRUE if web canvas has graphical editor

Bool_t TGtk4Canvas::HasEditor() const
{
   return (fClientBits & TCanvas::kShowEditor) != 0;
}

//////////////////////////////////////////////////////////////////////////////////////////
/// Returns kTRUE if web canvas has menu bar

Bool_t TGtk4Canvas::HasMenuBar() const
{
   return (fClientBits & TCanvas::kMenuBar) != 0;
}

//////////////////////////////////////////////////////////////////////////////////////////
/// Returns kTRUE if web canvas has status bar

Bool_t TGtk4Canvas::HasStatusBar() const
{
   return (fClientBits & TCanvas::kShowEventStatus) != 0;
}

//////////////////////////////////////////////////////////////////////////////////////////
/// Returns kTRUE if tooltips are activated in web canvas

Bool_t TGtk4Canvas::HasToolTips() const
{
   return (fClientBits & TCanvas::kShowToolTips) != 0;
}

//////////////////////////////////////////////////////////////////////////////////////////
/// Set window position of web canvas

void TGtk4Canvas::SetWindowPosition(Int_t, Int_t)
{
   // not directly possible in gtk4
   //if (fCanvasWindow)
   //   fCanvasWindow->move(x, y);
}

//////////////////////////////////////////////////////////////////////////////////////////
/// Set window size of web canvas

void TGtk4Canvas::SetWindowSize(UInt_t w, UInt_t h)
{
   if (fCanvasWindow)
      fCanvasWindow->set_default_size(w, h);
}

//////////////////////////////////////////////////////////////////////////////////////////
/// Set window title of web canvas

void TGtk4Canvas::SetWindowTitle(const char *newTitle)
{
   if (fCanvasWindow)
      fCanvasWindow->set_title(newTitle);
}

//////////////////////////////////////////////////////////////////////////////////////////
/// Set fixed canvas size - not supported yet in gtk

void TGtk4Canvas::SetCanvasSize(UInt_t cw, UInt_t ch)
{
   fFixedSize = kTRUE;
   if ((cw > 0) && (ch > 0)) {
      // Canvas()->fCw = cw;
      // Canvas()->fCh = ch;
   } else {
      // temporary value, will be reported back from client
      // Canvas()->fCw = Canvas()->fWindowWidth;
      // Canvas()->fCh = Canvas()->fWindowHeight;
   }
}

//////////////////////////////////////////////////////////////////////////////////////////
/// Iconify browser window

void TGtk4Canvas::Iconify()
{
   if (fCanvasWindow)
      fCanvasWindow->minimize();
}

//////////////////////////////////////////////////////////////////////////////////////////
/// Raise browser window

void TGtk4Canvas::RaiseWindow()
{
   if (fCanvasWindow)
      fCanvasWindow->present();
}

//////////////////////////////////////////////////////////////////////////////////////////
/// Assign clients bits

void TGtk4Canvas::AssignStatusBits(UInt_t bits)
{
   fClientBits = bits;
   Canvas()->SetBit(TCanvas::kShowEventStatus, bits & TCanvas::kShowEventStatus);
   Canvas()->SetBit(TCanvas::kShowEditor, bits & TCanvas::kShowEditor);
   Canvas()->SetBit(TCanvas::kShowToolTips, bits & TCanvas::kShowToolTips);
   Canvas()->SetBit(TCanvas::kMenuBar, bits & TCanvas::kMenuBar);
   Canvas()->SetBit(TCanvas::kShowToolBar, bits & TCanvas::kShowToolBar);
}

//////////////////////////////////////////////////////////////////////////////////////////
/// Returns canvas geometry

void TGtk4Canvas::GetCanvasGeometry(Int_t wid, UInt_t &w, UInt_t &h)
{
   (void) wid;
   if (fDrawArea) {
      w = fDrawArea->get_width();
      h = fDrawArea->get_height();
   } else {
      w = 780;
      h = 580;
   }
}


//////////////////////////////////////////////////////////////////////////////////////////
/// Returns window geometry including borders and menus

UInt_t TGtk4Canvas::GetWindowGeometry(Int_t &x, Int_t &y, UInt_t &w, UInt_t &h)
{
   if (fCanvasWindow) {
      // gtk4 does not deliver position
      x = y = 0;
      w = fCanvasWindow->get_width();
      h = fCanvasWindow->get_height();
   } else {
      x = y = 0;
      w = 800;
      h = 600;
   }

   return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////
/// if canvas or any subpad was modified,
/// invoke gtk4 queue_draw() which will trigger redraw area

Bool_t TGtk4Canvas::PerformUpdate(Bool_t async)
{
   if (!Canvas()->IsModified() || !fDrawArea)
      return kTRUE;

   fDrawArea->queue_draw();

   if (!async) {
      auto display = fDrawArea->get_display();
      if (display) {
         // Flushes all render requests and locks the local thread
         // until the windowing manager acknowledges processing complete.
         display->sync();
      }
   }

   return kTRUE;
}

//////////////////////////////////////////////////////////////////////////////////////////
/// Increment canvas version and force sending data to client - do not wait for reply

void TGtk4Canvas::ForceUpdate()
{
   if (fDrawArea)
      fDrawArea->queue_draw();
}


static Glib::RefPtr<Gtk::Application>  gApp;


//////////////////////////////////////////////////////////////////////////////////////////////////
/// Static method to create TGtk4Canvas instance
/// Used by plugin manager to directly create TGtk4Canvas without gui factory

TCanvasImp *TGtk4Canvas::NewCanvas(TCanvas *c, const char *name, Int_t x, Int_t y, UInt_t width, UInt_t height)
{
   if (!gApp && !Gio::Application::get_default())
      gApp = Gtk::Application::create("root.gtk4.canvasimp");

   auto widget = new Gtk4CanvasWindow(width, height);

   //widget->setWindowTitle(QString(c->GetTitle()));
   //if ((x < 0) && (y < 0))
   //   widget->resize(width, height);
   //else
   //   widget->setGeometry(x, y, width, height);
   widget->present();

   auto imp = new TGtk4Canvas(c, name, x, y, width, height);

   imp->fCanvasWindow = widget;
   imp->fDrawArea = widget->GetDrawArea();

   if (imp->fDrawArea)
      imp->fDrawArea->SetCanvas(c);

   // set all internal dimensions
   c->Resize();

   // TODO: maybe apply same logic to adjust canvas dimension as in TRootCanvas
   //       Keep commented code here intentionally to be able find this place when
   //       search for correspondent class members

   // c->fWindowTopX = x;
   // c->fWindowTopY = y;
   // c->fWindowWidth = width;
   // c->fWindowHeight = height;
   // if (!gROOT->IsBatch() && (height > 25))
   //   height -= 25;
   // c->fCw = width;
   // c->fCh = height;

   return imp;
}
