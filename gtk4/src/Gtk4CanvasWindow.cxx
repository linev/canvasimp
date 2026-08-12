// Author: Sergey Linev, GSI   12/08/2026

/*************************************************************************
 * Copyright (C) 1995-2026, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

/* gtkmm example Copyright (C) 2002 gtkmm development team
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, see <https://www.gnu.org/licenses/>.
 */

#include "Gtk4CanvasWindow.h"

#include <gtkmm.h>
#include <gtkmm/statusbar.h>
#include <iostream>

Gtk4CanvasWindow::Gtk4CanvasWindow(unsigned width, unsigned height)
: m_Box(Gtk::Orientation::VERTICAL)
{
  set_title("canvas widget");
  set_default_size(width, height);

  set_child(m_Box); //We can put a MenuBar at the top of the box and other stuff below it.

  //Define the actions:
  m_refActionGroup = Gio::SimpleActionGroup::create();

  // There are several ways of calling a function that takes a sigc::slot.
  // If the slot function is very short, it might be easy to skip the on_xxx()
  // method and put its contents directly in a lambda expression.
  m_refActionGroup->add_action("new",
    [] { std::cout << "A File|New menu item was selected.\n"; /* on_action_file_new() */});
  // With sigc::mem_fun() or (for non-member functions and static member functions)
  // sigc::ptr_fun(). The only way before C++11 introduced lambda expressions.
  m_refActionGroup->add_action("open",
    sigc::mem_fun(*this, &Gtk4CanvasWindow::on_action_others) );

  // With a lambda expression. Does not disconnect automatically when Gtk4CanvasWindow
  // is deleted, like sigc::mem_fun() does.
  m_refActionRain = m_refActionGroup->add_action_bool("rain",
    [this] { on_action_toggle(); }, false);

  m_refActionGroup->add_action("quit",
    sigc::mem_fun(*this, &Gtk4CanvasWindow::on_action_file_quit) );

  // With a lambda expression and sigc::track_obj() or sigc::track_object().
  // Disconnects automatically like sigc::mem_fun().
#if SIGCXX_MINOR_VERSION >= 4
  m_refActionGroup->add_action("cut",
    sigc::track_object([this] { on_action_others(); }, *this));
#else
  m_refActionGroup->add_action("cut",
    sigc::track_obj([this] { on_action_others(); }, *this));
#endif
  m_refActionGroup->add_action("copy",
    sigc::mem_fun(*this, &Gtk4CanvasWindow::on_action_others) );
  m_refActionGroup->add_action("paste",
    sigc::mem_fun(*this, &Gtk4CanvasWindow::on_action_others) );

  insert_action_group("example", m_refActionGroup);

  //Define how the actions are presented in the menus and toolbars:
  m_refBuilder = Gtk::Builder::create();

  //Layout the actions in a menubar and toolbar:
  const Glib::ustring ui_info =
    "<interface>"
    "  <menu id='menubar'>"
    "    <submenu>"
    "      <attribute name='label' translatable='yes'>_File</attribute>"
    "      <section>"
    "        <item>"
    "          <attribute name='label' translatable='yes'>_New</attribute>"
    "          <attribute name='action'>example.new</attribute>"
    "        </item>"
    "        <item>"
    "          <attribute name='label' translatable='yes'>_Open</attribute>"
    "          <attribute name='action'>example.open</attribute>"
    "        </item>"
    "      </section>"
    "      <section>"
    "        <item>"
    "          <attribute name='label' translatable='yes'>Rain</attribute>"
    "          <attribute name='action'>example.rain</attribute>"
    "        </item>"
    "      </section>"
    "      <section>"
    "        <item>"
    "          <attribute name='label' translatable='yes'>_Quit</attribute>"
    "          <attribute name='action'>example.quit</attribute>"
    "        </item>"
    "      </section>"
    "    </submenu>"
    "    <submenu>"
    "      <attribute name='label' translatable='yes'>_Edit</attribute>"
    "      <item>"
    "        <attribute name='label' translatable='yes'>_Cut</attribute>"
    "        <attribute name='action'>example.cut</attribute>"
    "      </item>"
    "      <item>"
    "        <attribute name='label' translatable='yes'>_Copy</attribute>"
    "        <attribute name='action'>example.copy</attribute>"
    "      </item>"
    "      <item>"
    "        <attribute name='label' translatable='yes'>_Paste</attribute>"
    "        <attribute name='action'>example.paste</attribute>"
    "      </item>"
    "    </submenu>"
    "  </menu>"
    "</interface>";

  try
  {
    m_refBuilder->add_from_string(ui_info);
  }
  catch(const Glib::Error& ex)
  {
    std::cerr << "Building menus and toolbar failed: " <<  ex.what();
  }


  //Get the menubar:
  auto gmenu = m_refBuilder->get_object<Gio::Menu>("menubar");
  if (!gmenu)
    g_warning("GMenu not found");
  else
  {
    auto pMenuBar = Gtk::make_managed<Gtk::PopoverMenuBar>(gmenu);

    //Add the PopoverMenuBar to the window:
    m_Box.append(*pMenuBar);
  }


  fDrawArea = Gtk::make_managed<Gtk4DrawArea>();
  fDrawArea->set_hexpand(true);
  fDrawArea->set_vexpand(true);
  m_Box.append(*fDrawArea);



  auto p_statusbar = Gtk::make_managed<Gtk::Statusbar>();
  unsigned int context_id = p_statusbar->get_context_id("main_status");
  p_statusbar->push("Ready to draw", context_id);
  m_Box.append(*p_statusbar);
}

Gtk4CanvasWindow::~Gtk4CanvasWindow()
{
}

void Gtk4CanvasWindow::on_action_file_quit()
{
  close();
}

//void Gtk4CanvasWindow::on_action_file_new()
//{
//   std::cout << "A File|New menu item was selected." << std::endl;
//}

void Gtk4CanvasWindow::on_action_others()
{
  std::cout << "A menu item was selected." << std::endl;
}

void Gtk4CanvasWindow::on_action_toggle()
{
  std::cout << "The toggle menu item was selected." << std::endl;

  bool active = false;
  m_refActionRain->get_state(active);

  //The toggle action's state does not change automatically:
  active = !active;
  m_refActionRain->change_state(active);

  Glib::ustring message;
  if(active)
    message = "Toggle is active.";
  else
    message = "Toggle is not active";

  std::cout << message << std::endl;
}
