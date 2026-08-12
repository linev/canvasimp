#include "exampledrawarea.h"
#include <cmath>

MyDrawingArea::MyDrawingArea() {
    // Bind the member function as the draw function
    set_draw_func(sigc::mem_fun(*this, &MyDrawingArea::on_draw));
}

void MyDrawingArea::on_draw(const Cairo::RefPtr<Cairo::Context>& cr, int width, int height) {
    // 1. Paint background dark gray
    cr->set_source_rgb(0.15, 0.15, 0.15);
    cr->paint();

    // 2. Draw a blue circle in the center that scales with the window
    double radius = std::min(width, height) / 4.0;
    cr->set_source_rgb(0.2, 0.5, 0.8);
    cr->arc(width / 2.0, height / 2.0, radius, 0.0, 2.0 * M_PI);
    cr->fill();
}
