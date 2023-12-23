#pragma once

#include "Effect.h"
#include <map>

class Shadertoy : public Effect
{
public:
    struct UniformData
    {
        glm::vec4 iResolution;  // viewport resolution (in pixels)
        float iTime;            // shader playback time (in seconds)
        float iTimeDelta;       // render time (in seconds)
        float iFrameRate;       // shader frame rate
        int32_t iFrame;         // shader playback frame
        glm::vec4 iChannelTime; // channel playback time (in seconds)
        glm::vec4 iChannelResolution[4];    // channel resolution (in pixels)
        glm::vec4 iMouse;       // mouse pixel coords. xy: current (if MLB down), zw: click
        glm::vec4 iDate;        // (year, month, day, time in seconds)
    };

    enum class ChannelType
    {
        Undefined,
        Texture2D,
        TextureCube,
        Texture3D,
        RenderTarget,   // sample the render target of other pass as texture.
        // TODO: Keyboard, Webcam, Microphone, Music, Videos?
    };

    enum class ChannelFilter
    {
        Nearest,
        Linear,
        Mipmap,
    };

    enum class ChannelWrap
    {
        Clamp,
        Repeat,
    };

    struct ChannelInfo
    {
        ChannelType type;
        std::string name;
        ChannelFilter filter;
        ChannelWrap wrap;
        bool vflip;
        VulkanImage* images[2];
    };

    struct ShaderPass
    {
        std::string shaderName;
        std::vector<VulkanShaderMacro> shaderMacros;
        UniformData uniforms;
        rad::Ref<VulkanBuffer> uniformBuffer;
        std::vector<ChannelInfo> channels;
        rad::Ref<VulkanPipeline> pipeline;
        // ping-pong buffering, current pass can sample color buffers of last frame.
        rad::Ref<VulkanImage> colorBuffers[2];
        rad::Ref<VulkanFramebuffer> framebuffers[2];
        rad::Ref<VulkanDescriptorSet> descriptorSets[2];
    };
    static constexpr uint32_t MaxShaderPassCount = 8;

    Shadertoy(VulkanWindow* window);
    ~Shadertoy();

    virtual bool Load(const rad::JsonValueRef jConfig) override;
    virtual void Render() override;

    virtual void Resize(uint32_t width, uint32_t height);

    virtual void OnMouseButtonDown(const SDL_MouseButtonEvent& mouseButton) override;
    virtual void OnMouseButtonUp(const SDL_MouseButtonEvent& mouseButton) override;

private:
    void CreateResources();
    void CreateRenderPasses();
    void CreateColorBuffers(uint32_t width, uint32_t height);
    void CreatePipelines();
    void UpdateResourceBindings();

    std::vector<std::string> m_passNames;
    std::map<std::string, ShaderPass> m_passes;
    std::map<std::string, rad::Ref<VulkanImage>> m_textures;

    rad::Ref<VulkanRenderPass> m_renderPass;
    VkFormat m_colorFormat = VK_FORMAT_R8G8B8A8_UNORM;
    float m_renderRatioX = 1.0f;
    float m_renderRatioY = 1.0f;
    uint32_t m_renderWidth = 0;
    uint32_t m_renderHeight = 0;

    rad::Ref<VulkanDescriptorPool> m_descriptorPool;
    rad::Ref<VulkanDescriptorSetLayout> m_descriptorSetLayout;
    rad::Ref<VulkanPipelineLayout> m_pipelineLayout;

    rad::Ref<VulkanSampler> m_samplerNearestClamp;
    rad::Ref<VulkanSampler> m_samplerNearestRepeat;
    rad::Ref<VulkanSampler> m_samplerLinearClamp;
    rad::Ref<VulkanSampler> m_samplerLinearRepeat;

    bool m_resetTime = true;

    using Clock = std::chrono::high_resolution_clock;
    Clock::time_point m_startTime;
    Clock::time_point m_lastTime;
    int32_t m_frameIndex = 0;

    bool m_mouseButtonDown = false;
    bool m_mouseClick = false;

}; // class Shadertoy
