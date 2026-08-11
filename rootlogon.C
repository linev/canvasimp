{
   gPluginMgr->AddHandler("TGuiFactory", "raylib", "ROOT::Experimental::TRaylibGuiFactory",
      "libROOTRaylibCanvas", "TRaylibGuiFactory()");

   gSystem->AddDynamicPath(".");
}
