// Author: Sergey Linev, GSI   12/08/2026

/*************************************************************************
 * Copyright (C) 1995-2026, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

#include "Gtk4DrawArea.h"

#include <cmath>

#include "TGtk4PadPainter.h"
#include "TCanvas.h"


Gtk4DrawArea::Gtk4DrawArea()
{
    // Bind the member function as the draw function
    set_draw_func(sigc::mem_fun(*this, &Gtk4DrawArea::on_draw));
}

void Gtk4DrawArea::on_draw(const Cairo::RefPtr<Cairo::Context>& cr, int width, int height)
{
    fContext = cr.get();

    // 1. Paint background dark gray
    cr->set_source_rgb(0.15, 0.15, 0.15);
    cr->paint();

    // 2. Draw a blue circle in the center that scales with the window
    double radius = std::min(width, height) / 4.0;
    cr->set_source_rgb(0.2, 0.5, 0.8);
    cr->arc(width / 2.0, height / 2.0, radius, 0.0, 2.0 * M_PI);
    cr->fill();

    // TODO: call canvas->paint instead

    fContext = nullptr;
}
