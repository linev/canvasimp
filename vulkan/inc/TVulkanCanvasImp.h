// TVulkanCanvasImp.h
//
// Equivalent of TRaylibCanvasImp. Where that class drove raylib's
// InitWindow()/WindowShouldClose() loop directly, this one drives a GLFW
// window (GLFW is the natural companion to Vulkan for surface + input,
// the way raylib bundles both into one library) and owns the
// VulkanRenderer + TVulkanPadPainter.
//
// NOTE: exact TCanvasImp virtual signatures vary by ROOT version - check
// $ROOTSYS/include/TCanvasImp.h and adjust. The overrides below cover what
// TRaylibCanvasImp almost certainly needs to implement: window lifecycle,
// status bar text, and forcing a repaint.

#pragma once

#include "TCanvasImp.h"

#include "TString.h"

struct GLFWwindow;
class VulkanRenderer;
class TVulkanPadPainter;

class TVulkanCanvasImp : public TCanvasImp {
protected:

   Bool_t PerformUpdate(Bool_t async) override;
   TVirtualPadPainter *CreatePadPainter() override;

   UInt_t fWinWidth = 0;
   UInt_t fWinHeight = 0;

public:
   TVulkanCanvasImp(TCanvas *canvas, const char *title, UInt_t width, UInt_t height);
   ~TVulkanCanvasImp() override;

   // Geometry
   UInt_t GetWindowGeometry(Int_t &x, Int_t &y, UInt_t &w, UInt_t &h) override;
   void GetCanvasGeometry(Int_t wid, UInt_t &w, UInt_t &h) override;


   // --- TCanvasImp interface ---------------------------------------------
   void   Close() override;
   void   ForceUpdate() override;      // called by ROOT when the pad needs a repaint
   void   Iconify() override {}
   Bool_t IsWeb() const override { return kFALSE; }
   void   RaiseWindow() override;
   void   SetStatusText(const char *text, Int_t partIdx) override;
   void   SetWindowPosition(Int_t x, Int_t y) override;
   void   SetWindowSize(UInt_t w, UInt_t h) override;
   void   SetWindowTitle(const char *title) override;
   void   ShowMenuBar(Bool_t show = kTRUE) override;
   void   ShowStatusBar(Bool_t show = kTRUE) override;
   void   Show() override;
   Int_t  InitWindow() override;

   // Drives one iteration of the GLFW event loop + repaint. In the raylib
   // version this is effectively the body of the WindowShouldClose() loop;
   // here it needs to be pumped from ROOT's own event loop instead of
   // owning a blocking while() loop, since Vulkan/GLFW must coexist with
   // gSystem->ProcessEvents(). Typically wired up via a TTimer that calls
   // this every ~16ms, or a gSystem->AddFileHandler() on GLFW's underlying
   // fd where the platform supports it.
   void   RunOnce();

private:
   static void CursorPosCallback(GLFWwindow *win, double x, double y);
   static void MouseButtonCallback(GLFWwindow *win, int button, int action, int mods);
   static void ScrollCallback(GLFWwindow *win, double xoff, double yoff);
   static void FramebufferSizeCallback(GLFWwindow *win, int width, int height);

   void HandleMove(double x, double y);
   void HandleButton(int button, int action);
   void HandleScroll(double xoff, double yoff);

   GLFWwindow         *fWindow = nullptr;
   VulkanRenderer      *fRenderer = nullptr;
   TVulkanPadPainter   *fPainter = nullptr;

   bool   fMenuBarVisible = kTRUE;
   bool   fStatusBarVisible = kTRUE;
   TString fStatusText;

   // Double-click detection state - same pattern as the plain raylib mouse
   // example: compare time+distance between successive presses.
   double fLastClickTime = -1.0;
   double fLastClickX = 0.0, fLastClickY = 0.0;
   static constexpr double kDoubleClickTime = 0.30;
   static constexpr double kDoubleClickMaxDist = 6.0;
};
