{
   gSystem->AddDynamicPath(".");

   gEnv->SetValue("Gui.Factory", "qt6");

   gPluginMgr->AddHandler("TGuiFactory", "qt6", "ROOT::Experimental::TQt6GuiFactory",
      "libROOTQt6Canvas", "TQt6GuiFactory()");
}
