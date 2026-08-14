// Author: Sergey Linev, GSI  14/08/2026

/*************************************************************************
 * Copyright (C) 1995-2026, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

#include "Gtk4MethodDialog.h"

#include "TString.h"
#include "TROOT.h"
#include "TVirtualPad.h"
#include "TMethod.h"
#include "TMethodArg.h"
#include "TMethodCall.h"
#include "TObjString.h"
#include "TContextMenu.h"

#include <iostream>

/** \class Gtk4MethodDialog
    \ingroup gtk4canvas

Specialized dialog to enter arguments for method exection
*/

////////////////////////////////////////////////////////////////////////////////
/// Create method dialog

Gtk4MethodDialog::Gtk4MethodDialog(unsigned width, unsigned height)
{

   set_title("method dialog");
   set_default_size(width, height);

   m_main_box = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL, 10);
   m_main_box->set_margin(20);
   set_child(*m_main_box);
}

////////////////////////////////////////////////////////////////////////////////
/// on color change

void Gtk4MethodDialog::addColorInput(Gtk::Box *box)
{
   auto color_dialog = Gtk::ColorDialog::create();
   color_dialog->set_title("Select a Custom Color");
   color_dialog->set_modal(true);

   auto layout_grid = Gtk::make_managed<Gtk::Grid>();
   layout_grid->set_column_spacing(15);
   layout_grid->set_hexpand(true);
   layout_grid->set_halign(Gtk::Align::FILL);
   box->append(*layout_grid);

   auto color_label = Gtk::make_managed<Gtk::Label>("Select Color:");
   color_label->set_halign(Gtk::Align::START);
   color_label->set_hexpand(true);              // Wants to expand
   color_label->set_halign(Gtk::Align::FILL);   // Stretch to fill its 50% allocation
   color_label->set_xalign(0.0);

   // Instantiate the specific ColorDialogButton using our dialog rules
   m_color_button = Gtk::make_managed<Gtk::ColorDialogButton>(color_dialog);
   m_color_button->set_halign(Gtk::Align::END);

   // Set an initial default color (e.g., solid Red)
   Gdk::RGBA initial_color;
   initial_color.set_rgba(1.0, 0.0, 0.0, 1.0);
   m_color_button->set_rgba(initial_color);
   m_color_button->set_hexpand(true);             // Wants to expand equally with the label
   m_color_button->set_halign(Gtk::Align::FILL);
   // 3. Connect a property notification listener to capture user selection changes
   m_color_button->property_rgba().signal_changed().connect(
      sigc::mem_fun(*this, &Gtk4MethodDialog::on_color_changed)
   );

   // Attach items to the Grid
   // grid->attach(widget, column, row, column_span, row_span)
   // We let the label take 3 columns (30%) and the button take 7 columns (70%)
   layout_grid->attach(*color_label,   0, 0, 3, 1);
   layout_grid->attach(*m_color_button, 3, 0, 7, 1);
}


////////////////////////////////////////////////////////////////////////////////
/// on color change

void Gtk4MethodDialog::on_color_changed()
{
   // 4. Retrieve the chosen RGBA color payload
   Gdk::RGBA selected_color = m_color_button->get_rgba();

   std::cout << "Color updated to -> Red: " << selected_color.get_red()
            << ", Green: " << selected_color.get_green()
            << ", Blue: " << selected_color.get_blue() << std::endl;
}

/*

////////////////////////////////////////////////////////////////////////////////
/// Add argument

void Gtk4MethodDialog::addArg(const char *argname, const char *value, const char *)
{
   QLabel* lbl = new QLabel(argname);
   argLayout->addWidget(lbl);

   QLineEdit* le = new QLineEdit();
   le->setGeometry(10,10, 130, 30);
   le->setFocus();
   le->setText(value);
   argLayout->addWidget(le);

   fArgs.push_back(le);
}

////////////////////////////////////////////////////////////////////////////////
/// Get argument value

QString Gtk4MethodDialog::getArg(int n)
{
   if ((n<0) || (n>=fArgs.size())) return QString("");
   return fArgs[n]->text();
}


////////////////////////////////////////////////////////////////////////////////
/// Run method dialog

void Gtk4MethodDialog::methodDialog(TContextMenu *menu, TObject *object, TFunction* func)
{
   if (!menu || !object || !func)
      return;

   setWindowTitle(menu->CreateDialogTitle(object, func));

   // iterate through all arguments and create appropriate input-data objects:
   // inputlines, option menus...
   TIter next(func->GetListOfMethodArgs());

   while (auto argument = (TMethodArg *) next()) {
      TString argTitle = menu->CreateArgumentTitle(argument);
      TString type = argument->GetTypeName();
      TDataType *datatype = gROOT->GetType(type);
      TString basictype;

      if (datatype) {
         basictype = datatype->GetTypeName();
      } else {
         if (type.CompareTo("enum") != 0)
            std::cout << "*** Warning in Dialog(): data type is not basic type, assuming (int)\n";
         basictype = "int";
      }

      if (TString(argument->GetTitle()).Index("*") != kNPOS) {
         basictype += "*";
         type = "char*";
      }

      TDataMember *m = argument->GetDataMember();
      if (m && m->GetterMethod()) {

         m->GetterMethod()->Init(object->IsA(), m->GetterMethod()->GetMethodName(), "");

         // Get the current value and form it as a text:

         TString val;

         if (basictype == "char*") {
            char *tdefval = nullptr;
            m->GetterMethod()->Execute(object, "", &tdefval);
            if (tdefval) val = tdefval;
         } else
         if ((basictype == "float") ||
             (basictype == "double")) {
            Double_t ddefval = 0.;
            m->GetterMethod()->Execute(object, "", ddefval);
            val = TString::Format("%g", ddefval);
         } else
         if ((basictype == "char") ||
             (basictype == "int")  ||
             (basictype == "bool")  ||
             (basictype == "long") ||
             (basictype == "short")) {
            Longptr_t ldefval = 0;
            m->GetterMethod()->Execute(object, "", ldefval);
            val = TString::Format("%ld", (long) ldefval);
         }

         // Find out whether we have options ...

         TList *opt;
         if ((opt = m->GetOptions()) != nullptr) {
            // should stop dialog
            // workaround JAM: do not stop dialog, use textfield (for time display toggle)
            addArg(argTitle.Data(), val.Data(), type.Data());
            //return;
         } else {
            // we haven't got options - textfield ...
            addArg(argTitle.Data(), val.Data(), type.Data());
         }
      } else {    // if m not found ...
         TString argDflt;
         if (argument->GetDefault())
            argDflt = argument->GetDefault();

         if ((argDflt.Length() > 1) &&
             (argDflt[0]=='\"') && (argDflt[argDflt.Length()-1]=='\"')) {
            // cut "" from the string argument
            argDflt.Remove(0,1);
            argDflt.Remove(argDflt.Length()-1,1);
         }

         addArg(argTitle.Data(), argDflt.Data(), type.Data());
      }
   }

   if (exec() != QDialog::Accepted)
      return;

   TObjArray tobjlist(func->GetListOfMethodArgs()->LastIndex() + 1);
   for (int n = 0; n <= func->GetListOfMethodArgs()->LastIndex(); n++) {
      QString s = getArg(n);
      tobjlist.AddLast(new TObjString(s.toLatin1().constData()));
   }

   menu->Execute(object, func, &tobjlist);
}

*/


void Gtk4MethodDialog::methodDialog(TContextMenu *menu, TObject *object, TFunction* func)
{
   if (!menu || !object || !func)
      return;

   addColorInput(m_main_box);

   present();
}