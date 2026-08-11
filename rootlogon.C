{
   gSystem->AddDynamicPath(".");

   gEnv->SetValue("Gui.Factory", "raylib");

   gPluginMgr->AddHandler("TGuiFactory", "raylib", "ROOT::Experimental::TRaylibGuiFactory",
      "libROOTRaylibCanvas", "TRaylibGuiFactory()");
}
