#include "VFXLab.h"
#include "rad/DirectMedia/Application.h"
#include "rad/IO/Logging.h"

int main(int argc, char* argv[])
{
    rad::Ref<DirectMedia::Application> app = new DirectMedia::Application();
    if (!app->Init(argc, argv))
    {
        LogGlobal(Error, "DirectMedia::Application::Init failed!");
        return -1;
    }

    if (!app->LoadVulkanLibrary())
    {
        LogGlobal(Error, "Cannot find Vulkan loader!");
        return -1;
    }
    rad::Ref<VulkanInstance> vulkanInstance = new VulkanInstance();
    std::set<std::string> extensionNames = app->GetVulkanInstanceExtensions();
    vulkanInstance->Init(
        "VFXLab",
        VK_MAKE_VERSION(0, 0, 0),
        extensionNames
    );

    rad::Ref<VFXLab> window = new VFXLab();
    window->SetInstance(vulkanInstance);
    window->Init();

    return app->Run();
}
