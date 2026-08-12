// TVulkanCanvasImp.cxx

#include "TVulkanCanvasImp.h"
#include "VulkanRenderer.h"
#include "TVulkanPadPainter.h"

#include "TCanvas.h"
#include "TVirtualPad.h"
#include "TROOT.h"

#include <GLFW/glfw3.h>
#include <cmath>

TVulkanCanvasImp::TVulkanCanvasImp(TCanvas *canvas, const char *title, UInt_t width, UInt_t height)
   : TCanvasImp(canvas, title, width, height)
{
   fWinWidth = width;
   fWinHeight = height;
   glfwInit();
   glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); // Vulkan, not GL - GLFW just gives us the surface

   fWindow = glfwCreateWindow((int)width, (int)height, title, nullptr, nullptr);
   glfwSetWindowUserPointer(fWindow, this);

   glfwSetCursorPosCallback(fWindow, &TVulkanCanvasImp::CursorPosCallback);
   glfwSetMouseButtonCallback(fWindow, &TVulkanCanvasImp::MouseButtonCallback);
   glfwSetScrollCallback(fWindow, &TVulkanCanvasImp::ScrollCallback);
   glfwSetFramebufferSizeCallback(fWindow, &TVulkanCanvasImp::FramebufferSizeCallback);

   fRenderer = new VulkanRenderer(fWindow, width, height);
   fPainter = new TVulkanPadPainter(fRenderer);
}

TVulkanCanvasImp::~TVulkanCanvasImp()
{
   delete fPainter;
   delete fRenderer;
   if (fWindow) glfwDestroyWindow(fWindow);
   glfwTerminate();
}

UInt_t TVulkanCanvasImp::GetWindowGeometry(Int_t &x, Int_t &y, UInt_t &w, UInt_t &h)
{
   x = y = 0;
   w = fWinWidth;
   h = fWinHeight;
   return 0;

}

void TVulkanCanvasImp::GetCanvasGeometry([[maybe_unused]] Int_t wid, UInt_t &w, UInt_t &h)
{
   w = fWinWidth;
   h = fWinHeight;
}



Int_t TVulkanCanvasImp::InitWindow()
{
   return 0; // 0 == success, matching TCanvasImp convention
}

void TVulkanCanvasImp::Show()
{
   if (fWindow) glfwShowWindow(fWindow);
}

void TVulkanCanvasImp::Close()
{
   if (fWindow) glfwSetWindowShouldClose(fWindow, GLFW_TRUE);
}

void TVulkanCanvasImp::RaiseWindow()
{
   if (fWindow) glfwFocusWindow(fWindow);
}

void TVulkanCanvasImp::SetWindowTitle(const char *title)
{
   if (fWindow) glfwSetWindowTitle(fWindow, title);
}

void TVulkanCanvasImp::SetWindowPosition(Int_t x, Int_t y)
{
   if (fWindow) glfwSetWindowPos(fWindow, x, y);
}

void TVulkanCanvasImp::SetWindowSize(UInt_t w, UInt_t h)
{
   if (fWindow) glfwSetWindowSize(fWindow, (int)w, (int)h);
}

void TVulkanCanvasImp::ShowMenuBar(Bool_t show)
{
   // GLFW has no native menu bar widget (same limitation noted in the
   // raylibcanvas README: "no context (right mouse) menu"). This would be
   // implemented the same way the raygui examples earlier in this
   // conversation did - hand-drawn dropdown panels rendered as part of the
   // Vulkan frame, driven by fMenuBarVisible.
   fMenuBarVisible = show;
}

void TVulkanCanvasImp::ShowStatusBar(Bool_t show)
{
   fStatusBarVisible = show;
}

void TVulkanCanvasImp::SetStatusText(const char *text, Int_t /*partIdx*/)
{
   fStatusText = text;
   // Rendered as part of RunOnce()'s frame, the same role GuiStatusBar()
   // played in the raygui menu/status-bar example earlier in this chat -
   // here it'd be drawn as textured glyph quads once text rendering exists.
}

void TVulkanCanvasImp::ForceUpdate()
{
   if (Canvas())
      Canvas()->Modified();
}

Bool_t TVulkanCanvasImp::PerformUpdate(Bool_t)
{
   if (!Canvas() || !Canvas()->IsModified())
      return kFALSE;

   RunOnce();
   return kTRUE;
}

TVirtualPadPainter *TVulkanCanvasImp::CreatePadPainter()
{
   return new TVulkanPadPainter(fRenderer);
}



void TVulkanCanvasImp::RunOnce()
{
   printf("Calling RunOnce\n");

   if (!fWindow || glfwWindowShouldClose(fWindow)) return;

   glfwPollEvents();

   fRenderer->BeginFrame();
   // Actual pad painting happens here: ROOT calls back into the attached
   // TVirtualPadPainter (fPainter) while walking the pad's primitive list,
   // typically via TCanvas::Paint() / TPad::PaintModified() invoked from
   // wherever this RunOnce() is pumped from.
   if (Canvas())
      Canvas()->Paint();
   fRenderer->EndFrame();
   printf("Calling RunOnce done\n");
}

// --- Input callbacks -------------------------------------------------------
// Same pattern discussed earlier for plain raylib: poll position each
// frame via the callback, use GetTime()-equivalent (glfwGetTime()) + a
// distance threshold for double-click, since GLFW has no double-click event
// either.

void TVulkanCanvasImp::CursorPosCallback(GLFWwindow *win, double x, double y)
{
   auto *self = static_cast<TVulkanCanvasImp *>(glfwGetWindowUserPointer(win));
   if (self) self->HandleMove(x, y);
}

void TVulkanCanvasImp::MouseButtonCallback(GLFWwindow *win, int button, int action, int /*mods*/)
{
   auto *self = static_cast<TVulkanCanvasImp *>(glfwGetWindowUserPointer(win));
   if (self) self->HandleButton(button, action);
}

void TVulkanCanvasImp::ScrollCallback(GLFWwindow *win, double xoff, double yoff)
{
   auto *self = static_cast<TVulkanCanvasImp *>(glfwGetWindowUserPointer(win));
   if (self) self->HandleScroll(xoff, yoff);
}

void TVulkanCanvasImp::FramebufferSizeCallback(GLFWwindow *win, int width, int height)
{
   auto *self = static_cast<TVulkanCanvasImp *>(glfwGetWindowUserPointer(win));
   if (self && self->fRenderer) self->fRenderer->OnResize((uint32_t)width, (uint32_t)height);
}

void TVulkanCanvasImp::HandleMove(double x, double y)
{
   if (!Canvas()) return;
   // ROOT's event enum names vary slightly by version; kMouseMotion is the
   // typical one for plain movement without a button held.
   Canvas()->HandleInput(kMouseMotion, (Int_t)x, (Int_t)y);
}

void TVulkanCanvasImp::HandleButton(int button, int action)
{
   if (!Canvas() || button != GLFW_MOUSE_BUTTON_LEFT) return;

   double x, y;
   glfwGetCursorPos(fWindow, &x, &y);

   if (action == GLFW_PRESS) {
      double now = glfwGetTime();
      double dx = x - fLastClickX, dy = y - fLastClickY;
      bool isDoubleClick = (now - fLastClickTime) <= kDoubleClickTime &&
                            std::sqrt(dx * dx + dy * dy) <= kDoubleClickMaxDist;

      if (isDoubleClick) {
         Canvas()->HandleInput(kButton1Double, (Int_t)x, (Int_t)y);
         fLastClickTime = -1.0; // avoid chaining into a third click, same as the raylib example
      } else {
         Canvas()->HandleInput(kButton1Down, (Int_t)x, (Int_t)y);
         fLastClickTime = now;
         fLastClickX = x; fLastClickY = y;
      }
   } else if (action == GLFW_RELEASE) {
      Canvas()->HandleInput(kButton1Up, (Int_t)x, (Int_t)y);
   }
}

void TVulkanCanvasImp::HandleScroll(double /*xoff*/, double yoff)
{
   if (!Canvas()) return;
   double x, y;
   glfwGetCursorPos(fWindow, &x, &y);
   Canvas()->HandleInput(yoff > 0 ? kWheelUp : kWheelDown, (Int_t)x, (Int_t)y);
}
