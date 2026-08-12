#ifndef MY_DRAWING_AREA_H
#define MY_DRAWING_AREA_H

#include <gtkmm/drawingarea.h>
#include <cairomm/context.h>

class MyDrawingArea : public Gtk::DrawingArea {
public:
    MyDrawingArea();
    virtual ~MyDrawingArea() = default;

protected:
    // The render function for gtkmm 4
    void on_draw(const Cairo::RefPtr<Cairo::Context>& cr, int width, int height);
};

#endif // MY_DRAWING_AREA_H
