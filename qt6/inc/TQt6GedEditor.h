// Author: Sergey Linev, GSI   27/08/2026

/*************************************************************************
 * Copyright (C) 1995-2026, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

#ifndef ROOT_TQt6GedEditor
#define ROOT_TQt6GedEditor

#include "TVirtualPadEditor.h"
#include "TQObject.h"

#include <functional>

class TVirtualPad;
class QDialog;
class QFormLayout;
class QVBoxLayout;


namespace ROOT {
namespace Experimental {

class TQt6GedEditor : public TVirtualPadEditor, public TObject, public TQObject {

   protected:

      TCanvas *fCanvas = nullptr;
      TVirtualPad *fPad = nullptr;
      TObject *fModel = nullptr;
      Bool_t fGlobal = kTRUE;

      QDialog *fDialog = nullptr;
      QFormLayout *fFormLayout = nullptr;

      void FillDialogsElements();

      void ModifiedPad();

      void AddHLine(QFormLayout *f, const char *lbl);

      void AddColorElements(int colindx, QFormLayout *layout, std::function<void(int)> callback);

   public:

      TQt6GedEditor(TCanvas *c = nullptr);
      virtual ~TQt6GedEditor();

      Bool_t   IsGlobal() const override { return fGlobal; }
      void     SetGlobal(Bool_t on) override { fGlobal = on; }

      TCanvas* GetCanvas() const override { return fCanvas; }

      void ConnectToCanvas(TCanvas *c);
      void DisconnectFromCanvas();
      void SetCanvas(TCanvas *c);

      void Show() override;
      void Hide() override;

      void RecursiveRemove(TObject* obj) override;

      virtual void  SetModel(TVirtualPad* pad, TObject* obj, Int_t event, Bool_t force =kFALSE);

   ClassDefOverride(TQt6GedEditor, 0) // Implementation for Ged Editor with Qt6
};

} // namespace Experimental
} // namespace ROOT

#endif