#include "Shadertoy.h"
#include "Logging.h"
#include "JsonDeserialize.h"

void FromJson(const rad::JsonValueRef& json, Shadertoy::ChannelType& type);
void FromJson(const rad::JsonValueRef& json, Shadertoy::ChannelFilter& filter);
void FromJson(const rad::JsonValueRef& json, Shadertoy::ChannelWrap& wrap);
void FromJson(const rad::JsonValueRef& json, Shadertoy::ChannelInfo& channelInfo);

bool Shadertoy::Load(const rad::JsonValueRef jConfig)
{
    m_passNames = jConfig["Passes"].GetVector<std::string>();
    for (const std::string& passName : m_passNames)
    {
        rad::JsonValueRef jPass = jConfig[passName];
        if (jPass)
        {
            ShaderPass& pass = m_passes[passName];
            pass.shaderName = jPass["Shader"].GetString();
            rad::JsonValueRef jShaderMacros = jPass["ShaderMacros"];
            if (jShaderMacros && jShaderMacros.IsArray())
            {
                pass.shaderMacros = jShaderMacros.GetVector<VulkanShaderMacro>();
            }
            rad::JsonValueRef jChannels = jPass["Channels"];
            if (jChannels)
            {
                pass.channels = jChannels.GetVector<ChannelInfo>();
            }
        }
    }

    rad::JsonValueRef jRenderRatio = jConfig["RenderRatio"];
    if (jRenderRatio && jRenderRatio.IsArray() && (jRenderRatio.ArraySize() == 2))
    {
        m_renderRatioX = jRenderRatio[0].Get(1.0f);
        m_renderRatioY = jRenderRatio[1].Get(1.0f);
    }

    int drawWidth = 0;
    int drawHeight = 0;
    m_window->GetDrawableSize(&drawWidth, &drawHeight);

    CreateResources();
    CreateRenderPasses();
    CreateColorBuffers(drawWidth, drawHeight);
    CreatePipelines();
    UpdateResourceBindings();

    return true;
}

void FromJson(const rad::JsonValueRef& json, Shadertoy::ChannelType& type)
{
    if (json.IsString())
    {
        const char* str = json.GetString();
        if (rad::StrCaseEqual(str, "Texture2D"))
        {
            type = Shadertoy::ChannelType::Texture2D;
        }
        else if (rad::StrCaseEqual(str, "Cubemap"))
        {
            type = Shadertoy::ChannelType::TextureCube;
        }
        else if (rad::StrCaseEqual(str, "Texture3D") ||
            rad::StrCaseEqual(str, "Volume"))
        {
            type = Shadertoy::ChannelType::Texture3D;
        }
        else if (rad::StrCaseEqual(str, "RenderTarget") ||
            rad::StrCaseEqual(str, "Buffer"))
        {
            type = Shadertoy::ChannelType::RenderTarget;
        }
        else
        {
            LogVFX(Error, "Invalid ChannelType: %s", str);
        }
    }
}

void FromJson(const rad::JsonValueRef& json, Shadertoy::ChannelFilter& filter)
{
    if (json.IsString())
    {
        const char* str = json.GetString();
        if (rad::StrCaseEqual(str, "nearest"))
        {
            filter = Shadertoy::ChannelFilter::Nearest;
        }
        else if (rad::StrCaseEqual(str, "linear"))
        {
            filter = Shadertoy::ChannelFilter::Linear;
        }
        else if (rad::StrCaseEqual(str, "mipmap"))
        {
            filter = Shadertoy::ChannelFilter::Mipmap;
        }
        else
        {
            LogVFX(Error, "Invalid ChannelFilter: %s", str);
        }
    }
}

void FromJson(const rad::JsonValueRef& json, Shadertoy::ChannelWrap& wrap)
{
    if (json.IsString())
    {
        const char* str = json.GetString();
        if (rad::StrCaseEqual(str, "clamp"))
        {
            wrap = Shadertoy::ChannelWrap::Clamp;
        }
        else if (rad::StrCaseEqual(str, "repeat"))
        {
            wrap = Shadertoy::ChannelWrap::Repeat;
        }
        else
        {
            LogVFX(Error, "Invalid ChannelWrap: %s", str);
        }
    }
    wrap = Shadertoy::ChannelWrap::Repeat;
}

void FromJson(const rad::JsonValueRef& json, Shadertoy::ChannelInfo& channelInfo)
{
    if (json.IsObject())
    {
        channelInfo.type = json["Type"].Get(Shadertoy::ChannelType::Undefined);
        channelInfo.name = json["Name"].Get("");
        channelInfo.filter = json["Filter"].Get(Shadertoy::ChannelFilter::Linear);
        channelInfo.wrap = json["Wrap"].Get(Shadertoy::ChannelWrap::Repeat);
        channelInfo.vflip = json["VFlip"].Get(false);
    }
}
