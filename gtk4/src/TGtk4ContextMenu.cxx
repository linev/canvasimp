// Author: Sergey Linev  13/08/2026

/*************************************************************************
 * Copyright (C) 1995-2026, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/


/** \class TGtk4ContextMenu
    \ingroup gtk4canvas

This class provides an interface to context-sensitive popup menus.
These menus pop up when the user hits the right mouse button, and
are destroyed when the menu pops downs.
*/


#include "TGtk4ContextMenu.h"

#include "TROOT.h"
#include "TContextMenu.h"
#include "TCanvas.h"
#include "TColor.h"
#include "TMethod.h"
#include "TDataMember.h"
#include "TToggle.h"
#include "TClassMenuItem.h"
#include "TAttText.h"
#include "TAttMarker.h"

#include "TGtk4Canvas.h"
#include "Gtk4DrawArea.h"
#include "Gtk4MethodDialog.h"

#include <iostream>

enum EContextMenu {
   kToggleStart       = 1000, // first id of toggle menu items
   kToggleListStart   = 2000, // first id of toggle list menu items
   kUserFunctionStart = 3000  // first id of user added functions/methods, etc...
};

////////////////////////////////////////////////////////////////////////////////
/// Create context menu.

TGtk4ContextMenu::TGtk4ContextMenu(TContextMenu *c, const char *)
    : TObject(), TContextMenuImp(c)
{
   gROOT->GetListOfCleanups()->Add(this);
}

////////////////////////////////////////////////////////////////////////////////
/// Delete a context menu.

TGtk4ContextMenu::~TGtk4ContextMenu()
{
   gROOT->GetListOfCleanups()->Remove(this);
   fTrash.Delete();
}

////////////////////////////////////////////////////////////////////////////////
/// Display context popup menu for currently selected object.

void TGtk4ContextMenu::DisplayPopup(Int_t x, Int_t y)
{
   // add menu items to popup menu
   // CreateMenu(fContextMenu->GetSelectedObject());

   auto object = fContextMenu->GetSelectedObject();
   if (!object)
      return;

   fCustomArg.clear();
   fTrash.Delete();


   auto canv = dynamic_cast<TCanvas *>(fContextMenu->GetSelectedCanvas());
   auto canvimp = dynamic_cast<ROOT::Experimental::TGtk4Canvas *>(canv->GetCanvasImp());
   auto widget = canvimp->GetDrawArea();

//   QPoint screenPos = widget->mapToGlobal(widget->rect().topLeft());

//   QMenu menu;
//   QSignalMapper map;

//   QObject::connect(&map, &QSignalMapper::mappedInt,
//                    this, &TGtk4ContextMenu::executeMenu);

   auto action_group = Gio::SimpleActionGroup::create();

   action_group->add_action_with_parameter(
      "action",
      Glib::VariantType("i"),  // Expected parameter type (int32)
      [this](const Glib::VariantBase& parameter) {
         if (auto var = Glib::VariantBase::cast_dynamic<Glib::Variant<int>>(parameter)) {
            int value = var.get();
            std::cout << "Action triggered with integer value: " << value << std::endl;
            executeMenu(value);
         }
      }
   );

   auto menu_model = Gio::Menu::create();

   auto current_section = Gio::Menu::create();


   auto addSeparator = [&menu_model, &current_section]() {
      menu_model->append_section(current_section);
      current_section = Gio::Menu::create();
   };

   auto addMenuAction = [&current_section, &action_group, this](const TString &text, int id, void *arg = nullptr, int checked = 0) {
      bool enabled = true;
      if ((text == "DrawClone") || (text == "DrawClass") || (text == "Inspect") ||
          (text == "SetShowProjectionX") || (text == "SetShowProjectionY") ||
          (text == "DrawPanel") || (text == "FitPanel"))
         enabled = false;


      if (checked != 0) {
         TString action_name = TString::Format("toggle-item-%d", id);
         auto action = Gio::SimpleAction::create_bool(action_name.Data(), checked > 0);
         action->signal_activate().connect([action,this, id](const Glib::VariantBase & parameter) {
            bool current;
            action->get_state(current);
            action->set_state(Glib::Variant<bool>::create(!current));
            std::cout << "Action triggered with integer value: " << id << std::endl;
            executeMenu(id);
         });
         action->set_enabled(enabled);
         action_group->add_action(action);

         action_name.Prepend("context.");
         auto item = Gio::MenuItem::create(text.Data(), action_name.Data());
         current_section->append_item(item);
      } else if (enabled) {
         auto item = Gio::MenuItem::create(text.Data(), "");
         item->set_action_and_target("context.action", Glib::Variant<int>::create(id));
         current_section->append_item(item);
      } else {
         // just refer to non-existing action
         current_section->append(text.Data(), "context.dummy");
      }

      fCustomArg[id] = arg;
   };

   // Add a title
   TString buffer = fContextMenu->CreatePopupTitle(object);
   addMenuAction(buffer, -1, nullptr);
   addSeparator();
   bool last_separ = true;

   int entry = 0, toggle = kToggleStart, togglelist = kToggleListStart;
   int userfunction = kUserFunctionStart;

   // Get list of menu items from the selected object's class
   TList *menuItemList = object->IsA()->GetMenuList();

   TIter nextItem(menuItemList);

   while (auto menuItem = (TClassMenuItem*) nextItem()) {
      switch (menuItem->GetType()) {
         case TClassMenuItem::kPopupSeparator: {
            if (!last_separ)
               addSeparator();
            last_separ = true;
            break;
         }
         case TClassMenuItem::kPopupStandardList: {
            // Standard list of class methods. Rebuild from scratch.
            // Get linked list of objects menu items (i.e. member functions
            // with the token *MENU in their comment fields.
            TList *methodList = new TList;
            object->IsA()->GetMenuItems(methodList);

            TMethod *method;
            TClass  *classPtr = nullptr;
            TIter next(methodList);
            Bool_t needSep = kFALSE;

            while ((method = (TMethod*) next())) {
               if (classPtr != method->GetClass()) {
                  needSep = kTRUE;
                  classPtr = method->GetClass();
               }

               EMenuItemKind menuKind = method->IsMenuItem();
               TString last_component;

               switch (menuKind) {
                  case kMenuDialog:
                     // search for arguments to the MENU statement
                     if (needSep) {
                        addSeparator();
                        needSep = kFALSE;
                     }
                     addMenuAction(method->GetName(), entry++, method);
                     break;
                  case kMenuSubMenu:
                     if (auto m = method->FindDataMember()) {
                        if (needSep) {
                           addSeparator();
                           needSep = kFALSE;
                        }

                        // TODO: implement selection menu
                        if (m->GetterMethod()) {
                           // QMenu *r = menu.addMenu(method->GetName());
                           TIter nxt(m->GetOptions());
                           while (auto it = (TOptionListItem*) nxt()) {
                              const char *name = it->fOptName;
                              Long_t val = it->fValue;

                              TToggle *t = new TToggle;
                              t->SetToggledObject(object, method);
                              t->SetOnValue(val);
                              fTrash.Add(t);

                              // TODO: implement checked state
                              addMenuAction(name, togglelist++, t, t->GetState() ? 1 : -1);
                           }
                        } else {
                           addMenuAction(method->GetName(), entry++, method);
                        }
                     }
                     break;

                  case kMenuToggle: {
                     if (needSep) {
                        addSeparator();
                        needSep = kFALSE;
                     }

                     TToggle *t = new TToggle;
                     t->SetToggledObject(object, method);
                     t->SetOnValue(1);
                     fTrash.Add(t);

                     addMenuAction(method->GetName(), toggle++, t, t->GetState() ? 1 : -1);
                     break;
                  }
                  default:
                     break;
               }
            }
            delete methodList;
         }
         break;
         case TClassMenuItem::kPopupUserFunction: {
            const char* menuItemTitle = menuItem->GetTitle();
            if (menuItem->IsToggle()) {
               TMethod* method = object->IsA()->GetMethodWithPrototype(menuItem->GetFunctionName(),menuItem->GetArgs());
               if (method) {
                  TToggle *t = new TToggle;
                  t->SetToggledObject(object, method);
                  t->SetOnValue(1);
                  fTrash.Add(t);

                  if (strlen(menuItemTitle)==0)
                     menuItemTitle = method->GetName();
                  addMenuAction(menuItemTitle, toggle++, t, t->GetState() ? 1 : -1);
               }
            } else {
               if (strlen(menuItemTitle)==0)
                  menuItemTitle = menuItem->GetFunctionName();
               addMenuAction(menuItemTitle, userfunction++, menuItem);
            }
            break;
         }

         default:
            break;
      }
   }

   widget->insert_action_group("context", action_group);

   widget->ShowContextMenu(menu_model, x, y);
}

////////////////////////////////////////////////////////////////////////////////
/// Create dialog object with OK and Cancel buttons. This dialog
/// prompts for the arguments of "method".

void TGtk4ContextMenu::Dialog(TObject *object, TMethod *method)
{
   Dialog(object, (TFunction *)method);
}

////////////////////////////////////////////////////////////////////////////////
/// Create dialog object with OK and Cancel buttons. This dialog
/// prompts for the arguments of "function".
/// function may be a global function or a method

void TGtk4ContextMenu::Dialog(TObject * object, TFunction * func)
{
   auto widget = new Gtk4MethodDialog(300, 300);

   widget->methodDialog(fContextMenu, object, func);
}

////////////////////////////////////////////////////////////////////////////////
/// Handle remove of some ROOT objects

void TGtk4ContextMenu::RecursiveRemove(TObject *obj)
{
   if (obj == fContextMenu->GetSelectedCanvas())
      fContextMenu->SetCanvas(nullptr);
   if (obj == fContextMenu->GetSelectedPad())
      fContextMenu->SetPad(nullptr);
   if (obj == fContextMenu->GetSelectedObject()) {
      // if the object being deleted is the one selected,
      // ungrab the mouse pointer and terminate (close) the menu
      fContextMenu->SetObject(nullptr);
   }
}

////////////////////////////////////////////////////////////////////////////////
/// Register menu action in signal map

/*

QAction* TGtk4ContextMenu::addMenuAction(QMenu* menu, QSignalMapper *map, const QString& text, int id, void *arg)
{
   bool enabled = true;

   QAction* act = new QAction(text, menu);

   if (!enabled)
      if ((text.compare("DrawClone") == 0) || (text.compare("DrawClass") == 0) || (text.compare("Inspect") == 0) ||
          (text.compare("SetShowProjectionX") == 0) || (text.compare("SetShowProjectionY") == 0) ||
          (text.compare("DrawPanel") == 0) || (text.compare("FitPanel") == 0))
         act->setEnabled(false);

   QObject::connect(act, &QAction::triggered, [id, map]() {
      map->mappedInt(id);
   });

   menu->addAction(act);
   map->setMapping(act, id);

   fCustomArg[id] = arg;

   return act;
}



////////////////////////////////////////////////////////////////////////////////
/// Add color elements to attributes editor dialog

void TGtk4ContextMenu::AddColorElements(int colindx, QFormLayout *layout)
{
   TColor *rootColor = gROOT->GetColor(colindx);
   QColor initialColor = Qt::black;
   int initialAlpha255 = 255; // Default fully opaque

   if (rootColor) {
      initialColor = QColor(rootColor->GetRed() * 255, rootColor->GetGreen() * 255, rootColor->GetBlue() * 255);
      initialAlpha255 = static_cast<int>(rootColor->GetAlpha() * 255);
   }
   initialColor.setAlpha(initialAlpha255);

   fColorButton = new QPushButton();
   fColorButton->setFixedWidth(80);

   QSlider *alphaSlider = new QSlider(Qt::Horizontal);
   alphaSlider->setRange(0, 255);
   alphaSlider->setValue(initialAlpha255);

   // Visual preview of current color
   fSelectedColor = initialColor;
   UpdateColorElements();

   QObject::connect(fColorButton, &QPushButton::clicked, [&, this]() {
      QColor col = QColorDialog::getColor(fSelectedColor, nullptr, "Select Color");
      if (col.isValid()) {
         fSelectedColor.setRed(col.red());
         fSelectedColor.setGreen(col.green());
         fSelectedColor.setBlue(col.blue());
         UpdateColorElements();
      }
   });

   // --- Slider Shift Connection ---
   QObject::connect(alphaSlider, &QSlider::valueChanged, [&](int value) {
      fSelectedColor.setAlpha(value);
      UpdateColorElements();
   });

   layout->addRow("Color:", fColorButton);

   layout->addRow("Opacity:", alphaSlider);
}

////////////////////////////////////////////////////////////////////////////////
/// Update color button with currently selected color

void TGtk4ContextMenu::UpdateColorElements()
{
   QString qss = QString("background-color: rgba(%1, %2, %3, %4); border: 1px solid gray;")
                     .arg(fSelectedColor.red())
                     .arg(fSelectedColor.green())
                     .arg(fSelectedColor.blue())
                     .arg(fSelectedColor.alpha() / 255.0);
   fColorButton->setStyleSheet(qss);
}

////////////////////////////////////////////////////////////////////////////////
/// Start TAttLine editor

void TGtk4ContextMenu::SetLineAttributesDialog()
{
   auto attline = dynamic_cast<TAttLine *>(fContextMenu->GetSelectedObject());
   if (!attline)
      return;

   QDialog dialog;
   dialog.setWindowTitle("Edit Line Attributes");
   dialog.setModal(true);

   QVBoxLayout *mainLayout = new QVBoxLayout(&dialog);
   QFormLayout *formLayout = new QFormLayout();

   // --- Color Selector ---
   AddColorElements(attline->GetLineColor(), formLayout);

   // --- Line Style Selector ---
   // ROOT Styles: 1=Solid, 2=Dashed, 3=Dotted, 4=Dash-Dot
   QComboBox *styleCombo = new QComboBox();
   styleCombo->addItem("None (0)", 0);
   styleCombo->addItem("Solid (1)", 1);
   styleCombo->addItem("Dashed (2)", 2);
   styleCombo->addItem("Dotted (3)", 3);
   styleCombo->addItem("Dash-Dot (4)", 4);
   styleCombo->addItem("Dash-Dot (5)", 5);
   styleCombo->addItem("Dash-Dot-Dot-Dot (6)", 6);
   styleCombo->addItem("Dashed medium (7)", 7);
   styleCombo->addItem("Dash-Dot-Dot (8)", 8);
   styleCombo->addItem("Dashed long (9)", 9);
   styleCombo->addItem("Dash-Dot long (10)", 10);

   // Find and set current style
   int currentStyle = attline->GetLineStyle();
   int styleIdx = styleCombo->findData(currentStyle);
   if (styleIdx != -1)
      styleCombo->setCurrentIndex(styleIdx);
   else
      styleCombo->addItem(QString("Custom (%1)").arg(currentStyle), currentStyle);

   formLayout->addRow("Style:", styleCombo);

   // --- Line Width Selector ---
   QSpinBox *widthSpin = new QSpinBox();
   widthSpin->setRange(1, 20);
   widthSpin->setValue(attline->GetLineWidth());
   formLayout->addRow("Width:", widthSpin);

   mainLayout->addLayout(formLayout);

   // --- Dialog Buttons (OK / Cancel) ---
   QHBoxLayout *buttonLayout = new QHBoxLayout();
   QPushButton *okButton = new QPushButton("OK");
   QPushButton *cancelButton = new QPushButton("Cancel");
   buttonLayout->addStretch();
   buttonLayout->addWidget(okButton);
   buttonLayout->addWidget(cancelButton);
   mainLayout->addLayout(buttonLayout);

   QObject::connect(okButton, &QPushButton::clicked, &dialog, &QDialog::accept);
   QObject::connect(cancelButton, &QPushButton::clicked, &dialog, &QDialog::reject);

   // 2. Execute Dialog and apply properties if accepted
   if (dialog.exec() == QDialog::Accepted) {
      // Update ROOT Color Index
      Color_t newColorIdx = TColor::GetColor(fSelectedColor.red(),
                                             fSelectedColor.green(),
                                             fSelectedColor.blue(),
                                             fSelectedColor.alpha() / 255.0);
      attline->SetLineColor(newColorIdx);

      // Update Line Style
      Style_t newStyle = styleCombo->currentData().toInt();
      attline->SetLineStyle(newStyle);

      // Update Line Width
      Width_t newWidth = widthSpin->value();
      attline->SetLineWidth(newWidth);
   }
}

////////////////////////////////////////////////////////////////////////////////
/// Start TAttFill editor

void TGtk4ContextMenu::SetFillAttributesDialog()
{
   auto attfill = dynamic_cast<TAttFill *>(fContextMenu->GetSelectedObject());
   if (!attfill)
      return;

   QDialog dialog;
   dialog.setWindowTitle("Edit Fill Attributes");
   dialog.setModal(true);

   QVBoxLayout *mainLayout = new QVBoxLayout(&dialog);
   QFormLayout *formLayout = new QFormLayout();

   // --- Color Selector ---
   AddColorElements(attfill->GetFillColor(), formLayout);

   // --- Fill Style Selector ---
   // ROOT Styles: 1=Solid, 2=Dashed, 3=Dotted, 4=Dash-Dot
   QComboBox *styleCombo = new QComboBox();
   styleCombo->addItem("None (0)", 0);
   styleCombo->addItem("Solid (1001)", 1001);
   for (int s = 3001; s <= 3025; ++s)
      styleCombo->addItem(QString("Style %1").arg(s), s);
   for (int s = 3144; s <= 3944; s += 100)
      styleCombo->addItem(QString("Style %1").arg(s), s);
   for (int s = 3305; s <= 3395; s += 10)
      styleCombo->addItem(QString("Style %1").arg(s), s);
   for (int s = 3350; s <= 3359; s += 1)
      styleCombo->addItem(QString("Style %1").arg(s), s);
   for (int s = 3409; s <= 3490; s += 9)
      styleCombo->addItem(QString("Style %1").arg(s), s);
   for (int s = 3609; s <= 3690; s += 9)
      styleCombo->addItem(QString("Style %1").arg(s), s);

   // Find and set current style
   int currentStyle = attfill->GetFillStyle();
   int styleIdx = styleCombo->findData(currentStyle);
   if (styleIdx != -1)
      styleCombo->setCurrentIndex(styleIdx);
   else
      styleCombo->addItem(QString("Style %1").arg(currentStyle), currentStyle);

   formLayout->addRow("Style:", styleCombo);

   mainLayout->addLayout(formLayout);

   // --- Dialog Buttons (OK / Cancel) ---
   QHBoxLayout *buttonLayout = new QHBoxLayout();
   QPushButton *okButton = new QPushButton("OK");
   QPushButton *cancelButton = new QPushButton("Cancel");
   buttonLayout->addStretch();
   buttonLayout->addWidget(okButton);
   buttonLayout->addWidget(cancelButton);
   mainLayout->addLayout(buttonLayout);

   QObject::connect(okButton, &QPushButton::clicked, &dialog, &QDialog::accept);
   QObject::connect(cancelButton, &QPushButton::clicked, &dialog, &QDialog::reject);

   // 2. Execute Dialog and apply properties if accepted
   if (dialog.exec() == QDialog::Accepted) {
      // Update ROOT Color Index
      Color_t newColorIdx = TColor::GetColor(fSelectedColor.red(),
                                             fSelectedColor.green(),
                                             fSelectedColor.blue(),
                                             fSelectedColor.alpha() / 255.0);
      attfill->SetFillColor(newColorIdx);

      // Update fill Style
      Style_t newStyle = styleCombo->currentData().toInt();
      attfill->SetFillStyle(newStyle);
   }
}


class CustomDoubleSpinBox : public QDoubleSpinBox {
protected:
    QString textFromValue(double value) const override {
        if (value == 0) return "Default";
        return QDoubleSpinBox::textFromValue(value);
    }

    double valueFromText(const QString &text) const override {
        if (text == "Default") return 0.;
        return QDoubleSpinBox::valueFromText(text);
    }
};

class CustomSpinBox : public QSpinBox {
protected:
    QString textFromValue(int value) const override {
        if (value == 0) return "Default";
        return QSpinBox::textFromValue(value);
    }

    int valueFromText(const QString &text) const override {
        if (text == "Default") return 0;
        return QSpinBox::valueFromText(text);
    }
};


////////////////////////////////////////////////////////////////////////////////
/// Start TAttText editor

void TGtk4ContextMenu::SetTextAttributesDialog()
{
   auto atttext = dynamic_cast<TAttText *>(fContextMenu->GetSelectedObject());
   if (!atttext)
      return;

   QDialog dialog;
   dialog.setWindowTitle("Edit Text Attributes");
   dialog.setModal(true);

   QVBoxLayout *mainLayout = new QVBoxLayout(&dialog);
   QFormLayout *formLayout = new QFormLayout();

   // --- Color Selector ---
   AddColorElements(atttext->GetTextColor(), formLayout);

   QComboBox *fontCombo = new QComboBox();
   fontCombo->addItem("1. Times italic", 1);
   fontCombo->addItem("2. Times bold", 2);
   fontCombo->addItem("3. Times bold italic", 3);
   fontCombo->addItem("4. Helvetica", 4);
   fontCombo->addItem("5. Helvetica italic", 5);
   fontCombo->addItem("6. Helvetica bold", 6);
   fontCombo->addItem("7. Helvetica bold italic", 7);
   fontCombo->addItem("8. Courier", 8);
   fontCombo->addItem("9. Courier italic", 9);
   fontCombo->addItem("10. Courier bold", 10);
   fontCombo->addItem("11. Courier bold italic", 11);
   fontCombo->addItem("12. Symbol", 12);
   fontCombo->addItem("13. Times", 13);
   fontCombo->addItem("14. Wingdings", 14);
   fontCombo->addItem("15. Symbol italic", 15);

   // Find and set current style
   int currentPrec = atttext->GetTextFont() % 10;
   int currentFont = atttext->GetTextFont() / 10;
   int styleIdx = fontCombo->findData(currentFont);
   if (styleIdx >= 0)
      fontCombo->setCurrentIndex(styleIdx);
   else
      fontCombo->addItem(QString("Font %1").arg(currentFont), currentFont);

   formLayout->addRow("Font:", fontCombo);

   QDoubleSpinBox* floatSpinBox = nullptr;
   QSpinBox *intSpinBox = nullptr;

   if (currentPrec == 2) {
      floatSpinBox = new CustomDoubleSpinBox();
      floatSpinBox->setRange(0.0, 1.0);   // Set your minimum and maximum limits
      floatSpinBox->setSingleStep(0.01);   // Set step size to 00.1
      floatSpinBox->setDecimals(3);        // Force it to show exactly 3 decimal places
      floatSpinBox->setValue(atttext->GetTextSize());
      formLayout->addRow("Size:", floatSpinBox);
   } else {
      intSpinBox = new CustomSpinBox();
      intSpinBox->setRange(0, 128);
      intSpinBox->setValue(atttext->GetTextSize());
      formLayout->addRow("Size:", intSpinBox);
   }

   QComboBox *alignCombo = new QComboBox();
   alignCombo->addItem("11. Left Bottom", 11);
   alignCombo->addItem("12. Left Center", 12);
   alignCombo->addItem("13. Left Top", 13);
   alignCombo->addItem("21. Middle Bottom", 21);
   alignCombo->addItem("22. Middle Center", 22);
   alignCombo->addItem("23. Middle Top", 23);
   alignCombo->addItem("31. Right Bottom", 31);
   alignCombo->addItem("32. Right Center", 32);
   alignCombo->addItem("33. Right Top", 33);

   // Find and set current style
   int alignIdx = alignCombo->findData(atttext->GetTextAlign());
   if (alignIdx < 0)
      alignIdx = alignCombo->findData(11);
   alignCombo->setCurrentIndex(alignIdx);

   formLayout->addRow("Align:", alignCombo);

   mainLayout->addLayout(formLayout);

   // --- Dialog Buttons (OK / Cancel) ---
   QHBoxLayout *buttonLayout = new QHBoxLayout();
   QPushButton *okButton = new QPushButton("OK");
   QPushButton *cancelButton = new QPushButton("Cancel");
   buttonLayout->addStretch();
   buttonLayout->addWidget(okButton);
   buttonLayout->addWidget(cancelButton);
   mainLayout->addLayout(buttonLayout);

   QObject::connect(okButton, &QPushButton::clicked, &dialog, &QDialog::accept);
   QObject::connect(cancelButton, &QPushButton::clicked, &dialog, &QDialog::reject);

   // 2. Execute Dialog and apply properties if accepted
   if (dialog.exec() == QDialog::Accepted) {
      // Update ROOT Color Index
      Color_t newColorIdx = TColor::GetColor(fSelectedColor.red(),
                                             fSelectedColor.green(),
                                             fSelectedColor.blue(),
                                             fSelectedColor.alpha() / 255.0);
      atttext->SetTextColor(newColorIdx);

      // Update font Style
      Int_t newFont = fontCombo->currentData().toInt();
      atttext->SetTextFont(newFont * 10 + currentPrec);

      if (floatSpinBox)
         atttext->SetTextSize(floatSpinBox->value());
      else if (intSpinBox)
         atttext->SetTextSize(intSpinBox->value());

      atttext->SetTextAlign(alignCombo->currentData().toInt());
   }
}

////////////////////////////////////////////////////////////////////////////////
/// Start TAttMarker editor

void TGtk4ContextMenu::SetMarkerAttributesDialog()
{
   auto attmarker = dynamic_cast<TAttMarker *>(fContextMenu->GetSelectedObject());
   if (!attmarker)
      return;

   QDialog dialog;
   dialog.setWindowTitle("Edit Marker Attributes");
   dialog.setModal(true);

   QVBoxLayout *mainLayout = new QVBoxLayout(&dialog);
   QFormLayout *formLayout = new QFormLayout();

   // --- Color Selector ---
   AddColorElements(attmarker->GetMarkerColor(), formLayout);

   // --- Marker Style Selector ---
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

   formLayout->addRow("Style:", styleCombo);


   QDoubleSpinBox* floatSpinBox = new QDoubleSpinBox();

   // 2. Configure its ranges and step parameters
   floatSpinBox->setRange(0.0, 10.0);   // Set your minimum and maximum limits
   floatSpinBox->setSingleStep(0.1);    // Set step size to 0.1
   floatSpinBox->setDecimals(1);        // Force it to show exactly 1 decimal place (e.g., 1.5)
   floatSpinBox->setValue(attmarker->GetMarkerSize());
   formLayout->addRow("Size:", floatSpinBox);

   mainLayout->addLayout(formLayout);

   // --- Dialog Buttons (OK / Cancel) ---
   QHBoxLayout *buttonLayout = new QHBoxLayout();
   QPushButton *okButton = new QPushButton("OK");
   QPushButton *cancelButton = new QPushButton("Cancel");
   buttonLayout->addStretch();
   buttonLayout->addWidget(okButton);
   buttonLayout->addWidget(cancelButton);
   mainLayout->addLayout(buttonLayout);

   QObject::connect(okButton, &QPushButton::clicked, &dialog, &QDialog::accept);
   QObject::connect(cancelButton, &QPushButton::clicked, &dialog, &QDialog::reject);

   // 2. Execute Dialog and apply properties if accepted
   if (dialog.exec() == QDialog::Accepted) {
      // Update ROOT Color Index
      Color_t newColorIdx = TColor::GetColor(fSelectedColor.red(),
                                             fSelectedColor.green(),
                                             fSelectedColor.blue(),
                                             fSelectedColor.alpha() / 255.0);
      attmarker->SetMarkerColor(newColorIdx);

      // Update Style
      Style_t newStyle = styleCombo->currentData().toInt();
      attmarker->SetMarkerStyle(newStyle);

      float newsize = floatSpinBox->value();
      attmarker->SetMarkerSize(newsize);
   }

}

*/


////////////////////////////////////////////////////////////////////////////////
/// Execute specified menu item

void TGtk4ContextMenu::executeMenu(int id)
{
   if (id < 0)
      return;
   void *ud = fCustomArg[id];

   if (ud) {
      // retrieve the highlighted function
      TFunction *function = nullptr;
      if (id < kToggleStart) {
         TMethod *m = (TMethod *)ud;
         function = (TFunction *)m;
      } else if (id >= kToggleStart && id < kUserFunctionStart) {
         TToggle *t = (TToggle *)ud;
         TMethodCall *mc = (TMethodCall *)t->GetSetter();
         function = (TFunction *)mc->GetMethod();
      } else {
         TClassMenuItem *mi = (TClassMenuItem *)ud;
         function = gROOT->GetGlobalFunctionWithPrototype(mi->GetFunctionName());
      }
      if (function)
         fContextMenu->SetMethod(function);
   }

   if (id < kToggleStart) {
      auto m = (TMethod *) ud;

      if (!strcmp(m->GetName(), "SetLineAttributes")) {
         auto attline = dynamic_cast<TAttLine *>(fContextMenu->GetSelectedObject());
         if (attline) {
            auto widget = new Gtk4MethodDialog(300, 300);
            widget->attLineDialog(attline);
         }
      } else if (!strcmp(m->GetName(), "SetFillAttributes")) {
         auto attfill = dynamic_cast<TAttFill *>(fContextMenu->GetSelectedObject());
         if (attfill) {
            auto widget = new Gtk4MethodDialog(300, 300);
            widget->attFillDialog(attfill);
         }
      } else if (!strcmp(m->GetName(), "SetTextAttributes")) {
         auto atttext = dynamic_cast<TAttText *>(fContextMenu->GetSelectedObject());
         if (atttext) {
            auto widget = new Gtk4MethodDialog(300, 300);
            widget->attTextDialog(atttext);
         }
      } else if (!strcmp(m->GetName(), "SetMarkerAttributes")) {
         auto attmark = dynamic_cast<TAttMarker *>(fContextMenu->GetSelectedObject());
         if (attmark) {
            auto widget = new Gtk4MethodDialog(300, 300);
            widget->attMarkerDialog(attmark);
         }
      } else {
         fContextMenu->Action(m);
      }
   } else if (id >= kToggleStart && id < kToggleListStart) {
      TToggle *t = (TToggle *) ud;
      fContextMenu->Action(t);
   } else if (id >= kToggleListStart && id < kUserFunctionStart) {
      TToggle *t = (TToggle *) ud;
      if (t->GetState() == 0)
         t->SetState(1);
   } else {
      TClassMenuItem *mi = (TClassMenuItem*)ud;
      fContextMenu->Action(mi);
   }
}
