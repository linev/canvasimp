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
#include "TROOT.h"
#include "TColor.h"
#include "TAttMarker.h"
#include "TQt6Canvas.h"
#include "QCanvasWidget.h"

#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QComboBox>
#include <QSpinBox>
#include <QPushButton>
#include <QLabel>
#include <QColorDialog>
#include <QDoubleSpinBox>
#include <QGroupBox>

#include <memory>

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
   Hide();
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

   auto prev = fModel;

   fPad = pad;
   fModel = obj ? obj : pad;

   printf("SetModel %p %s\n", fModel, fModel->ClassName());

   if (fModel != prev)
      FillDialogsElements();
}

void TQt6GedEditor::Show()
{
   if (gPad)
      SetCanvas(gPad->GetCanvas());

   if (fCanvas && fGlobal)
      SetModel(fCanvas->GetClickSelectedPad(), fCanvas->GetClickSelected(), kButton1Down);

   printf("Show editor\n");

   if (!gROOT->GetListOfCleanups()->FindObject(this))
      gROOT->GetListOfCleanups()->Add(this);


   fDialog = new QDialog;
   fDialog->setWindowTitle("Edit Attributes");
   fDialog->setModal(false);

   auto imp = dynamic_cast<TQt6Canvas *>(fCanvas->GetCanvasImp());
   if (imp) {
     auto widget = imp->GetCanvasWidget();
     fDialog->resize(200, widget->height());
     QPoint pos = widget->mapToGlobal(QPoint(0, 0));
     fDialog->move(pos.x() - fDialog->width(), pos.y());
   }

   auto mainLayout = new QVBoxLayout(fDialog);
   fFormLayout = new QFormLayout();

   mainLayout->addLayout(fFormLayout);


   // --- Dialog Buttons (OK / Cancel) ---
   QHBoxLayout *buttonLayout = new QHBoxLayout();
   QPushButton *okButton = new QPushButton("OK");
   QPushButton *cancelButton = new QPushButton("Cancel");
   buttonLayout->addStretch();
   buttonLayout->addWidget(okButton);
   buttonLayout->addWidget(cancelButton);
   mainLayout->addLayout(buttonLayout);

   QObject::connect(okButton, &QPushButton::clicked, fDialog, &QDialog::accept);
   QObject::connect(cancelButton, &QPushButton::clicked, fDialog, &QDialog::reject);

   fDialog->setAttribute(Qt::WA_DeleteOnClose);

   QObject::connect(fDialog, &QDialog::finished, [this](int result) {
      printf("Dialog finished %d\n", result);
      fDialog = nullptr;
      fFormLayout = nullptr;
   });

   if (fModel)
      FillDialogsElements();

   fDialog->show();
}

void TQt6GedEditor::AddHLine(QFormLayout *f, const char *lbl)
{
   QWidget *container = new QWidget(fDialog);
   QHBoxLayout *layout = new QHBoxLayout(container);
   layout->setContentsMargins(0, 5, 0, 5);

   QFrame *leftLine = new QFrame(fDialog);
   leftLine->setFrameShape(QFrame::HLine);

   QLabel *label = new QLabel(lbl, fDialog);

   QFrame *rightLine = new QFrame(fDialog);
   rightLine->setFrameShape(QFrame::HLine);

   layout->addWidget(leftLine, 1);  // Stretch factor 1
   layout->addWidget(label, 0);     // Fits content tightly
   layout->addWidget(rightLine, 4); // Stret

   f->addRow(container);
}

void TQt6GedEditor::AddColorElements(int colindx, QFormLayout *layout, std::function<void(int)> callback)
{
   TColor *rootColor = gROOT->GetColor(colindx);
   QColor initialColor = Qt::black;
   int initialAlpha255 = 255; // Default fully opaque

   if (rootColor) {
      initialColor = QColor(rootColor->GetRed() * 255, rootColor->GetGreen() * 255, rootColor->GetBlue() * 255);
      initialAlpha255 = static_cast<int>(rootColor->GetAlpha() * 255);
   }
   initialColor.setAlpha(initialAlpha255);

   auto colorButton = new QPushButton();
   colorButton->setFixedWidth(80);

   QSlider *alphaSlider = new QSlider(Qt::Horizontal);
   alphaSlider->setRange(0, 255);
   alphaSlider->setValue(initialAlpha255);

   // instance will be deleted when last lambda is removed
   auto selectedColor = std::make_shared<QColor>(initialColor);

   auto updateColorElements = [colorButton, selectedColor, callback]() {
      QString qss = QString("background-color: rgba(%1, %2, %3, %4); border: 1px solid gray;")
                        .arg(selectedColor->red())
                        .arg(selectedColor->green())
                        .arg(selectedColor->blue())
                        .arg(selectedColor->alpha() / 255.0);
      colorButton->setStyleSheet(qss);
      Color_t newColorIdx = TColor::GetColor(selectedColor->red(),
                                             selectedColor->green(),
                                             selectedColor->blue(),
                                             selectedColor->alpha() / 255.0);
      callback(newColorIdx);
   };

   updateColorElements();

   QObject::connect(colorButton, &QPushButton::clicked, [selectedColor, updateColorElements]() {
      QColor col = QColorDialog::getColor(*selectedColor, nullptr, "Select Color");
      if (col.isValid()) {
         selectedColor->setRed(col.red());
         selectedColor->setGreen(col.green());
         selectedColor->setBlue(col.blue());
         updateColorElements();
      }
   });

   // --- Slider Shift Connection ---
   QObject::connect(alphaSlider, &QSlider::valueChanged, [selectedColor, updateColorElements](int value) {
      selectedColor->setAlpha(value);
      updateColorElements();
   });

   layout->addRow("Color:", colorButton);

   layout->addRow("Opacity:", alphaSlider);
}

void TQt6GedEditor::ModifiedPad()
{
   auto pad = fPad;
   if (!pad)
      pad = fCanvas;
   if (pad)
      pad->ModifiedUpdate();
}


void TQt6GedEditor::FillDialogsElements()
{
   if (!fFormLayout || !fModel)
      return;

   // first delete all elements
   while (fFormLayout->count() > 0) {
      // Always take from index 0 or use a reverse loop
      auto item = fFormLayout->takeAt(0);

      if (QWidget *widget = item->widget())
         widget->deleteLater(); // Safely deletes the widget

      delete item; // Deletes the layout item wrapper
   }

   auto attmarker = dynamic_cast<TAttMarker *>(fModel);
   if (attmarker) {
      AddHLine(fFormLayout, "TAttMarker");

      AddColorElements(attmarker->GetMarkerColor(), fFormLayout, [attmarker, this](int colindx) {
         attmarker->SetMarkerColor(colindx);
         ModifiedPad();
      });

      QComboBox *styleCombo = new QComboBox();
      for (int s = 1; s <= 49; ++s)
         styleCombo->addItem(QString("Style %1").arg(s), s);

      // Find and set current style
      int currentStyle = attmarker->GetMarkerStyle();
      int styleIdx = styleCombo->findData(currentStyle);
      if (styleIdx != -1)
         styleCombo->setCurrentIndex(styleIdx);
      else
         styleCombo->addItem(QString("Style %1").arg(currentStyle), currentStyle);

      QObject::connect(styleCombo, &QComboBox::currentIndexChanged, [this, attmarker](int indx) {
         attmarker->SetMarkerStyle(indx + 1);
         ModifiedPad();
      });

      fFormLayout->addRow("Style:", styleCombo);

      QDoubleSpinBox *floatSpinBox = new QDoubleSpinBox();
      floatSpinBox->setRange(0.0, 100.0); // Set your minimum and maximum limits
      floatSpinBox->setSingleStep(1);  // Set step size to 1
      floatSpinBox->setDecimals(1);      // Force it to show exactly 1 decimal place (e.g., 1.5)
      floatSpinBox->setValue(attmarker->GetMarkerSize());
      fFormLayout->addRow("Size:", floatSpinBox);

      QObject::connect(floatSpinBox, &QDoubleSpinBox::valueChanged, [this, attmarker](double v) {
        attmarker->SetMarkerSize(v);
        ModifiedPad();
      });
   }
}

void TQt6GedEditor::Hide()
{
   gROOT->GetListOfCleanups()->Remove(this);
   if (fDialog) {
      fDialog->close();
      fDialog = nullptr;
      fFormLayout = nullptr;
   }
}

void TQt6GedEditor::RecursiveRemove(TObject* obj)
{
   if (obj == fModel) {
      SetModel(fPad, fPad, kButton1Down);
   } else if (obj == fPad) {
      SetModel(fCanvas, fCanvas, kButton1Down);
   } else if (obj == fCanvas)
      Hide();
}
