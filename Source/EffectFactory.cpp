#include "EffectFactory.h"
#include "Shadertoy/Shadertoy.h"
#include "Logging.h"

rad::Ref<Effect> CreateEffect(VulkanWindow* window, std::string_view typeStr)
{
    if (rad::StrCaseEqual(typeStr, "Shadertoy"))
    {
        return RAD_NEW Shadertoy(window);
    }
    else
    {
        LogVFX(Error, "Invalid effect type: %s", typeStr.data());
    }
    return nullptr;
}
