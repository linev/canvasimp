// Author: Sergey Linev, GSI  06/08/2026

/*************************************************************************
 * Copyright (C) 1995-2026, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

#include "TRaylibCanvas.h"
#include "TRaylibPadPainter.h"

#include "TSystem.h"
#include "TStyle.h"
#include "TCanvas.h"
#include "TROOT.h"
#include "TClass.h"
#include "TError.h"
#include "TTimer.h"
#include "TApplication.h"

#include <raylib.h>
#include <raymath.h>
#include <iostream>
#include <mutex>

#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

using namespace ROOT::Experimental;

// ─── Shared window state ──────────────────────────────────────────────


static bool sRaylibInitialized = false;
RenderTexture2D persistentCanvas;
Bool_t hasPersistentCanvas = false;

const int menuBarHeight = 28;
const int statusBarHeight = 24;

class TRaylibEventsTimer : public TTimer {

public:
   TRaylibEventsTimer(Long_t milliSec, Bool_t mode) : TTimer(milliSec, mode) {}

   /// used to send control messages to clients
   void Timeout() override
   {
      if (!::IsWindowReady())
         return;

      // Process input events
      ::PollInputEvents();

      TCanvas *canv = gPad ? gPad->GetCanvas() : nullptr;
      TRaylibCanvas *imp = canv ? dynamic_cast<TRaylibCanvas *>(canv->GetCanvasImp()) : nullptr;

      if (imp)
         imp->RunRaylib();
   }
};

static TRaylibEventsTimer *sTimer = nullptr;

// ─── Initialize raylib window ─────────────────────────────────────────


void TRaylibCanvas::EnsureRaylibInitialized(int width, int height)
{
   // std::lock_guard<std::mutex> lock(sRaylibInitMutex);
   if (!sRaylibInitialized) {
      SetTraceLogLevel(LOG_WARNING);
      SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_MSAA_4X_HINT);
      ::InitWindow(width > 0 ? width : 1200, height > 0 ? height : 800, "ROOT Canvas");
      SetTargetFPS(30);
      // SetRenderHint(RENDER_HINT_ANTIALIASED_LINES, true);
      sRaylibInitialized = true;

      // enable timer for the raylib events processing
      sTimer = new TRaylibEventsTimer(10, kTRUE);
      sTimer->TurnOn();
   }
}


// ─── Constructor / Destructor ──────────────────────────────────────────

TRaylibCanvas::TRaylibCanvas(TCanvas *c, const char *name, Int_t x, Int_t y,
                             UInt_t width, UInt_t height)
   : TCanvasImp(c, name, x, y, width, height),
     fWindowWidth(width), fWindowHeight(height),
     fPosX(x), fPosY(y)
{
}

TRaylibCanvas::~TRaylibCanvas()
{
}

// ─── Window Lifecycle ─────────────────────────────────────────────────

Int_t TRaylibCanvas::InitWindow()
{
   // raylib uses shared window — nothing to initialize per-canvas
   return 0;
}

void TRaylibCanvas::Close()
{
    // not implemented yet

}

void TRaylibCanvas::Show()
{
   if (!IsWindowReady() && Canvas())
      ::InitWindow(Canvas()->GetWw(), Canvas()->GetWh(), Canvas()->GetTitle());
}

// ─── Geometry ─────────────────────────────────────────────────────────

UInt_t TRaylibCanvas::GetWindowGeometry(Int_t &x, Int_t &y, UInt_t &w, UInt_t &h)
{
   x = fPosX;
   y = fPosY;
   w = (UInt_t)fWindowWidth;
   h = (UInt_t)fWindowHeight;
   return 0;
}

void TRaylibCanvas::GetCanvasGeometry(Int_t /*wid*/, UInt_t &w, UInt_t &h)
{
   w = (UInt_t)fWindowWidth;
   h = (UInt_t)fWindowHeight;
   if (fMenuBar)
      h -= menuBarHeight;
   if (fStatusBar)
      h -= statusBarHeight;
}

void TRaylibCanvas::ResizeCanvasWindow(Int_t /*wid*/)
{
   // No-op in raylib immediate mode
}

void TRaylibCanvas::UpdateDisplay(Int_t /*mode*/, Bool_t /*sleep*/)
{
}

// ─── Force Update ─────────────────────────────────────────────────────

void TRaylibCanvas::ForceUpdate()
{
   if (Canvas())
      Canvas()->Modified();
}


// ─── Position & Size ──────────────────────────────────────────────────

void TRaylibCanvas::SetWindowPosition(Int_t x, Int_t y)
{
   fPosX = x;
   fPosY = y;
   if (sRaylibInitialized && IsWindowReady()) {
      ::SetWindowPosition(x, y);
   }
}

void TRaylibCanvas::SetWindowSize(UInt_t w, UInt_t h)
{
   fWindowWidth = (int)w;
   fWindowHeight = (int)h;
   if (sRaylibInitialized && IsWindowReady()) {
      ::SetWindowSize((int)fWindowWidth, (int)fWindowHeight);
      fWindowWidth = GetScreenWidth();
      fWindowHeight = GetScreenHeight();
      fResized = kTRUE;
   }
}

void TRaylibCanvas::SetWindowTitle(const char *newTitle)
{
   fWindowTitle = newTitle ? newTitle : "";
   if (sRaylibInitialized && IsWindowReady()) {
      ::SetWindowTitle(newTitle ? newTitle : "");
   }
}

/////////////////////////////////////////////////////////////////////////////////
/// Method used to set fixed canvas size with scrolliing, not supported with raylib

void TRaylibCanvas::SetCanvasSize(UInt_t cw, UInt_t ch)
{
   // fWindowWidth = (int)cw;
   // fWindowHeight = (int)ch;
}

void TRaylibCanvas::Iconify()
{
   if (sRaylibInitialized && IsWindowReady()) {
      ::MinimizeWindow();
   }
}

void TRaylibCanvas::RaiseWindow()
{
   if (sRaylibInitialized && IsWindowReady()) {
      if (::IsWindowMinimized())
         ::RestoreWindow();
   }
}

// ─── Perform Update (called from TCanvas::Update) ─────────────────────


Bool_t TRaylibCanvas::PerformUpdate(Bool_t /*async*/)
{
   if (!Canvas() || !Canvas()->IsModified() || !IsWindowReady() || !hasPersistentCanvas)
      return kFALSE;

   // One can make painting directly

   BeginTextureMode(persistentCanvas);
   ClearBackground(RAYWHITE);

   // actually now perform paint
   Canvas()->Paint();

   EndTextureMode();

   // empty for now
   return kTRUE;
}

// ─── Create Pad Painter ───────────────────────────────────────────────

TVirtualPadPainter *TRaylibCanvas::CreatePadPainter()
{
   return new TRaylibPadPainter();
}

///////////////////////////////////////////////////////
/// Central method to run raylib/raygui functionality

void TRaylibCanvas::RunRaylib()
{
   if (::WindowShouldClose()) {
      ::CloseWindow();
      return;
   }

   Vector2 winPos = GetWindowPosition();
   fPosX = (Int_t) winPos.x;
   fPosY = (Int_t) winPos.y;

   if (::IsWindowResized() || fResized || !hasPersistentCanvas) {
      fWindowWidth = GetScreenWidth();
      fWindowHeight = GetScreenHeight();
      Canvas()->Resize();
      if (hasPersistentCanvas)
         UnloadRenderTexture(persistentCanvas);
      int canvh = fWindowHeight;
      if (fMenuBar) canvh -= menuBarHeight;
      if (fStatusBar) canvh -= statusBarHeight;
      persistentCanvas = LoadRenderTexture(fWindowWidth, canvh);
      hasPersistentCanvas = kTRUE;

      Canvas()->ModifiedUpdate();
      fResized = kFALSE;
   }

   Bool_t isInside = IsCursorOnScreen();
   Vector2 mousePos = GetMousePosition();
   if (fMenuBar)
      mousePos.y -= menuBarHeight;
   if (isInside && ((mousePos.y < 0) || (mousePos.y >= persistentCanvas.texture.height)))
      isInside = kFALSE;

   if (!isInside)
      SetMouseCursor(MOUSE_CURSOR_DEFAULT);

   if (fCursowWasInside && !isInside)
      Canvas()->HandleInput(kMouseLeave, 0, 0);
   else if (!fCursowWasInside && isInside)
      Canvas()->HandleInput(kMouseEnter, 0, 0);

   fCursowWasInside = isInside;

   if (isInside) {
      if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
         Canvas()->HandleInput(kButton1Down, mousePos.x, mousePos.y);

      if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT))
         Canvas()->HandleInput(kButton1Up, mousePos.x, mousePos.y);

      if (IsMouseButtonPressed(MOUSE_BUTTON_MIDDLE))
         Canvas()->HandleInput(kButton2Down, mousePos.x, mousePos.y);

      if (IsMouseButtonReleased(MOUSE_BUTTON_MIDDLE))
         Canvas()->HandleInput(kButton2Up, mousePos.x, mousePos.y);

      if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT))
         Canvas()->HandleInput(kButton3Down, mousePos.x, mousePos.y);

      if (IsMouseButtonReleased(MOUSE_BUTTON_RIGHT))
         Canvas()->HandleInput(kButton3Up, mousePos.x, mousePos.y);
   }

   float wheel = GetMouseWheelMove();
   if ((wheel != 0) && isInside)
      fCanvas->HandleInput(wheel > 0 ? kWheelUp : kWheelDown, mousePos.x, mousePos.y);

   Vector2 delta  = GetMouseDelta();

   if (isInside && (delta.x != 0.0f || delta.y != 0.0f)) {
      if (IsMouseButtonDown(MOUSE_BUTTON_LEFT))
         Canvas()->HandleInput(kButton1Motion, mousePos.x, mousePos.y);
      else
         Canvas()->HandleInput(kMouseMotion, mousePos.x, mousePos.y);

      TObject *selected = fCanvas->GetSelected();
      Int_t px = fCanvas->GetEventX();
      Int_t py = fCanvas->GetEventY();
      fStatusMessage = "";
      if (selected) {
         fStatusMessage = selected->GetName();
         fStatusMessage += "  ";
         fStatusMessage += selected->GetObjectInfo(px, py);
      } else {
         fStatusMessage = "No selected object x = ";
         fStatusMessage += px;
         fStatusMessage += "  y = ";
         fStatusMessage += py;
      }
   }



   ::BeginDrawing();
   ::ClearBackground(RAYWHITE);

   Rectangle sourceRect = { 0, 0, (float)persistentCanvas.texture.width, -(float)persistentCanvas.texture.height };
   Vector2 targetPos = { 0, fMenuBar ? menuBarHeight : 0 }; // Place below our top menu layout border line
   DrawTextureRec(persistentCanvas.texture, sourceRect, targetPos, WHITE);

   // Draw top layout border frame
   // GuiPanel((Rectangle){ 0, 0, (float)fWindowWidth, menuBarHeight }, "");

   // Draw bottom status layout bar
   // GuiStatusBar is a native control built for standard message layouts
   if (fStatusBar)
      GuiStatusBar((Rectangle){ 0, (float)fWindowHeight - statusBarHeight, (float)fWindowWidth, statusBarHeight }, fStatusMessage.Data());

   if (!fMenuBar) {
      ::EndDrawing();
      return;
   }

   // GuiDropdownBox tracks focus selections and updates internal states
   if (GuiDropdownBox( (Rectangle){ 10, 4, 80, 24 }, "File;New canvas;Open;Save;Save as ...;Close canvas;Print;Quit ROOT", &fFileMenuSelection, fFileDropdownOpen)) {
      fFileDropdownOpen = !fFileDropdownOpen; // Hide dropdown overlay on action complete
      switch (fFileMenuSelection) {
         case 0: fStatusMessage = "Action: Select file"; break;
         case 1: fStatusMessage = "Action: New canvas"; break;
         case 2: fStatusMessage = "Action: Open ROOT file"; break;
         case 3: fStatusMessage = "Action: Save canvas"; break;
         case 4: fStatusMessage = "Action: Save as canvas"; break;
         case 5: fStatusMessage = "Action: Close canvas"; break;
         case 6: fStatusMessage = "Action: Print canvas"; break;
         case 7:
            fStatusMessage = "Action: Quit ROOT";
            gApplication->Terminate(0);
            break;
      }
      fFileMenuSelection = 0;
   }

   if (GuiDropdownBox( (Rectangle){ 100, 4, 80, 24 }, "Edit;Clear pad;Clear canvas", &fEditMenuSelection, fEditDropdownOpen)) {
      fEditDropdownOpen = !fEditDropdownOpen;
      switch (fEditMenuSelection) {
         case 0: break;
         case 1:
            gPad->Clear();
            gPad->ModifiedUpdate();
            break;
         case 2:
            Canvas()->Clear();
            Canvas()->ModifiedUpdate();
            break;
      }
      fEditMenuSelection = 0;
   }

   if (GuiDropdownBox( (Rectangle){ 190, 4, 80, 24 }, "View;Editor;Toolbar;Statusbar;Tooltip", &fViewMenuSelection, fViewDropdownOpen)) {
      fViewDropdownOpen = !fViewDropdownOpen;
      switch (fViewMenuSelection) {
         case 0:
            break;
         case 1:
            break;
         case 2:
            break;
         case 3:
            ShowStatusBar(!fStatusBar);
            break;
         case 4:
            break;
      }
      fViewMenuSelection = 0;
   }

   if (GuiDropdownBox( (Rectangle){ fWindowWidth - 70, 4, 60, 24 }, "Help;Canvas;Menu;Editor", &fHelpMenuSelection, fHelpDropdownOpen)) {
      fHelpDropdownOpen = !fHelpDropdownOpen;
      switch (fHelpMenuSelection) {
         case 0:
            break;
         case 1:
            break;
         case 2:
            break;
         case 3:
            break;
         case 4:
            break;
      }
      fHelpMenuSelection = 0;
   }



   ::EndDrawing();
}


// ─── Static Factory: NewCanvas (plugin entry point) ───────────────────

TCanvasImp *TRaylibCanvas::NewCanvas(TCanvas *c, const char *name, Int_t x, Int_t y,
                                     UInt_t width, UInt_t height)
{
   // Ensure raylib is initialized
   EnsureRaylibInitialized(width, height);

   auto *imp = new TRaylibCanvas(c, name, x, y, width, height);

   if (x >= 0 && y >= 0)
      imp->SetWindowPosition(x, y);
   imp->SetWindowSize(width, height);

   TString title = c->GetTitle();
   imp->SetWindowTitle(title.Data());

   // Set internal dimensions
   c->Resize();

   return imp;
}

