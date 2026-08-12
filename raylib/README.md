# raylib-based canvas for ROOT

## That is it

This is experimental implementation of [`TCanvasImp`](https://root.cern.ch/doc/master/classTCanvasImp.html) class with use of [raylib](https://www.raylib.com/). Just prove of principles that painting and interactivity handling can be performed in absolutely
new environment.

## How to use

1. Checkout repository

2. Build with:

     cmake path/to/checkout
     make --build .

3. Run ROOT from build directory - one uses `rootlogon.C` script to preconfigure raylib plugin

     root path/to/hsimple.C


## That is missing

   * only works with single canvas
      - one can implement dynamic switch between different canvases

   * no context (right mouse) menu
      - one can add support of dynamic GuiDropdownBox when context menu activated

   * no support of ROOT fonts
      - especially important for symbols.ttf for greek letters
      - raylib has tools for working with external fonts, but code need debugging

   * no support of toolbar with buttons to create new objects
      - probably most easy part to implement

   * no support for graphics attribute editors
      - typically activated via context menu, but also via methods calls
      - need extension in ROOT API
      - same problem with TQt6Canvas




If there is interest to continue this project - contact authors.
