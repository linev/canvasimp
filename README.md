# raylib-based canvas for ROOT

This is experimental implementation of `TCanvasImp` class with use of raylib

Missing features:

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

Just prove of principles that painting and interactivity handling can be performed in absolutely
new environment.

If there is interest to continue this project - contact authors.
