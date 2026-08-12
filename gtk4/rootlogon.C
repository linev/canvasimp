{
   gSystem->AddDynamicPath(".");

   gEnv->SetValue("Gui.Factory", "gtk4");

   gPluginMgr->AddHandler("TGuiFactory", "gtk4", "ROOT::Experimental::TGtk4GuiFactory",
      "libROOTGtk4Canvas", "TGtk4GuiFactory()");
}
