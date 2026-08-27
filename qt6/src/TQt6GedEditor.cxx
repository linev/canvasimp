// Author: Sergey Linev, GSI   27/08/2026

/*************************************************************************
 * Copyright (C) 1995-2026, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

#include "TQt6GedEditor.h"

#include "TCanvas.h"

using namespace ROOT::Experimental;

/** \class TQt6GedEditor
    \ingroup qt6canvas
    \brief TVirtualPadEditor ABI implementation for Qt6
*/

TQt6GedEditor::TQt6GedEditor(TCanvas *c)
{
   SetCanvas(c);
}

TQt6GedEditor::~TQt6GedEditor()
{

}


void TQt6GedEditor::SetCanvas(TCanvas *newcan)
{
   if (fCanvas == newcan) return;

   DisconnectFromCanvas();
   fCanvas = newcan;

   if (!newcan) return;

   // SetWindowName(Form("%s_Editor", fCanvas->GetName()));
   fPad = fCanvas->GetSelectedPad();
   if (!fPad) fPad = fCanvas;
   ConnectToCanvas(fCanvas);
}


////////////////////////////////////////////////////////////////////////////////
/// Connect this editor to the Selected signal of canvas 'c'.

void TQt6GedEditor::ConnectToCanvas(TCanvas *c)
{
   printf("Connect to canvas %p\n", c);
   c->Connect("Selected(TVirtualPad*,TObject*,Int_t)", "ROOT::Experimental::TQt6GedEditor",
              this, "SetModel(TVirtualPad*,TObject*,Int_t)");
}

////////////////////////////////////////////////////////////////////////////////
/// Disconnect this editor from the Selected signal of fCanvas.

void TQt6GedEditor::DisconnectFromCanvas()
{
   if (fCanvas)
      Disconnect(fCanvas, "Selected(TVirtualPad*,TObject*,Int_t)", this, "SetModel(TVirtualPad*,TObject*,Int_t)");
}

////////////////////////////////////////////////////////////////////////////////
/// Activate object editors according to the selected object.

void TQt6GedEditor::SetModel(TVirtualPad* pad, TObject* obj, Int_t event, Bool_t force)
{
   if (event != kButton1Down)
      return;

   fPad = pad;
   fModel = obj ? obj : pad;

   printf("SetModel %p %s\n", fModel, fModel->ClassName());
}

void TQt6GedEditor::Show()
{
   if (gPad)
      SetCanvas(gPad->GetCanvas());

   if (fCanvas && fGlobal)
      SetModel(fCanvas->GetClickSelectedPad(), fCanvas->GetClickSelected(), kButton1Down);

   printf("Show editor\n");
}

void TQt6GedEditor::Hide()
{

}
