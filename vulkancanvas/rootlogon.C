{
   gSystem->AddDynamicPath(".");

   gEnv->SetValue("Gui.Factory", "vulkan");

   gPluginMgr->AddHandler("TGuiFactory", "vulkan",
                          "ROOT::Experimental::TVulkanGuiFactory",
                          "libROOTVulkanCanvas", "TVulkanGuiFactory()");
}
