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

class TContextMenu;
class TObject;
class TFunction;

class Gtk4MethodDialog: public Gtk::Window {

   public:
      Gtk4MethodDialog(unsigned width = 400, unsigned height = 200);

      //void addArg(const char *argname, const char *value, const char *type);

      //QString getArg(int n);

      void methodDialog(TContextMenu *menu, TObject *object, TFunction* func);

      //void MenuCommandExecuted(TObject *obj, const char *method_name);

   protected:

      void on_color_changed();

      void addColorInput(Gtk::Box *box);

      Gtk::Box *m_main_box = nullptr;

      Gtk::ColorDialogButton* m_color_button = nullptr;

      // QVector<QLineEdit*> fArgs;
};

#endif
