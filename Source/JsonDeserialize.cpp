#include "JsonDeserialize.h"

void FromJson(const rad::JsonValueRef& json, VulkanShaderMacro& macro)
{
    if (json.IsString())
    {
        auto strs = rad::StrSplit(json.GetString(), "=", true);
        if (strs.size() == 1)
        {
            macro.m_name = rad::StrTrim(strs[0]);
        }
        else if (strs.size() == 2)
        {
            macro.m_name = rad::StrTrim(strs[0]);
            macro.m_definition = rad::StrTrim(strs[1]);
        }
    }
}
