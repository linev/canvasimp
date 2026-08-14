// Author: Sergey Linev, GSI  14/08/2026

/*************************************************************************
 * Copyright (C) 1995-2026, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/


#ifndef ROOT_Gtk4MethodDialog
#define ROOT_Gtk4MethodDialog

#include <gtkmm.h>
#include <vector>

class TContextMenu;
class TObject;
class TFunction;

class Gtk4MethodDialog: public Gtk::Window {

   public:
      Gtk4MethodDialog(unsigned width = 400, unsigned height = 200);

      //QString getArg(int n);

      void methodDialog(TContextMenu *menu, TObject *object, TFunction* func);

      //void MenuCommandExecuted(TObject *obj, const char *method_name);

   protected:

      void on_color_changed();
      void on_ok_button_clicked();
      void on_cancel_button_clicked();

      void addColorInput(Gtk::Box *box);

      void addArg(const char *argname, const char *value, const char *type);

      void addOkCancelButtons();

      Gtk::Box *m_main_box = nullptr;

      Gtk::ColorDialogButton* m_color_button = nullptr;

      std::vector<Gtk::Entry *> fArgs;
      TContextMenu *fMenu = nullptr;
      TObject *fObject = nullptr;
      TFunction *fFunc = nullptr;
};

#endif
