{
   gSystem->AddDynamicPath(".");

   gEnv->SetValue("Gui.Factory", "vulkan");

   gPluginMgr->AddHandler("TGuiFactory", "vulkan",
                          "TVulkanGuiFactory",
                          "libVulkanCanvas", "TVulkanGuiFactory()");
}
