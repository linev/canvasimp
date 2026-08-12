// Author: Sergey Linev, GSI   12/08/2026

/*************************************************************************
 * Copyright (C) 1995-2026, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

#ifndef ROOT_Gtk4DrawArea_H
#define ROOT_Gtk4DrawArea_H

#include <gtkmm/drawingarea.h>
#include <cairomm/context.h>

class TCanvas;

class Gtk4DrawArea : public Gtk::DrawingArea {
public:
    Gtk4DrawArea();
    virtual ~Gtk4DrawArea() = default;

    void SetCanvas(TCanvas *c) { fCanvas = c; }

    Cairo::Context *GetContext() const { return fContext; }

protected:
    // The render function for gtkmm 4
    void on_draw(const Cairo::RefPtr<Cairo::Context>& cr, int width, int height);

    TCanvas *fCanvas = nullptr;
    Cairo::Context *fContext = nullptr;
};

#endif
