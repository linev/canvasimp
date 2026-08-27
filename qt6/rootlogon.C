{
   gSystem->AddDynamicPath(".");

   gEnv->SetValue("Gui.Factory", "qt6");

   gEnv->SetValue("Root.PadEditor","qt6");

   gPluginMgr->AddHandler("TGuiFactory", "qt6", "ROOT::Experimental::TQt6GuiFactory",
      "libROOTQt6Canvas", "TQt6GuiFactory()");

   gPluginMgr->AddHandler("TVirtualPadEditor", "qt6", "ROOT::Experimental::TQt6GedEditor",
      "libROOTQt6Canvas", "TQt6GedEditor(TCanvas*)");
}
